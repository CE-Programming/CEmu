#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils.h"
#include "sendinghandler.h"
#include "visualizerwidget.h"
#include "dockwidget.h"
#include "basiccodeviewerwindow.h"
#include "capture/animated-png.h"
#include "keypad/qtkeypadbridge.h"
#include "tivars_lib_cpp/src/TIModels.h"
#include "tivars_lib_cpp/src/TIVarTypes.h"
#include "tivars_lib_cpp/src/TypeHandlers/TypeHandlers.h"
#include "../../core/emu.h"
#include "../../core/asic.h"
#include "../../core/cpu.h"
#include "../../core/mem.h"
#include "../../core/extras.h"
#include "../../core/link.h"
#include "../../tests/autotester/crc32.hpp"
#include "../../tests/autotester/autotester.h"

#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QBuffer>
#include <QtCore/QProcess>
#include <QtGui/QFont>
#include <QtGui/QWindow>
#include <QtGui/QDesktopServices>
#include <QtGui/QClipboard>
#include <QtGui/QScreen>
#include <QShortcut> /* Different module in Qt5 vs Qt6 */
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QMessageBox>
#include <QtNetwork/QNetworkReply>
#include <fstream>
#include <iostream>
#include <cmath>

#ifdef Q_OS_MACX
    #include "os/mac/kdmactouchbar.h"
#endif

Q_DECLARE_METATYPE(calc_var_t)
Q_DECLARE_METATYPE(emu_state_t)
Q_DECLARE_METATYPE(emu_data_t)

#ifdef _MSC_VER
    #include <direct.h>
    #define chdir _chdir
#else
    #include <unistd.h>
#endif

MainWindow::MainWindow(CEmuOpts &cliOpts, QWidget *p) : QMainWindow(p), ui(new Ui::MainWindow), opts(cliOpts) {
    keypadBridge = new QtKeypadBridge(this); // This must be before setupUi for some reason >.>

    // setup translations
    if (m_appTranslator.load(QLocale::system().name(), QStringLiteral(":/i18n/i18n/"))) {
        qApp->installTranslator(&m_appTranslator);
    }

    // setting metatypes
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    qRegisterMetaTypeStreamOperators<QList<int>>("QList<int>");
    qRegisterMetaTypeStreamOperators<QList<bool>>("QList<bool>");
#else
    qRegisterMetaType<QList<int>>("QList<int>");
    qRegisterMetaType<QList<bool>>("QList<bool>");
#endif

    ui->setupUi(this);

#if (QT_VERSION < QT_VERSION_CHECK(6, 1, 0))
    m_styleForMode[0] = m_styleForMode[1] = QApplication::style()->objectName();
#else
    m_styleForMode[0] = m_styleForMode[1] = QApplication::style()->name();
#endif
    darkModeSwitch(isSystemInDarkMode());

    setStyleSheet(QStringLiteral("QMainWindow::separator{ width: 0px; height: 0px; }"));

    if (!ipcSetup()) {
        m_initPassed = false;
        return;
    }

#ifdef Q_OS_MACX
    KDMacTouchBar *touchBar = new KDMacTouchBar(this);
#endif

    // init tivars_lib stuff
    tivars::TIModels::initTIModelsArray();
    tivars::TIVarTypes::initTIVarTypesArray();
    tivars::TypeHandlers::TH_Tokenized::initTokens();

    ui->centralWidget->hide();
    ui->statusBar->addWidget(&m_speedLabel);
    ui->statusBar->addPermanentWidget(&m_msgLabel);
    ui->statusBar->addPermanentWidget(&m_fpsLabel);

    m_watchpoints = ui->watchpoints;
    m_breakpoints = ui->breakpoints;
    m_ports = ui->ports;
    m_disasm = ui->disasm;

    ui->console->setMaximumBlockCount(1000);

    setWindowTitle(QStringLiteral("CEmu | ") + opts.idString);

    connect(keypadBridge, &QtKeypadBridge::keyStateChanged, ui->keypadWidget, &KeypadWidget::changeKeyState);
    connect(keypadBridge, &QtKeypadBridge::sendKeys, &emu, &EmuThread::enqueueKeys);
    installEventFilter(keypadBridge);

    ui->centralWidget->installEventFilter(keypadBridge);
    ui->screenWidget->installEventFilter(keypadBridge);
    ui->tabWidget->installEventFilter(keypadBridge);

    // Same for all the tabs/docks (iterate over them instead of hardcoding their names)
    // ... except the Lua scripting one, which has things that can be used while emulation isn't paused
    for (const auto &tab : ui->tabWidget->children()[0]->children()) {
        if (tab == ui->tabScripting) {
            continue;
        }
        tab->installEventFilter(keypadBridge);
    }


    m_btnCancelTranser = new QPushButton(this);
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMaximumHeight(ui->statusBar->height() / 2);
    ui->statusBar->addWidget(m_progressBar);
    ui->statusBar->addWidget(m_btnCancelTranser);
    sendingHandler = new SendingHandler(this, m_btnCancelTranser, m_progressBar, ui->varLoadedView);

    m_varTableModel = new VarTableModel(ui->emuVarView);
    m_varTableSortFilterModel = new VarTableSortFilterModel(m_varTableModel);
    ui->emuVarView->setModel(m_varTableSortFilterModel);
    ui->emuVarView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->emuVarView->sortByColumn(VarTableModel::VAR_NAME_COL, Qt::AscendingOrder);
    ui->filterVarList->setItemData(0, -1);
    for (int varType = CALC_VAR_TYPE_REAL; varType < CALC_VAR_TYPE_OPERATING_SYSTEM; varType++) {
        if ((varType >= CALC_VAR_TYPE_UNDEF && varType <= CALC_VAR_TYPE_APP) ||
            varType != calc_var_normalized_type((calc_var_type_t)varType))
            continue;
        QString varTypeName = calc_var_type_names[varType];
        if (!varTypeName.contains(QStringLiteral("Unknown")) &&
            !varTypeName.contains(QStringLiteral("Temp")) &&
            !varTypeName.contains(QStringLiteral("New"))) {
            ui->filterVarList->addItem(varTypeName.replace(QStringLiteral("Real"), QStringLiteral("Real/Complex")), varType);
        }
    }
    // Scrollbar truncates text for some reason, so just show all items
    ui->filterVarList->setMaxVisibleItems(ui->filterVarList->count());
    connect(ui->filterVarList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        m_varTableSortFilterModel, [this](int index) { m_varTableSortFilterModel->setTypeFilter(ui->filterVarList->itemData(index).toInt()); });

    // emulator -> gui (Should be queued)
    connect(&emu, &EmuThread::consoleStr, this, &MainWindow::consoleStr, Qt::UniqueConnection);
    connect(&emu, &EmuThread::consoleClear, this, &MainWindow::consoleClear, Qt::QueuedConnection);
    connect(&emu, &EmuThread::sendAsicRevInfo, this, &MainWindow::showAsicRevInfo, Qt::QueuedConnection);
    connect(&emu, &EmuThread::sendSpeed, this, &MainWindow::showEmuSpeed, Qt::QueuedConnection);
    connect(&emu, &EmuThread::debugDisable, this, &MainWindow::debugDisable, Qt::QueuedConnection);
    connect(&emu, &EmuThread::debugCommand, this, &MainWindow::debugCommand, Qt::QueuedConnection);
    connect(&emu, &EmuThread::saved, this, &MainWindow::emuSaved, Qt::QueuedConnection);
    connect(&emu, &EmuThread::loaded, this, &MainWindow::emuCheck, Qt::QueuedConnection);
    connect(&emu, &EmuThread::blocked, this, &MainWindow::emuBlocked, Qt::QueuedConnection);
    connect(&emu, &EmuThread::tested, this, &MainWindow::autotesterTested, Qt::QueuedConnection);

    // console actions
    connect(ui->buttonConsoleclear, &QPushButton::clicked, ui->console, &QPlainTextEdit::clear);
    connect(ui->radioDock, &QRadioButton::clicked, this, &MainWindow::consoleModified);
    connect(ui->radioConsole, &QRadioButton::clicked, this, &MainWindow::consoleModified);

    // debug actions
    connect(ui->buttonRun, &QPushButton::clicked, this, &MainWindow::debugToggle);
    connect(ui->checkADLDisasm, &QCheckBox::stateChanged, this, &MainWindow::disasmUpdate);
    connect(ui->checkADLStack, &QCheckBox::stateChanged, this, &MainWindow::stackUpdate);
    connect(ui->checkADL, &QCheckBox::stateChanged, [this]{ disasmUpdate(); stackUpdate(); });
    connect(ui->buttonAddPort, &QPushButton::clicked, this, &MainWindow::portAddSlot);
    connect(ui->buttonAddBreakpoint, &QPushButton::clicked, this, &MainWindow::breakAddSlot);
    connect(ui->buttonAddWatchpoint, &QPushButton::clicked, this, &MainWindow::watchAddSlot);
    connect(ui->buttonStepIn, &QPushButton::clicked, this, &MainWindow::stepIn);
    connect(ui->buttonStepOver, &QPushButton::clicked, this, &MainWindow::stepOver);
    connect(ui->buttonStepNext, &QPushButton::clicked, this, &MainWindow::stepNext);
    connect(ui->buttonStepOut, &QPushButton::clicked, this, &MainWindow::stepOut);
    connect(ui->buttonGoto, &QPushButton::clicked, this, &MainWindow::gotoPressed);
    connect(ui->console, &QWidget::customContextMenuRequested, this, &MainWindow::contextConsole);
    connect(m_disasm, &QWidget::customContextMenuRequested, this, &MainWindow::contextDisasm);
    connect(ui->vatView, &QWidget::customContextMenuRequested, this, &MainWindow::contextVat);
    connect(ui->opView, &QWidget::customContextMenuRequested, this, &MainWindow::contextOp);
    connect(ui->opStack, &QWidget::customContextMenuRequested, this, &MainWindow::contextOp);
    connect(ui->fpStack, &QWidget::customContextMenuRequested, this, &MainWindow::contextOp);
    connect(m_ports, &QTableWidget::itemChanged, this, &MainWindow::portModified);
    connect(m_ports, &QTableWidget::currentItemChanged, this, &MainWindow::portSetPrev);
    connect(m_ports, &QTableWidget::itemPressed, [this](QTableWidgetItem *item){ portSetPrev(item, Q_NULLPTR); });
    connect(m_breakpoints, &QTableWidget::itemChanged, this, &MainWindow::breakModified);
    connect(m_breakpoints, &QTableWidget::currentItemChanged, this, &MainWindow::breakSetPrev);
    connect(m_breakpoints, &QTableWidget::itemPressed, [this](QTableWidgetItem *item){ breakSetPrev(item, Q_NULLPTR); });
    connect(m_watchpoints, &QTableWidget::itemChanged, this, &MainWindow::watchModified);
    connect(m_watchpoints, &QTableWidget::currentItemChanged, this, &MainWindow::watchSetPrev);
    connect(m_watchpoints, &QTableWidget::itemPressed, [this](QTableWidgetItem *item){ watchSetPrev(item, Q_NULLPTR); });
    connect(ui->checkCharging, &QCheckBox::toggled, this, &MainWindow::batterySetCharging);
    connect(ui->sliderBattery, &QSlider::valueChanged, this, &MainWindow::batterySet);
    connect(ui->checkAddSpace, &QCheckBox::toggled, this, &MainWindow::setDebugDisasmSpace);
    connect(ui->checkOperandTab, &QCheckBox::toggled, this, &MainWindow::setDebugDisasmTab);
    connect(ui->checkDisableSoftCommands, &QCheckBox::toggled, this, &MainWindow::setDebugSoftCommands);
    connect(ui->buttonZero, &QPushButton::clicked, this, &MainWindow::debugZeroCycles);
    connect(ui->buttonCertID, &QPushButton::clicked, this, &MainWindow::setCalcId);
    connect(m_disasm, &DataWidget::gotoDisasmAddress, this, &MainWindow::gotoDisasmAddr);
    connect(m_disasm, &DataWidget::gotoMemoryAddress, this, &MainWindow::gotoMemAddr);

#ifdef Q_OS_MACX
    {
        QAction *action = new QAction(ui->actionReportBug->icon(), tr("Run/Stop"));
        touchBar->addAction(action);
        connect(action, &QAction::triggered, this, &MainWindow::debugToggle);
    }
    {
        QAction *action = new QAction(ui->buttonStepIn->icon(), "in");
        touchBar->addAction(action);
        connect(action, &QAction::triggered, this, &MainWindow::stepIn);
    }
    {
        QAction *action = new QAction(ui->buttonStepOver->icon(), "over");
        touchBar->addAction(action);
        connect(action, &QAction::triggered, this, &MainWindow::stepOver);
    }
    {
        QAction *action = new QAction(ui->buttonStepNext->icon(), "next");
        touchBar->addAction(action);
        connect(action, &QAction::triggered, this, &MainWindow::stepNext);
    }
    {
        QAction *action = new QAction(ui->buttonStepOut->icon(), "out");
        touchBar->addAction(action);
        connect(action, &QAction::triggered, this, &MainWindow::stepOut);
    }
#endif

    // ctrl + click
    connect(ui->console, &QPlainTextEdit::cursorPositionChanged, [this]{ handleCtrlClickText(ui->console); });
    connect(ui->stackView, &QPlainTextEdit::cursorPositionChanged, [this]{ handleCtrlClickText(ui->stackView); });
    connect(ui->hlregView, &QLineEdit::cursorPositionChanged, [this]{ handleCtrlClickLine(ui->hlregView); });
    connect(ui->bcregView, &QLineEdit::cursorPositionChanged, [this]{ handleCtrlClickLine(ui->bcregView); });
    connect(ui->deregView, &QLineEdit::cursorPositionChanged, [this]{ handleCtrlClickLine(ui->deregView); });
    connect(ui->ixregView, &QLineEdit::cursorPositionChanged, [this]{ handleCtrlClickLine(ui->ixregView); });
    connect(ui->iyregView, &QLineEdit::cursorPositionChanged, [this]{ handleCtrlClickLine(ui->iyregView); });
    connect(ui->pcregView, &QLineEdit::cursorPositionChanged, [this]{ handleCtrlClickLine(ui->pcregView); });
    connect(ui->splregView, &QLineEdit::cursorPositionChanged, [this]{ handleCtrlClickLine(ui->splregView); });
    connect(ui->spsregView, &QLineEdit::cursorPositionChanged, [this]{ handleCtrlClickLine(ui->spsregView); });
    connect(ui->hl_regView, &QLineEdit::cursorPositionChanged, [this]{ handleCtrlClickLine(ui->hl_regView); });
    connect(ui->bc_regView, &QLineEdit::cursorPositionChanged, [this]{ handleCtrlClickLine(ui->bc_regView); });
    connect(ui->de_regView, &QLineEdit::cursorPositionChanged, [this]{ handleCtrlClickLine(ui->de_regView); });

    // debug options
    connect(ui->buttonAddEquateFile, &QToolButton::clicked, this, &MainWindow::equatesAddDialog);
    connect(ui->buttonClearEquates, &QToolButton::clicked, this, &MainWindow::equatesClear);
    connect(ui->buttonRefreshEquates, &QToolButton::clicked, this, &MainWindow::equatesRefresh);
    connect(ui->buttonToggleBreakpoints, &QToolButton::toggled, this, &MainWindow::setDebugIgnoreBreakpoints);
    connect(ui->checkDebugResetTrigger, &QCheckBox::toggled, this, &MainWindow::setDebugResetTrigger);
    connect(ui->checkDisasmDataCol, &QCheckBox::toggled, this, &MainWindow::setDebugDisasmDataCol);
    connect(ui->checkDisasmBoldSymbols, &QCheckBox::toggled, this, &MainWindow::setDebugDisasmBoldSymbols);
    connect(ui->checkDisasmAddr, &QCheckBox::toggled, this, &MainWindow::setDebugDisasmAddrCol);
    connect(ui->checkDisasmImplict, &QCheckBox::toggled, this, &MainWindow::setDebugDisasmImplict);
    connect(ui->checkDisasmUppercase, &QCheckBox::toggled, this, &MainWindow::setDebugDisasmUppercase);
    connect(ui->checkDebugDma, &QCheckBox::toggled, this, &MainWindow::setDebugLcdDma);
    connect(ui->textSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::setFont);

    // debug files
    connect(ui->actionImportDebugger, &QAction::triggered, [this]{ debugImportFile(debugGetFile(false)); });
    connect(ui->actionExportDebugger, &QAction::triggered, [this]{ debugExportFile(debugGetFile(true)); });

    // linking
    connect(ui->buttonSend, &QPushButton::clicked, this, &MainWindow::varSelect);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::varSelect);
    connect(ui->buttonEnableVarList, &QPushButton::clicked, this, &MainWindow::varToggle);
    connect(ui->buttonRefreshVarList, &QPushButton::clicked, this, &MainWindow::varShow);
    connect(ui->buttonReceiveFile, &QPushButton::clicked, this, &MainWindow::varSaveSelected);
    connect(ui->buttonReceiveFiles, &QPushButton::clicked, this, &MainWindow::varSaveSelectedFiles);
    connect(ui->buttonResendFiles, &QPushButton::clicked, this, &MainWindow::varResend);

    // autotester
    connect(ui->buttonOpenJSONconfig, &QPushButton::clicked, this, &MainWindow::autotesterLoad);
    connect(ui->buttonReloadJSONconfig, &QPushButton::clicked, this, &MainWindow::autotesterReload);
    connect(ui->buttonLaunchTest, &QPushButton::clicked, this, &MainWindow::autotesterLaunch);
    connect(ui->comboBoxPresetCRC, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::autotesterUpdatePresets);
    connect(ui->buttonRefreshCRC, &QPushButton::clicked, this, &MainWindow::autotesterRefreshCRC);

    // menubar actions
    connect(ui->actionSetup, &QAction::triggered, this, &MainWindow::runSetup);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionScreenshot, &QAction::triggered, this, &MainWindow::screenshot);
#ifdef PNG_WRITE_APNG_SUPPORTED
    connect(ui->actionRecordAnimated, &QAction::triggered, this, &MainWindow::recordAnimated);
#endif
    connect(ui->actionSaveState, &QAction::triggered, [this]{ stateToPath(m_pathImage); });
    connect(ui->actionExportCalculatorState, &QAction::triggered, this, &MainWindow::stateToFile);
    connect(ui->actionExportRomImage, &QAction::triggered, this, &MainWindow::romExport);
    connect(ui->actionExportRamImage, &QAction::triggered, this, &MainWindow::ramExport);
    connect(ui->actionImportRamImage, &QAction::triggered, this, &MainWindow::ramImport);
    connect(ui->actionImportCalculatorState, &QAction::triggered, this, &MainWindow::stateFromFile);
    connect(ui->actionReloadROM, &QAction::triggered, [this]{ emuLoad(EMU_DATA_ROM); });
    connect(ui->actionRestoreState, &QAction::triggered, [this]{ emuLoad(EMU_DATA_IMAGE); });
    connect(ui->actionResetALL, &QAction::triggered, this, &MainWindow::resetCEmu);
    connect(ui->actionResetGUI, &QAction::triggered, this, &MainWindow::resetGui);
    connect(ui->actionResetCalculator, &QAction::triggered, this, &MainWindow::resetEmu);
    connect(ui->actionHideMenuBar, &QAction::triggered, this, &MainWindow::setMenuBarState);
    connect(ui->actionHideStatusBar, &QAction::triggered, this, &MainWindow::setStatusBarState);
    connect(ui->buttonResetCalculator, &QPushButton::clicked, this, &MainWindow::resetEmu);
    connect(ui->buttonReloadROM, &QPushButton::clicked, [this]{ emuLoad(EMU_DATA_ROM); });

#ifdef Q_OS_MACX
    touchBar->addSeparator();
    {
        QAction *resetAction = new QAction(ui->actionResetCalculator->icon(), tr("Reset"));
        QAction *reloadAction = new QAction(ui->actionReloadROM->icon(), tr("Reload"));
        touchBar->addActions({ resetAction, reloadAction });

        connect(resetAction, &QAction::triggered, this, [touchBar, resetAction, reloadAction, this] {
            touchBar->removeAction(resetAction);
            touchBar->removeAction(reloadAction);
            QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            touchBar->addDialogButtonBox(buttonBox);
            connect(buttonBox, &QDialogButtonBox::accepted, this, [touchBar, buttonBox, resetAction, reloadAction, this] {
                this->resetEmu();
                touchBar->removeDialogButtonBox(buttonBox);
                touchBar->addActions({ resetAction, reloadAction });
            });
            connect(buttonBox, &QDialogButtonBox::rejected, this, [touchBar, buttonBox, resetAction, reloadAction] {
                touchBar->removeDialogButtonBox(buttonBox);
                touchBar->addActions({ resetAction, reloadAction });
            });
        });

        connect(reloadAction, &QAction::triggered, this, [touchBar, resetAction, reloadAction, this] {
            touchBar->removeAction(resetAction);
            touchBar->removeAction(reloadAction);
            QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            touchBar->addDialogButtonBox(buttonBox);
            connect(buttonBox, &QDialogButtonBox::accepted, this, [touchBar, buttonBox, resetAction, reloadAction, this] {
                this->emuLoad(EMU_DATA_ROM);
                touchBar->removeDialogButtonBox(buttonBox);
                touchBar->addActions({ resetAction, reloadAction });
            });
            connect(buttonBox, &QDialogButtonBox::rejected, this, [touchBar, buttonBox, resetAction, reloadAction] {
                touchBar->removeDialogButtonBox(buttonBox);
                touchBar->addActions({ resetAction, reloadAction });
            });
        });
    }
#endif

    // lcd flow
    connect(ui->lcd, &LCDWidget::updateLcd, this, &MainWindow::lcdUpdate, Qt::QueuedConnection);
    connect(this, &MainWindow::setLcdResponseMode, ui->lcd, &LCDWidget::setResponseMode);
    connect(this, &MainWindow::setLcdFrameskip, ui->lcd, &LCDWidget::setFrameskip);
    connect(ui->statusInterval, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::setStatusInterval);
    connect(&m_timerEmu, &QTimer::timeout, [this]{ if (std::exchange(m_timerEmuTriggered, true)) timeoutEmuSpeed(); });
    connect(&m_timerFps, &QTimer::timeout, [this]{ if (std::exchange(m_timerFpsTriggered, true)) timeoutFpsSpeed(); });

    // screen capture
    connect(ui->buttonSavePNG, &QPushButton::clicked, this, &MainWindow::screenshot);
    connect(ui->buttonCopyPNG, &QPushButton::clicked, this, &MainWindow::lcdCopy);
    connect(ui->actionClipScreen, &QAction::triggered, this, &MainWindow::lcdCopy);
#ifdef PNG_WRITE_APNG_SUPPORTED
    connect(ui->buttonRecordAnimated, &QPushButton::clicked, this, &MainWindow::recordAnimated);
    connect(ui->apngSkip, &QSlider::valueChanged, this, &MainWindow::setFrameskip);
    connect(ui->checkOptimizeRecording, &QCheckBox::stateChanged, this, &MainWindow::setOptimizeRecord);
#else
    ui->actionRecordAnimated->setEnabled(false);
    ui->buttonRecordAnimated->setEnabled(false);
    ui->apngSkip->setEnabled(false);
    ui->checkOptimizeRecording->setEnabled(false);
#endif

    // about menus
    connect(ui->actionCheckForUpdates, &QAction::triggered, [this]{ checkUpdate(true); });
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(ui->actionReportBug, &QAction::triggered, []{ QDesktopServices::openUrl(QUrl("https://github.com/CE-Programming/CEmu/issues")); });

    // other gui actions
    connect(ui->checkAllowGroupDrag, &QCheckBox::stateChanged, this, &MainWindow::setDockGroupDrag);
    connect(ui->buttonRunSetup, &QPushButton::clicked, this, &MainWindow::runSetup);
    connect(ui->scaleLCD, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::setLcdScale);
    connect(ui->upscaleLCD, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::setLcdUpscale);
    connect(ui->fullscreenLCD, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::setLcdFullscreen);
    connect(ui->guiSkip, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::setGuiSkip);
    connect(ui->checkSkin, &QCheckBox::stateChanged, this, &MainWindow::setSkinToggle);
    connect(ui->comboBoxAsicRev, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::setAsicRevision);
    connect(ui->checkPythonEdition, &QCheckBox::stateChanged, this, &MainWindow::setPythonEdition);
    connect(ui->checkDma, &QCheckBox::toggled, this, &MainWindow::setLcdDma);
    connect(ui->checkGamma, &QCheckBox::toggled, this, &MainWindow::setLcdGamma);
    connect(ui->checkResponse, &QCheckBox::toggled, this, &MainWindow::setLcdResponse);
    connect(ui->checkAlwaysOnTop, &QCheckBox::stateChanged, this, &MainWindow::setTop);
    connect(ui->emulationSpeed, &QSlider::valueChanged, this, &MainWindow::setEmuSpeed);
    connect(ui->emulationSpeedSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::setEmuSpeed);
    connect(ui->checkThrottle, &QCheckBox::stateChanged, this, &MainWindow::setThrottle);
    connect(ui->checkAutoEquates, &QCheckBox::stateChanged, this, &MainWindow::setDebugAutoEquates);
    connect(ui->checkSaveRestore, &QCheckBox::stateChanged, this, &MainWindow::setAutoSave);
    connect(ui->checkPortable, &QCheckBox::stateChanged, this, &MainWindow::setPortable);
    connect(ui->checkSaveRecent, &QCheckBox::stateChanged, this, &MainWindow::setRecentSave);
    connect(ui->checkSaveLoadDebug, &QCheckBox::stateChanged, this, &MainWindow::setDebugAutoSave);
    connect(ui->buttonChangeSavedImagePath, &QPushButton::clicked, this, &MainWindow::setImagePath);
    connect(ui->buttonChangeSavedDebugPath, &QPushButton::clicked, this, &MainWindow::setDebugPath);
    connect(ui->checkFocus, &QCheckBox::stateChanged, this, &MainWindow::setFocusSetting);
    connect(ui->checkAllowAnyRev, &QCheckBox::stateChanged, this, &MainWindow::setAllowAnyRev);
    connect(ui->checkNormOs, &QCheckBox::stateChanged, this, &MainWindow::setNormalOs);
    connect(ui->flashBytes, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui->flashEdit, &HexWidget::setBytesPerLine);
    connect(ui->ramBytes, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui->ramEdit, &HexWidget::setBytesPerLine);
    connect(ui->ramAscii, &QToolButton::toggled, [this](bool set){ ui->ramEdit->setAsciiArea(set); });
    connect(ui->flashAscii, &QToolButton::toggled, [this](bool set){ ui->flashEdit->setAsciiArea(set); });
    connect(ui->emuVarView, &QTableView::doubleClicked, this, &MainWindow::varPressed);
    connect(ui->emuVarView, &QTableView::customContextMenuRequested, this, &MainWindow::contextVars);
    connect(ui->buttonAddSlot, &QPushButton::clicked, this, &MainWindow::stateAddNew);
    connect(ui->actionExportCEmuImage, &QAction::triggered, this, &MainWindow::bootImageExport);
    connect(ui->lcd, &LCDWidget::sendROM, this, &MainWindow::setRom);
    connect(ui->lcd, &LCDWidget::customContextMenuRequested, this, &MainWindow::contextLcd);
    connect(ui->checkUpdates, &QCheckBox::stateChanged, this, &MainWindow::setAutoUpdates);

    // languages
    connect(ui->actionEnglish,  &QAction::triggered, [this]{ translateSwitch(QStringLiteral("en_EN")); });
    connect(ui->actionFran_ais, &QAction::triggered, [this]{ translateSwitch(QStringLiteral("fr_FR")); });
    connect(ui->actionDutch,    &QAction::triggered, [this]{ translateSwitch(QStringLiteral("nl_NL")); });
    connect(ui->actionEspanol,  &QAction::triggered, [this]{ translateSwitch(QStringLiteral("es_ES")); });
    connect(ui->actionChinese,  &QAction::triggered, [this]{ translateSwitch(QStringLiteral("zh_CN")); });

    // sending handler
    connect(sendingHandler, &SendingHandler::send, &emu, &EmuThread::send, Qt::QueuedConnection);
    connect(sendingHandler, &SendingHandler::cancelTransfer, &emu, &EmuThread::cancelTransfer, Qt::QueuedConnection);
    connect(&emu, &EmuThread::linkProgress, sendingHandler, &SendingHandler::linkProgress, Qt::QueuedConnection);
    connect(sendingHandler, &SendingHandler::loadEquateFile, this, &MainWindow::equatesAddFile);
    connect(sendingHandler, &SendingHandler::sendFinished, this, [this] { if (guiReceive) { varShow(); } });

    // memory editors
    connect(ui->buttonFlashSearch, &QPushButton::clicked, [this]{ memSearchEdit(ui->flashEdit); });
    connect(ui->buttonRamSearch, &QPushButton::clicked, [this]{ memSearchEdit(ui->ramEdit); });
    connect(ui->buttonFlashGoto, &QPushButton::clicked, this, &MainWindow::flashGotoPressed);
    connect(ui->buttonRamGoto, &QPushButton::clicked, this, &MainWindow::ramGotoPressed);
    connect(ui->buttonFlashSync, &QToolButton::clicked, this, &MainWindow::flashSyncPressed);
    connect(ui->buttonRamSync, &QToolButton::clicked, this, &MainWindow::ramSyncPressed);
    connect(ui->flashEdit, &HexWidget::customContextMenuRequested, this, &MainWindow::contextMem);
    connect(ui->ramEdit, &HexWidget::customContextMenuRequested, this, &MainWindow::contextMem);
    connect(ui->flashEdit, &HexWidget::focused, [this] { m_memWidget = Q_NULLPTR; });
    connect(ui->ramEdit, &HexWidget::focused, [this] { m_memWidget = Q_NULLPTR; });

    // keymap
    connect(ui->radioNaturalKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radioCEmuKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radioTilEmKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radioWabbitemuKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radiojsTIfiedKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radioSmartPadKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radioCustomKeys, &QRadioButton::clicked, this, &MainWindow::keymapCustomSelected);

    // keypad
    connect(ui->buttonTrueBlue, &QPushButton::clicked, this, &MainWindow::keypadChanged);
    connect(ui->buttonDenim, &QPushButton::clicked, this, &MainWindow::keypadChanged);
    connect(ui->buttonPink, &QPushButton::clicked, this, &MainWindow::keypadChanged);
    connect(ui->buttonPlum, &QPushButton::clicked, this, &MainWindow::keypadChanged);
    connect(ui->buttonRed, &QPushButton::clicked, this, &MainWindow::keypadChanged);
    connect(ui->buttonLightning, &QPushButton::clicked, this, &MainWindow::keypadChanged);
    connect(ui->buttonGolden, &QPushButton::clicked, this, &MainWindow::keypadChanged);
    connect(ui->buttonWhite, &QPushButton::clicked, this, &MainWindow::keypadChanged);
    connect(ui->buttonBlack, &QPushButton::clicked, this, &MainWindow::keypadChanged);
    connect(ui->buttonSilver, &QPushButton::clicked, this, &MainWindow::keypadChanged);
    connect(ui->buttonSpaceGrey, &QPushButton::clicked, this, &MainWindow::keypadChanged);
    connect(ui->buttonCoral, &QPushButton::clicked, this, &MainWindow::keypadChanged);
    connect(ui->buttonMint, &QPushButton::clicked, this, &MainWindow::keypadChanged);
    connect(ui->buttonRoseGold, &QPushButton::clicked, this, &MainWindow::keypadChanged);
    connect(ui->buttonMatteBlack, &QPushButton::clicked, this, &MainWindow::keypadChanged);
    connect(ui->buttonTangentTeal, &QPushButton::clicked, this, &MainWindow::keypadChanged);
    connect(ui->buttonTotallyTeal, &QPushButton::clicked, this, &MainWindow::keypadChanged);

    // gui configurations
    connect(ui->actionExportWindowConfig, &QAction::triggered, this, &MainWindow::guiExport);
    connect(ui->actionImportWindowConfig, &QAction::triggered, this, &MainWindow::guiImport);

    // keypad mappings
    connect(ui->actionExportKeypadMapping, &QAction::triggered, this, &MainWindow::keymapExport);

    // application state
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &MainWindow::pauseEmu);

    // process communication
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::ipcSpawn);
    connect(ui->actionChangeID, &QAction::triggered, this, &MainWindow::ipcSetId);
    connect(&com, &InterCom::readDone, this, &MainWindow::ipcReceived);

    // docks
    translateExtras(TRANSLATE_INIT);
    m_actionToggleUI = new QAction(MSG_EDIT_UI, this);
    m_actionToggleUI->setCheckable(true);
    connect(m_actionToggleUI, &QAction::triggered, [this]{ setUIEditMode(!m_uiEditMode); });

    m_actionAddMemory = new QAction(MSG_ADD_MEMORY, this);
    connect(m_actionAddMemory, &QAction::triggered, [this]{ addMemDock(randomString(20), 8, true); });

    m_actionAddVisualizer = new QAction(MSG_ADD_VISUALIZER, this);
    connect(m_actionAddVisualizer, &QAction::triggered, [this]{ addVisualizerDock(randomString(20), QString()); });

    // already have action for key history
    connect(ui->actionKeyHistory, &QAction::triggered, [this]{ addKeyHistoryDock(randomString(20), 9); });

    // shortcut connections
    m_shortcutStepIn = new QShortcut(QKeySequence(Qt::Key_F6), this);
    m_shortcutStepOver = new QShortcut(QKeySequence(Qt::Key_F7), this);
    m_shortcutStepNext = new QShortcut(QKeySequence(Qt::Key_F8), this);
    m_shortcutStepOut = new QShortcut(QKeySequence(Qt::Key_F9), this);
    m_shortcutDebug = new QShortcut(QKeySequence(Qt::Key_F10), this);
    m_shortcutFullscreen = new QShortcut(QKeySequence(Qt::Key_F11), this);
    m_shortcutAsm = new QShortcut(QKeySequence(Qt::Key_Pause), this);
    m_shortcutResend = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_X), this);

    m_shortcutFullscreen->setAutoRepeat(false);
    m_shortcutDebug->setAutoRepeat(false);
    m_shortcutAsm->setAutoRepeat(false);
    m_shortcutResend->setAutoRepeat(false);

    connect(m_shortcutFullscreen, &QShortcut::activated, [this]{ setFullscreen(m_fullscreen + 1); });
    connect(m_shortcutResend, &QShortcut::activated, this, &MainWindow::varResend);
    connect(m_shortcutAsm, &QShortcut::activated, [this]{ sendEmuKey(CE_KEY_ASM); });
    connect(m_shortcutDebug, &QShortcut::activated, this, &MainWindow::debugToggle);
    connect(m_shortcutStepIn, &QShortcut::activated, this, &MainWindow::stepIn);
    connect(m_shortcutStepOver, &QShortcut::activated, this, &MainWindow::stepOver);
    connect(m_shortcutStepNext, &QShortcut::activated, this, &MainWindow::stepNext);
    connect(m_shortcutStepOut, &QShortcut::activated, this, &MainWindow::stepOut);

    m_ports->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_breakpoints->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_watchpoints->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    // Lua
    connect(ui->buttonRunLuaScript, &QPushButton::clicked, this, &MainWindow::runLuaScript);
    connect(ui->buttonLoadLuaScript, &QPushButton::clicked, this, &MainWindow::loadLuaScript);
    connect(ui->buttonSaveLuaScript, &QPushButton::clicked, this, &MainWindow::saveLuaScript);
    connect(ui->resetREPLLuaState, &QPushButton::clicked, this, [&](){ this->initLuaThings(repl_lua, true); });
    connect(ui->clearREPLConsole, &QPushButton::clicked, ui->REPLConsole, &QPlainTextEdit::clear);
    connect(ui->REPLInput, &QLineEdit::returnPressed, this, &MainWindow::LuaREPLeval);

    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    // configure table font
    QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    fixedFont.setStyleHint(QFont::Monospace);
    ui->breakpoints->setFont(fixedFont);
    ui->watchpoints->setFont(fixedFont);
    ui->ports->setFont(fixedFont);

    // absolute paths
    QString portableConfig = appDir().path() + SETTING_DEFAULT_CONFIG_FILE;
    QString sharedConfig = configPath + SETTING_DEFAULT_CONFIG_FILE;

    if (opts.settingsFile.isEmpty()) {
        if (bootImageCheck()) {
            m_pathConfig = sharedConfig;
        } else if (fileExists(portableConfig)) {
            m_pathConfig = portableConfig;
            m_portable = true;
        } else {
            m_pathConfig = sharedConfig;
        }
    } else {
        m_pathConfig = QFileInfo(opts.settingsFile).absoluteFilePath();
    }

    if (opts.useSettings) {
        m_config = new QSettings(m_pathConfig, QSettings::IniFormat);
    } else {
        ui->checkPortable->blockSignals(true);
        ui->checkPortable->setEnabled(false);
        ui->checkPortable->blockSignals(false);
        m_config = new QSettings();
        m_config->clear();
    }
    if (m_loadedBootImage) {
        m_config->setValue(SETTING_FIRST_RUN, false);
    }

    QFileInfo configDir(QFileInfo(m_pathConfig).path());
    if (opts.useSettings && configDir.isDir() && !configDir.isWritable()) {
        m_pathConfig = portableConfig;
        ui->pathConfig->setText(portableConfig);
        m_portableActivated = true;
        m_portable = true;
    }

    setAutoUpdates(m_config->value(SETTING_AUTOUPDATE, CEMU_RELEASE).toBool());
    checkVersion();

#ifdef Q_OS_WIN
    installToggleConsole();
#endif

#ifdef Q_OS_MACX
    ui->actionHideMenuBar->setVisible(false);
#endif

    iconsLoad();
    memLoadState();
    optLoadFiles(opts);
    setFrameskip(m_config->value(SETTING_CAPTURE_FRAMESKIP, 1).toInt());
    setOptimizeRecord(m_config->value(SETTING_CAPTURE_OPTIMIZE, true).toBool());
    setStatusInterval(m_config->value(SETTING_STATUS_INTERVAL, 1).toInt());
    setLcdScale(m_config->value(SETTING_SCREEN_SCALE, 100).toInt());
    setLcdUpscale(m_config->value(SETTING_SCREEN_UPSCALE, LCDWidget::SharpBilinear).toInt());
    setLcdFullscreen(m_config->value(SETTING_SCREEN_FULLSCREEN, LCDWidget::PreserveAspectRatio).toInt());
    setSkinToggle(m_config->value(SETTING_SCREEN_SKIN, true).toBool());
    setLcdDma(m_config->value(SETTING_SCREEN_DMA, true).toBool());
    setLcdGamma(m_config->value(SETTING_SCREEN_GAMMA, true).toBool());
    setLcdResponse(m_config->value(SETTING_SCREEN_RESPONSE, true).toBool());
    setGuiSkip(m_config->value(SETTING_SCREEN_FRAMESKIP, 0).toInt());
    setKeypadHolding(m_config->value(SETTING_KEYPAD_HOLDING, true).toBool());
    setEmuSpeed(m_config->value(SETTING_EMUSPEED, 100).toInt());
    ui->checkSaveRestore->setChecked(m_config->value(SETTING_SAVE_ON_CLOSE, true).toBool());
    setFont(m_config->value(SETTING_DEBUGGER_TEXT_SIZE, 9).toInt());
    setDebugDisasmSpace(m_config->value(SETTING_DEBUGGER_DISASM_SPACE, false).toBool());
    setDebugDisasmTab(m_config->value(SETTING_DEBUGGER_DISASM_TAB, false).toBool());
    setDebugDisasmAddrCol(m_config->value(SETTING_DEBUGGER_ADDR_COL, true).toBool());
    setDebugDisasmDataCol(m_config->value(SETTING_DEBUGGER_DATA_COL, true).toBool());
    setDebugDisasmBoldSymbols(m_config->value(SETTING_DEBUGGER_BOLD_SYMBOLS, false).toBool());
    setDebugDisasmUppercase(m_config->value(SETTING_DEBUGGER_UPPERCASE, false).toBool());
    setDebugDisasmImplict(m_config->value(SETTING_DEBUGGER_IMPLICT, false).toBool());
    setDebugAutoSave(m_config->value(SETTING_DEBUGGER_RESTORE_ON_OPEN, false).toBool());
    setDebugAutoEquates(m_config->value(SETTING_DEBUGGER_AUTO_EQUATES, false).toBool());
    setDebugResetTrigger(m_config->value(SETTING_DEBUGGER_RESET_OPENS, false).toBool());
    setDebugIgnoreBreakpoints(m_config->value(SETTING_DEBUGGER_BREAK_IGNORE, false).toBool());
    setDebugSoftCommands(m_config->value(SETTING_DEBUGGER_ENABLE_SOFT, true).toBool());
    setAllowAnyRev(m_config->value(SETTING_DEBUGGER_ALLOW_ANY_REV, false).toBool());
    setPythonEdition(qvariant_cast<Qt::CheckState>(m_config->value(SETTING_PYTHON_EDITION, Qt::PartiallyChecked)));
    setNormalOs(m_config->value(SETTING_DEBUGGER_NORM_OS, true).toBool());
    setDebugLcdDma(!m_config->value(SETTING_DEBUGGER_IGNORE_DMA, true).toBool());
    setFocusSetting(m_config->value(SETTING_PAUSE_FOCUS, false).toBool());
    setRecentSave(m_config->value(SETTING_RECENT_SAVE, true).toBool());
    setDockGroupDrag(m_config->value(SETTING_WINDOW_GROUP_DRAG, false).toBool());
    setMenuBarState(m_config->value(SETTING_WINDOW_MENUBAR, false).toBool());
    setStatusBarState(m_config->value(SETTING_WINDOW_STATUSBAR, false).toBool());
    setTop(m_config->value(SETTING_ALWAYS_ON_TOP, false).toBool());

    if (m_config->value(SETTING_NATIVE_CONSOLE, false).toBool()) {
        ui->radioConsole->setChecked(true);
        consoleModified();
    }

    // server name
    console(QStringLiteral("[CEmu] Initialized Server [") + opts.idString +
            QStringLiteral(" | ") + com.getServerName() + QStringLiteral("]\n"));

    m_dir.setPath(m_config->value(SETTING_CURRENT_DIR, appDir().path()).toString());

    if (!m_config->contains(SETTING_IMAGE_PATH) || m_portable) {
        QString path = QFileInfo(m_pathConfig).absolutePath() + SETTING_DEFAULT_IMAGE_FILE;
        if (m_portable) {
            path = appDir().relativeFilePath(path);
        }
        m_config->setValue(SETTING_IMAGE_PATH, QDir::cleanPath(path));
    }
    ui->pathImage->setText(m_config->value(SETTING_IMAGE_PATH).toString());

    m_pathImage = QDir::cleanPath(appDir().absoluteFilePath(ui->pathImage->text()));

    if (!m_config->contains(SETTING_DEBUGGER_IMAGE_PATH) || m_portable) {
        QString path = QFileInfo(m_pathConfig).absolutePath() + SETTING_DEFAULT_DEBUG_FILE;
        if (m_portable) {
            path = appDir().relativeFilePath(path);
        }
        m_config->setValue(SETTING_DEBUGGER_IMAGE_PATH, QDir::cleanPath(path));
    }
    ui->pathDebug->setText(m_config->value(SETTING_DEBUGGER_IMAGE_PATH).toString());

    keymapLoad();
    debugInit();
    debugBasicInit();

    if (!fileExists(m_pathRom)) {
        if (!runSetup()) {
            m_initPassed = false;
        }
    } else {
        if (opts.useSettings && opts.restoreOnOpen && m_config->value(SETTING_RESTORE_ON_OPEN, true).toBool()) {
            emuLoad(opts.forceReloadRom ? EMU_DATA_ROM : EMU_DATA_IMAGE);
        } else {
            emuLoad(EMU_DATA_ROM);
        }
    }

    ui->pathRom->setText(m_portable ? appDir().relativeFilePath(m_pathRom) : m_pathRom);

    if (opts.useSettings) {
        ui->pathConfig->setText(m_pathConfig);
    }

    if (m_portable) {
        ui->checkPortable->blockSignals(true);
        ui->checkPortable->setChecked(true);
        ui->buttonChangeSavedDebugPath->setEnabled(false);
        ui->buttonChangeSavedImagePath->setEnabled(false);
        ui->checkPortable->blockSignals(false);
    }
}

void MainWindow::translateSwitch(const QString& lang) {
    qApp->removeTranslator(&m_appTranslator);
    // For English, nothing to load after removing the translator.
    if (lang == QStringLiteral("en_EN") || (m_appTranslator.load(lang, QStringLiteral(":/i18n/i18n/"))
                                            && qApp->installTranslator(&m_appTranslator))) {
        m_config->setValue(SETTING_PREFERRED_LANG, lang);
    } else {
        QMessageBox::warning(this, MSG_WARNING, tr("No translation available for this language :("));
    }
}

void MainWindow::translateExtras(int init) {
    QAction *action;

    TITLE_DOCKS = tr("Docks");
    TITLE_DEBUG = tr("Debug");
    MSG_INFORMATION = tr("Information");
    MSG_WARNING = tr("Warning");
    MSG_ERROR = tr("Error");
    MSG_ADD_MEMORY = tr("Add memory view");
    MSG_ADD_VISUALIZER = tr("Add memory visualizer");
    MSG_EDIT_UI = tr("Enable UI edit mode");
    ACTION_TOGGLE_BREAK = tr("Toggle Breakpoint");
    ACTION_TOGGLE_READ = tr("Toggle Read Watchpoint");
    ACTION_TOGGLE_WRITE = tr("Toggle Write Watchpoint");
    ACTION_TOGGLE_RW = tr("Toggle Read/Write Watchpoint");
    ACTION_GOTO_MEMORY_VIEW = tr("Goto Memory View");
    ACTION_GOTO_VAT_MEMORY_VIEW = tr("Goto VAT Memory View");
    ACTION_GOTO_DISASM_VIEW = tr("Goto Disasm View");
    ACTION_COPY_ADDR = tr("Copy Address");
    ACTION_COPY_DATA = tr("Copy Data");
    ACTION_RUN_UNTIL = tr("Run Until");

    QString __TXT_MEM_DOCK = tr("Memory View");
    QString __TXT_VISUALIZER_DOCK = tr("Memory Visualizer");
    QString __TXT_KEYHISTORY_DOCK = tr("Keypress History");

    QString __TXT_CLEAR_HISTORY = tr("Clear History");
    QString __TXT_SIZE = tr("Size");

    QString __TXT_GOTO = tr("Goto");
    QString __TXT_SEARCH = tr("Search");
    QString __TXT_SYNC = tr("Sync Changes");
    QString __TXT_ASCII = tr("Show ASCII");

    QString __TXT_CONSOLE = tr("Console");
    QString __TXT_SETTINGS = tr("Settings");
    QString __TXT_VARIABLES = tr("Variables");
    QString __TXT_CAPTURE = tr("Capture");
    QString __TXT_STATE = tr("State");
    QString __TXT_KEYPAD = tr("Keypad");

    QString __TXT_TI_BASIC_DEBUG = tr("TI-Basic Debug");
    QString __TXT_DEBUG_CONTROL = tr("Debug Control");
    QString __TXT_CPU_STATUS = tr("CPU Status");
    QString __TXT_DISASSEMBLY = tr("Disassembly");
    QString __TXT_MEMORY = tr("Memory");
    QString __TXT_TIMERS = tr("Timers");
    QString __TXT_BREAKPOINTS = tr("Breakpoints");
    QString __TXT_WATCHPOINTS = tr("Watchpoints");
    QString __TXT_PORTMON = tr("Port Monitor");
    QString __TXT_OS_VIEW = tr("OS Variables");
    QString __TXT_OS_STACKS = tr("OS Stacks");
    QString __TXT_MISC = tr("Miscellaneous");
    QString __TXT_AUTOTESTER = tr("AutoTester");

    setWindowTitle(QStringLiteral("CEmu | ") + opts.idString);

    if (init == TRANSLATE_UPDATE) {
        for (const auto &dock : findChildren<DockWidget*>()) {
            if (dock->windowTitle() == TXT_MEM_DOCK) {
                QList<QPushButton*> buttons = dock->findChildren<QPushButton*>();
                QList<QToolButton*> tools = dock->findChildren<QToolButton*>();
                dock->setWindowTitle(__TXT_MEM_DOCK);
                buttons.at(0)->setText(__TXT_GOTO);
                buttons.at(1)->setText(__TXT_SEARCH);
                tools.at(0)->setToolTip(__TXT_ASCII);
                tools.at(1)->setToolTip(__TXT_SYNC);
            }
            if (dock->windowTitle() == TXT_VISUALIZER_DOCK) {
                dock->setWindowTitle(__TXT_VISUALIZER_DOCK);
                static_cast<VisualizerWidget*>(dock->widget())->translate();
            }
            if (dock->windowTitle() == TXT_KEYHISTORY_DOCK) {
                QList<QPushButton*> buttons = dock->findChildren<QPushButton*>();
                QList<QLabel*> labels = dock->findChildren<QLabel*>();
                dock->setWindowTitle(__TXT_KEYHISTORY_DOCK);
                buttons.at(0)->setText(__TXT_CLEAR_HISTORY);
                labels.at(0)->setText(__TXT_SIZE);
            }
            if (dock->windowTitle() == TXT_CONSOLE) {
                dock->setWindowTitle(__TXT_CONSOLE);
            }
            if (dock->windowTitle() == TXT_SETTINGS) {
                dock->setWindowTitle(__TXT_SETTINGS);
            }
            if (dock->windowTitle() == TXT_VARIABLES) {
                dock->setWindowTitle(__TXT_VARIABLES);
            }
            if (dock->windowTitle() == TXT_CAPTURE) {
                dock->setWindowTitle(__TXT_CAPTURE);
            }
            if (dock->windowTitle() == TXT_STATE) {
                dock->setWindowTitle(__TXT_STATE);
            }
            if (dock->windowTitle() == TXT_KEYPAD) {
                dock->setWindowTitle(__TXT_KEYPAD);
            }
            if (dock->windowTitle() == TXT_TI_BASIC_DEBUG) {
                dock->setWindowTitle(__TXT_TI_BASIC_DEBUG);
            }
            if (dock->windowTitle() == TXT_DEBUG_CONTROL) {
                dock->setWindowTitle(__TXT_DEBUG_CONTROL);
            }
            if (dock->windowTitle() == TXT_CPU_STATUS) {
                dock->setWindowTitle(__TXT_CPU_STATUS);
            }
            if (dock->windowTitle() == TXT_DISASSEMBLY) {
                dock->setWindowTitle(__TXT_DISASSEMBLY);
            }
            if (dock->windowTitle() == TXT_MEMORY) {
                dock->setWindowTitle(__TXT_MEMORY);
            }
            if (dock->windowTitle() == TXT_TIMERS) {
                dock->setWindowTitle(__TXT_TIMERS);
            }
            if (dock->windowTitle() == TXT_BREAKPOINTS) {
                dock->setWindowTitle(__TXT_BREAKPOINTS);
            }
            if (dock->windowTitle() == TXT_WATCHPOINTS) {
                dock->setWindowTitle(__TXT_WATCHPOINTS);
            }
            if (dock->windowTitle() == TXT_PORTMON) {
                dock->setWindowTitle(__TXT_PORTMON);
            }
            if (dock->windowTitle() == TXT_OS_VIEW) {
                dock->setWindowTitle(__TXT_OS_VIEW);
            }
            if (dock->windowTitle() == TXT_OS_STACKS) {
                dock->setWindowTitle(__TXT_OS_STACKS);
            }
            if (dock->windowTitle() == TXT_MISC) {
                dock->setWindowTitle(__TXT_MISC);
            }
            if (dock->windowTitle() == TXT_AUTOTESTER) {
                dock->setWindowTitle(__TXT_AUTOTESTER);
            }
        }

        m_varTableModel->retranslate();
    }
    varUpdate();

    TXT_MEM_DOCK = __TXT_MEM_DOCK;
    TXT_VISUALIZER_DOCK = __TXT_VISUALIZER_DOCK;
    TXT_KEYHISTORY_DOCK = __TXT_KEYHISTORY_DOCK;

    TXT_CLEAR_HISTORY = __TXT_CLEAR_HISTORY;
    TXT_SIZE = __TXT_SIZE;

    TXT_CONSOLE = __TXT_CONSOLE;
    TXT_SETTINGS = __TXT_SETTINGS;
    TXT_VARIABLES = __TXT_VARIABLES;
    TXT_CAPTURE = __TXT_CAPTURE;
    TXT_STATE = __TXT_STATE;
    TXT_KEYPAD = __TXT_KEYPAD;

    TXT_TI_BASIC_DEBUG = __TXT_TI_BASIC_DEBUG;
    TXT_DEBUG_CONTROL = __TXT_DEBUG_CONTROL;
    TXT_CPU_STATUS = __TXT_CPU_STATUS;
    TXT_DISASSEMBLY = __TXT_DISASSEMBLY;
    TXT_MEMORY = __TXT_MEMORY;
    TXT_TIMERS = __TXT_TIMERS;
    TXT_BREAKPOINTS = __TXT_BREAKPOINTS;
    TXT_WATCHPOINTS = __TXT_WATCHPOINTS;
    TXT_PORTMON = __TXT_PORTMON;
    TXT_OS_VIEW = __TXT_OS_VIEW;
    TXT_OS_STACKS = __TXT_OS_STACKS;
    TXT_MISC = __TXT_MISC;
    TXT_AUTOTESTER = __TXT_AUTOTESTER;

#ifdef _WIN32
    TXT_TOGGLE_CONSOLE = tr("Toggle Windows Console");
#endif

    if (init == TRANSLATE_UPDATE) {
        m_actionToggleUI->setText(MSG_EDIT_UI);
        m_actionAddMemory->setText(MSG_ADD_MEMORY);
        m_actionAddVisualizer->setText(MSG_ADD_VISUALIZER);
        m_menuDebug->setTitle(TITLE_DEBUG);
        m_menuDocks->setTitle(TITLE_DOCKS);

        action = m_menuDocks->actions().at(0);
        action->setText(TXT_VARIABLES);
        action = m_menuDocks->actions().at(1);
        action->setText(TXT_CAPTURE);
        action = m_menuDocks->actions().at(2);
        action->setText(TXT_SETTINGS);
        action = m_menuDocks->actions().at(3);
        action->setText(TXT_CONSOLE);
        action = m_menuDocks->actions().at(4);
        action->setText(TXT_STATE);
        action = m_menuDocks->actions().at(5);
        action->setText(TXT_KEYPAD);

        action = m_menuDebug->actions().at(0);
        action->setText(TXT_TI_BASIC_DEBUG);
        action = m_menuDebug->actions().at(1);
        action->setText(TXT_DEBUG_CONTROL);
        action = m_menuDebug->actions().at(2);
        action->setText(TXT_CPU_STATUS);
        action = m_menuDebug->actions().at(3);
        action->setText(TXT_DISASSEMBLY);
        action = m_menuDebug->actions().at(4);
        action->setText(TXT_MEMORY);
        action = m_menuDebug->actions().at(5);
        action->setText(TXT_TIMERS);
        action = m_menuDebug->actions().at(6);
        action->setText(TXT_BREAKPOINTS);
        action = m_menuDebug->actions().at(7);
        action->setText(TXT_WATCHPOINTS);
        action = m_menuDebug->actions().at(8);
        action->setText(TXT_PORTMON);
        action = m_menuDebug->actions().at(9);
        action->setText(TXT_OS_VIEW);
        action = m_menuDebug->actions().at(10);
        action->setText(TXT_OS_STACKS);
        action = m_menuDebug->actions().at(11);
        action->setText(TXT_MISC);
        action = m_menuDebug->actions().at(12);
        action->setText(TXT_AUTOTESTER);

#ifdef _WIN32
        actionToggleConsole->setText(TXT_TOGGLE_CONSOLE);
#endif
    }
}

void MainWindow::darkModeSwitch(bool darkMode) {
    QApplication::setStyle(m_styleForMode[darkMode]);
    if (darkMode != isRunningInDarkMode()) {
        m_styleForMode[darkMode] = QStringLiteral("fusion");
        QApplication::setStyle(m_styleForMode[darkMode]);
        darkMode = isRunningInDarkMode();
    }

    m_isInDarkMode = darkMode;
    if (darkMode) {
        m_cBack.setColor(QPalette::Base, QColor(Qt::blue).lighter(180));
        m_cBack.setColor(QPalette::Text, Qt::black);
    } else {
        m_cBack.setColor(QPalette::Base, QColor(Qt::yellow).lighter(160));
        m_cBack.setColor(QPalette::Text, Qt::black);
    }
    ui->disasm->updateDarkMode();
    debugBasicUpdateDarkMode();
}

void MainWindow::changeEvent(QEvent* event) {
    const auto eventType = event->type();
    if (eventType == QEvent::LanguageChange) {
        if (m_setup) {
            ui->retranslateUi(this);
            translateExtras(TRANSLATE_UPDATE);
        }
    } else if (eventType == QEvent::LocaleChange) {
        translateSwitch(QLocale::system().name());
    }
    QMainWindow::changeEvent(event);
    if (eventType == QEvent::ThemeChange) {
        bool darkMode = isSystemInDarkMode();
        if (darkMode != m_isInDarkMode) {
            darkModeSwitch(darkMode);
        }
    }
}

void MainWindow::showEvent(QShowEvent *e) {
    if (!m_setup) {
        e->ignore();
        return;
    }
    QMainWindow::showEvent(e);
    e->accept();
}

DockWidget *MainWindow::redistributeFindDock(const QPoint &pos) {
    QWidget *child = childAt(pos);
    if (QTabBar *tabBar = findSelfOrParent<QTabBar *>(child)) {
        child = childAt({pos.x(), tabBar->mapTo(this, QPoint{}).y() - 1});
    }
    return findSelfOrParent<DockWidget *>(child);
}

bool MainWindow::redistributeDocks(const QPoint &pos, const QPoint &offset,
                                   Qt::CursorShape cursorShape,
                                   int (QSize::*dimension)() const,
                                   Qt::Orientation orientation) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    if (cursor().shape() == cursorShape) {
        if (DockWidget *before = redistributeFindDock(pos - offset)) {
            if (DockWidget *after = redistributeFindDock(pos + offset)) {
                if (before != after) {
                    int size = (before->size().*dimension)() + (after->size().*dimension)();
                    resizeDocks({before, after}, {size / 2, size - size / 2}, orientation);
                    return true;
                }
            }
        }
    }
#else
    (void)pos; (void)offset; (void)cursorShape; (void)dimension; (void)orientation;
#endif
    return false;
}

void MainWindow::raiseContainingDock(QWidget *widget) {
    QWidget *dock = findSelfOrParent<QDockWidget*>(widget);
    if (dock != Q_NULLPTR) {
        if (m_uiEditMode) {
            dock->show();
        }
        dock->activateWindow();
        dock->raise();
    }
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event) {
    if (!childAt(event->pos())) {
        int sep = style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent, Q_NULLPTR, this);
        if (redistributeDocks(event->pos(), {sep, 0}, Qt::SplitHCursor, &QSize::width, Qt::Horizontal) ||
            redistributeDocks(event->pos(), {0, sep}, Qt::SplitVCursor, &QSize::height, Qt::Vertical)) {
            event->accept();
            return;
        }
    }
    QMainWindow::mouseDoubleClickEvent(event);
}

void MainWindow::setup() {
    if (!m_initPassed) {
        QFile(m_pathConfig).remove();
        close();
        return;
    }

    m_uiEditMode = m_config->value(SETTING_UI_EDIT_MODE, true).toBool();


    setUIDocks();
    show();

    const QByteArray geometry = m_config->value(SETTING_WINDOW_GEOMETRY, QByteArray()).toByteArray();
    if (restoreState(m_config->value(SETTING_WINDOW_STATE).toByteArray()) && restoreGeometry(geometry) && restoreGeometry(geometry)) {
        const QPoint position = m_config->value(SETTING_WINDOW_POSITION).toPoint();
        QTimer::singleShot(0, [this, position]() { move(position); });
    } else {
        foreach (DockWidget *dw, m_dockPtrs) {
            dw->setVisible(true);
        }
        resize(minimumWidth(), minimumHeight());
        setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->screens().first()->availableGeometry()));
    }

    // restore the fullscreen modes as needed
    if (opts.fullscreen == -1) {
        setFullscreen(m_config->value(SETTING_WINDOW_FULLSCREEN, 0).toInt());
    } else {
        setFullscreen(opts.fullscreen);
    }

    setUIEditMode(m_uiEditMode);

    stateLoadInfo();
    recentLoadInfo();

    if (m_config->value(SETTING_DEBUGGER_RESTORE_ON_OPEN).toBool()) {
        if (!opts.debugFile.isEmpty()) {
            debugImportFile(opts.debugFile);
        } else {
            debugImportFile(m_config->value(SETTING_DEBUGGER_IMAGE_PATH).toString());
        }
    }

    if (opts.useSettings && isFirstRun() && m_initPassed && !m_needFullReset) {
        QMessageBox *info = new QMessageBox();
        m_config->setValue(SETTING_FIRST_RUN, true);
        info->setWindowTitle(MSG_INFORMATION);
        info->setText(tr("Welcome!\n\nCEmu uses a customizable dock-style interface. "
                            "Drag and drop to move tabs and windows around on the screen, "
                            "and choose which docks are available in the 'Docks' menu in the topmost bar. "
                            "Be sure that 'Enable UI edit mode' is selected when laying out your interface. "
                            "Enjoy!"
                             "\n\n(Notice: depending on your version, you can drag grouped tabs or an individual tab from their title or tab bar, respectively)"
                         ));
        info->setWindowModality(Qt::NonModal);
        info->setWindowFlags(info->windowFlags() | Qt::WindowStaysOnTopHint);
        info->setAttribute(Qt::WA_DeleteOnClose);
        info->show();
    }

    QString prefLang = m_config->value(SETTING_PREFERRED_LANG, QStringLiteral("none")).toString();
    if (prefLang != QStringLiteral("none")) {
        translateSwitch(prefLang);
    }

    translateExtras(TRANSLATE_UPDATE);

    optSend(opts);
    if (opts.speed != -1) {
        setEmuSpeed(opts.speed);
    }

    if (m_portableActivated) {
        QMessageBox *info = new QMessageBox();
        info->setWindowTitle(MSG_INFORMATION);
        info->setText(tr("CEmu was not able to write to the standard settings location.\n"
                         "Portable mode has been activated."));
        info->setWindowModality(Qt::NonModal);
        info->setWindowFlags(info->windowFlags() | Qt::WindowStaysOnTopHint);
        info->setAttribute(Qt::WA_DeleteOnClose);
        info->show();
    }

    setUIDockEditMode(m_uiEditMode);
    ui->lcd->setFocus();
    m_setup = true;
}

void MainWindow::sendEmuKey(uint16_t key) {
    emu.enqueueKeys(key);
}

void MainWindow::sendEmuLetterKey(char letter) {
    if (letter >= '0' && letter <= '9') {
        sendEmuKey(0x8E + letter - '0');
    } else if (letter >= 'A' && letter <= 'Z') {
        sendEmuKey(0x9A + letter - 'A');
    } else if (letter == 'Z' + 1 || letter == '@') { /* [ or @ for theta (caller should replace it) */
        sendEmuKey(0xCC);
    }
}

void MainWindow::optSend(CEmuOpts &o) {
    int speed = m_config->value(SETTING_EMUSPEED).toInt();
    if (!o.autotesterFile.isEmpty()) {
        if (!autotesterOpen(o.autotesterFile)) {
           if (!o.deforceReset) {
               resetEmu();
           }
           setEmuSpeed(100);

           // Race condition requires this
           guiDelay(o.deforceReset ? 100 : 4000);
           autotesterLaunch();
        }
    }

    if (!o.sendFiles.isEmpty() || !o.sendArchFiles.isEmpty() || !o.sendRAMFiles.isEmpty()) {
        if (!o.deforceReset) {
            resetEmu();
        }
        setEmuSpeed(100);

        // Race condition requires this
        guiDelay(o.deforceReset ? 100 : 4000);
        if (!o.sendFiles.isEmpty()) {
            sendingHandler->sendFiles(o.sendFiles, LINK_FILE);
        }
        if (!o.sendArchFiles.isEmpty()) {
            sendingHandler->sendFiles(o.sendArchFiles, LINK_ARCH);
        }
        if (!o.sendRAMFiles.isEmpty()) {
            sendingHandler->sendFiles(o.sendRAMFiles, LINK_RAM);
        }
    }

    initLuaThings(repl_lua, true);

    setThrottle(o.useUnthrottled ? Qt::Unchecked : Qt::Checked);
    ui->lcdWidget->setFocus();
    setEmuSpeed(speed);

    if (!o.launchPrgm.isEmpty()) {
        guiDelay(50);
        sendEmuKey(CE_KEY_CLEAR);
        sendEmuKey(CE_KEY_PRGM);
        for (const QChar &c : o.launchPrgm) {
            sendEmuLetterKey(c.toLatin1());
        }
        sendEmuKey(CE_KEY_ENTER);
    }
}

void MainWindow::optLoadFiles(CEmuOpts &o) {
    if (o.romFile.isEmpty()) {
        if (m_loadedBootImage) {
            m_pathRom = configPath + SETTING_DEFAULT_ROM_FILE;
            m_config->setValue(SETTING_ROM_PATH, m_pathRom);
        } else {
            const QString path = m_config->value(SETTING_ROM_PATH, QString()).toString();
            if (path.isEmpty()) {
                m_pathRom.clear();
            } else {
                m_pathRom = QDir::cleanPath(appDir().absoluteFilePath(path));
            }
        }
    } else {
        m_pathRom = QDir::cleanPath(o.romFile);
        if (!m_config->contains(SETTING_ROM_PATH)) {
            m_config->setValue(SETTING_ROM_PATH, m_pathRom);
        }
    }

    if (!o.imageFile.isEmpty()) {
        if (fileExists(o.imageFile)) {
            m_pathImage = QDir::cleanPath(QFileInfo(o.imageFile).absoluteFilePath());
        }
    }
}

void MainWindow::optAttemptLoad(CEmuOpts &o) {
    if (!fileExists(m_pathRom)) {
        if (!runSetup()) {
            close();
        }
    } else {
        if (o.restoreOnOpen && !o.imageFile.isEmpty() && fileExists(m_pathImage)) {
            emuLoad(EMU_DATA_IMAGE);
        } else {
            if (o.forceReloadRom) {
                emuLoad(EMU_DATA_ROM);
                guiDelay(500);
            }
        }
    }
}

MainWindow::~MainWindow() {
    delete m_config;
    delete ui;
}

bool MainWindow::isInitialized() {
    return m_initPassed;
}

void MainWindow::resetCEmu() {
    ipcCloseConnected();
    m_needReload = true;
    m_needFullReset = true;
    close();
}

void MainWindow::resetGui() {
    ipcCloseConnected();
    m_config->remove(SETTING_SCREEN_SKIN);
    m_config->remove(SETTING_SCREEN_SCALE);
    m_config->remove(SETTING_SCREEN_UPSCALE);
    m_config->remove(SETTING_SCREEN_FULLSCREEN);
    m_config->remove(SETTING_WINDOW_FULLSCREEN);
    m_config->remove(SETTING_WINDOW_GEOMETRY);
    m_config->remove(SETTING_WINDOW_POSITION);
    m_config->remove(SETTING_WINDOW_MEMORY_DOCKS);
    m_config->remove(SETTING_WINDOW_MEMORY_DOCK_ASCII);
    m_config->remove(SETTING_WINDOW_MEMORY_DOCK_BYTES);
    m_config->remove(SETTING_WINDOW_VISUALIZER_DOCKS);
    m_config->remove(SETTING_WINDOW_VISUALIZER_CONFIG);
    m_config->remove(SETTING_WINDOW_STATE);
    m_config->remove(SETTING_WINDOW_MENUBAR);
    m_config->remove(SETTING_WINDOW_STATUSBAR);
    m_config->remove(SETTING_WINDOW_SEPARATOR);
    m_config->remove(SETTING_UI_EDIT_MODE);
    m_needReload = true;
    close();
}

bool MainWindow::isReload() {
    return m_needReload;
}

bool MainWindow::isResetAll() {
    if (m_needFullReset) {
        QFile(m_config->value(SETTING_IMAGE_PATH).toString()).remove();
        QFile(m_config->value(SETTING_DEBUGGER_IMAGE_PATH).toString()).remove();
        if (m_keepSetup) {
            m_config->remove(SETTING_WINDOW_FULLSCREEN);
            m_config->remove(SETTING_IMAGE_PATH);
            m_config->remove(SETTING_DEBUGGER_IMAGE_PATH);
            m_config->remove(SETTING_SCREEN_SKIN);
            m_config->remove(SETTING_WINDOW_POSITION);
            m_config->remove(SETTING_WINDOW_GEOMETRY);
            m_config->remove(SETTING_WINDOW_STATE);
            m_config->remove(SETTING_WINDOW_MEMORY_DOCKS);
            m_config->remove(SETTING_UI_EDIT_MODE);
            m_config->remove(SETTING_WINDOW_MENUBAR);
            m_config->remove(SETTING_WINDOW_STATUSBAR);
            m_config->remove(SETTING_WINDOW_SEPARATOR);
            m_config->sync();
        } else {
            QFile(m_pathConfig).remove();
        }
    }
    return m_needFullReset;
}

void MainWindow::stateToPath(const QString &path) {
    emu.save(EMU_DATA_IMAGE, appDir().absoluteFilePath(path));
}

void MainWindow::stateFromPath(const QString &path) {
    QString prev = m_pathImage;
    m_pathImage = appDir().absoluteFilePath(path);
    emuLoad(EMU_DATA_IMAGE);
    m_pathImage = prev;
}

void MainWindow::stateFromFile() {
    QString path = QFileDialog::getOpenFileName(this, tr("Select saved image to restore from"),
                                                      m_dir.absolutePath(),
                                                      tr("CEmu images (*.ce);;All files (*.*)"));
    if (!path.isEmpty()) {
        m_dir = QFileInfo(path).absoluteDir();
        stateFromPath(path);
    }
}

void MainWindow::stateToFile() {
    QString path = QFileDialog::getSaveFileName(this, tr("Set image to save to"),
                                                      m_dir.absolutePath(),
                                                      tr("CEmu images (*.ce);;All files (*.*)"));
    if (!path.isEmpty()) {
        m_dir = QFileInfo(path).absoluteDir();
        stateToPath(path);
    }
}

void MainWindow::romExport() {
    QString filter = tr("ROM images (*.rom)");
    QString path = QFileDialog::getSaveFileName(this, tr("Set ROM image to save to"),
                                                m_dir.absolutePath(),
                                                filter, &filter);
    if (!path.isEmpty()) {
        m_dir = QFileInfo(path).absoluteDir();
        emu.save(EMU_DATA_ROM, path);
    }
}

void MainWindow::ramExport() {
    QString filter = tr("RAM images (*.ram)");
    QString path = QFileDialog::getSaveFileName(this, tr("Set RAM image to save to"),
                                                m_dir.absolutePath(),
                                                filter, &filter);
    if (!path.isEmpty()) {
        m_dir = QFileInfo(path).absoluteDir();
        emu.save(EMU_DATA_RAM, path);
    }
}

void MainWindow::ramImport() {
    QString path = QFileDialog::getOpenFileName(this, tr("Select RAM image to load"),
                                                      m_dir.absolutePath(),
                                                      tr("RAM images (*.ram);;All files (*.*)"));
    if (!path.isEmpty()) {
        m_dir = QFileInfo(path).absoluteDir();
        m_pathRam = path;
        emuLoad(EMU_DATA_RAM);
    }
}

void MainWindow::dropEvent(QDropEvent *e) {
    if (m_isSendingRom) {
        setRom(m_dragRom);
    } else {
        sendingHandler->dropOccured(e, LINK_FILE);
    }
    equatesRefresh();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e) {

    // check if we are dragging a rom file
    m_dragRom = sendingROM(e, &m_isSendingRom);

    if (!m_isSendingRom) {
        sendingHandler->dragOccured(e);
    }
}

void MainWindow::emuSaved(bool success) {
    if (!success) {
        QMessageBox::warning(this, MSG_WARNING, tr("Saving failed. Please check write permissions in settings directory."));
    }

    if (m_shutdown) {
        close();
    }
}

void MainWindow::closeEvent(QCloseEvent *e) {
    guiEmuValid = false;

    if (!m_shutdown) {
        m_shutdown = true;

        com.idClose();

        if (!m_initPassed) {
            QMainWindow::closeEvent(e);
            return;
        }

        if (guiDebugBasic) {
            debugBasic(false);
        }

        if (guiDebug) {
            debugToggle();
        }

        if (guiReceive) {
            varToggle();
        }

        if (!m_needReload) {
            saveSettings();

            if (m_config->value(SETTING_SAVE_ON_CLOSE).toBool()) {
                stateToPath(m_pathImage);
                e->ignore();
                return;
            }
        }
    }

    emu.stop();
    QMainWindow::closeEvent(e);
}

void MainWindow::console(const QString &str, int type) {
    if (m_nativeConsole) {
        fputs(str.toStdString().c_str(), type == EmuThread::ConsoleErr ? stderr : stdout);
    } else {
        QTextCharFormat format = ui->console->currentCharFormat();
        if (type == EmuThread::ConsoleNorm) {
            format.setFontWeight(QFont::Normal);
        } else {
            format.setFontWeight(QFont::Black);
        }
        QTextCursor cur(ui->console->document());
        cur.movePosition(QTextCursor::End);
        cur.insertText(str, format);
        if (ui->checkAutoScroll->isChecked()) {
            ui->console->setTextCursor(cur);
        }
    }
}

void MainWindow::console(int type, const char *str, int size) {
    if (size == -1) {
        size = static_cast<int>(strlen(str));
    }
    if (m_nativeConsole) {
        fwrite(str, sizeof(char), static_cast<size_t>(size), type == EmuThread::ConsoleErr ? stderr : stdout);
    } else {
        static int state = CONSOLE_ESC;
        const char *tok;
        const QColor lookup[8] = { Qt::black, Qt::red, Qt::green, Qt::yellow, Qt::blue, Qt::magenta, Qt::cyan, Qt::white };

        if ((tok = static_cast<const char*>(memchr(str, '\x1B', static_cast<size_t>(size))))) {
            QTextCharFormat format = ui->console->currentCharFormat();
            if (tok != str) {
                console(QString::fromUtf8(str, static_cast<int>(tok - str)), type);
                size -= tok - str;
            }
            do {
                while(--size > 0) {
                    char x = *tok++;
                    switch (state) {
                        case CONSOLE_ESC:
                            if (x == '\x1B') {
                                state = CONSOLE_BRACKET;
                            }
                            break;
                        case CONSOLE_BRACKET:
                            if (x == '[') {
                                state = CONSOLE_PARSE;
                            } else {
                                state = CONSOLE_ESC;
                            }
                            break;
                        case CONSOLE_PARSE:
                            switch (x) {
                                case '0':
                                    state = CONSOLE_ENDVAL;
                                    format.clearBackground();
                                    format.clearForeground();
                                    break;
                                case '3':
                                    state = CONSOLE_FGCOLOR;
                                    break;
                                case '4':
                                    state = CONSOLE_BGCOLOR;
                                    break;
                                default:
                                    state = CONSOLE_ESC;
                                    format.clearBackground();
                                    format.clearForeground();
                                    ui->console->setCurrentCharFormat(format);
                                    break;
                            }
                            break;
                        case CONSOLE_FGCOLOR:
                            if (x >= '0' && x <= '7') {
                                state = CONSOLE_ENDVAL;
                                format.setForeground(lookup[x - '0']);
                            } else {
                                state = CONSOLE_ESC;
                            }
                            break;
                        case CONSOLE_BGCOLOR:
                            if (x >= '0' && x <= '7') {
                                state = CONSOLE_ENDVAL;
                                format.setBackground(lookup[x - '0']);
                            } else {
                                state = CONSOLE_ESC;
                            }
                            break;
                        case CONSOLE_ENDVAL:
                            if (x == ';') {
                                state = CONSOLE_PARSE;
                            } else if (x == 'm') {
                                ui->console->setCurrentCharFormat(format);
                                state = CONSOLE_ESC;
                            } else {
                                state = CONSOLE_ESC;
                            }
                            break;
                        default:
                            state = CONSOLE_ESC;
                            break;
                    }
                    if (state == CONSOLE_ESC) {
                        break;
                    }
                }
                if (size > 0) {
                    const char *tokn = static_cast<const char*>(memchr(tok, '\x1B', static_cast<size_t>(size)));
                    if (tokn) {
                        console(QString::fromUtf8(tok, static_cast<int>(tokn - tok)), type);
                        size -= tokn - tok;
                    } else {
                        console(QString::fromUtf8(tok, static_cast<int>(size)), type);
                    }
                    tok = tokn;
                } else {
                    tok = nullptr;
                }
            } while (tok);
        } else {
            console(QString::fromUtf8(str, size), type);
        }
    }
}

void MainWindow::consoleClear() {
    if (m_nativeConsole) {
        int ret;
#ifdef _WIN32
        ret = system("cls");
#else
        ret = system("clear");
#endif
        if (ret == -1) {
            console(QStringLiteral("[CEmu] Error clearing console\n"), EmuThread::ConsoleErr);
        }
    } else {
        ui->console->clear();
    }
}

void MainWindow::consoleStr() {
    if (int available = emu.read.available()) {
        int remaining = CONSOLE_BUFFER_SIZE - emu.readPos;
        emu.read.acquire(available);

        console(emu.type, emu.buffer + emu.readPos, available < remaining ? available : remaining);
        if (available < remaining) {
            emu.readPos += available;
        } else if (available == remaining) {
            emu.readPos = 0;
        } else {
            emu.readPos = available - remaining;
            console(emu.type, emu.buffer, emu.readPos);
        }

        emu.write.release(available);
    }
}

void MainWindow::showAsicRevInfo(const QList<int>& supportedRevs, int loadedRev, int defaultRev, bool python) {
    QString defaultRevStr = ui->comboBoxAsicRev->itemText(defaultRev);
    ui->comboBoxAsicRev->setItemText(0, tr("Auto (%0)").arg(defaultRevStr));

    QString loadedRevStr = ui->comboBoxAsicRev->itemText(loadedRev);
    if (python) {
        loadedRevStr += " Python";
    }
    ui->currAsicRev->setText(tr("Current: %0 (change requires reset)").arg(loadedRevStr));

    m_supportedRevs = supportedRevs;
    setAsicValidRevisions();

    if (loadedRev != defaultRev || ui->comboBoxAsicRev->currentIndex() != 0) {
        ui->comboBoxAsicRev->setCurrentIndex(loadedRev);
    }

    setCalcSkinTopFromType(python);
}

void MainWindow::showEmuSpeed(double emuTime) {
    static double emuRunTime = 0;
    static uint32_t emuRunCount = 0;
    emuRunTime += emuTime;
    emuRunCount++;
    if (m_timerEmuTriggered && emuRunTime > 0) {
        unsigned int speed = (unsigned int)round(emuRunCount * 100.0 / emuRunTime);
        m_speedLabel.setText(QStringLiteral("  ") + tr("Emulated Speed: ") + QString::number(speed) + QStringLiteral("%"));
        emuRunTime = emuRunCount = 0;
        m_timerEmuTriggered = false;
    }
}

void MainWindow::timeoutEmuSpeed() {
    m_speedLabel.setText(QStringLiteral("  ") + tr("Emulated Speed: ") + tr("N/A"));
}

void MainWindow::showFpsSpeed(double emuFps, double guiFps) {
    static double guiFpsPrev = 0;
    static double emuFpsPrev = 0;
    if (emuFps && (emuFps < emuFpsPrev - 0.01 || emuFps > emuFpsPrev + 0.01)) {
        ui->maxFps->setText(tr("Actual FPS: ") + QString::number(emuFps, 'f', 2));
        emuFpsPrev = emuFps;
    }
    if (guiFps < guiFpsPrev - 0.01 || guiFps > guiFpsPrev + 0.01) {
        m_fpsLabel.setText("FPS: " + (guiFps ? QString::number(guiFps, 'f', 2) : tr("N/A")));
        guiFpsPrev = guiFps;
    }
}

void MainWindow::timeoutFpsSpeed() {
    showFpsSpeed(0.0, 0.0);
}

void MainWindow::lcdUpdate(double emuFps) {
    static double guiFrameTime = 0;
    static uint32_t guiFrameCount = 0;
    guiFrameTime += ui->lcd->refresh();
    guiFrameCount++;
    if (m_timerFpsTriggered && guiFrameTime > 0) {
        showFpsSpeed(emuFps, guiFrameCount / guiFrameTime);
        guiFrameTime = guiFrameCount = 0;
        m_timerFpsTriggered = false;
    }
}

void MainWindow::showStatusMsg(const QString &str) {
    m_msgLabel.setText(str);
}

void MainWindow::setRom(const QString &path) {
    m_pathRom = QDir::cleanPath(appDir().absoluteFilePath(path));
    m_config->setValue(SETTING_ROM_PATH, m_pathRom);
    ui->pathRom->setText(m_portable ? appDir().relativeFilePath(m_pathRom) : m_pathRom);
    emuLoad(EMU_DATA_ROM);
}

bool MainWindow::runSetup() {
    RomSelection *romWizard = new RomSelection();
    romWizard->setWindowModality(Qt::NonModal);
    romWizard->exec();

    const QString path = romWizard->getRomPath();

    delete romWizard;

    if (path.isEmpty()) {
        return false;
    } else {
        setRom(path);
    }

    return true;
}

void MainWindow::screenshotSave(const QString& nameFilter, const QString& defaultSuffix, const QString& temppath) {
    QFileDialog dialog(this);

    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setDirectory(m_dir);
    dialog.setNameFilter(nameFilter);
    dialog.setWindowTitle(tr("Save Screen"));
    dialog.setDefaultSuffix(defaultSuffix);
    dialog.exec();

    if (!(dialog.selectedFiles().isEmpty())) {
        QStringList selected = dialog.selectedFiles();
        QString filename = selected.first();
        if (filename.isEmpty()) {
            QFile::remove(temppath);
        } else {
            QFile::remove(filename);
            QFile::rename(temppath, filename);
        }
    } else {
        QFile::remove(temppath);
    }
    m_dir.setPath(dialog.directory().absolutePath());
}

void MainWindow::screenshot() {
    QString path = QDir::tempPath() + QDir::separator() + QStringLiteral("cemu_tmp.img");
    if (!ui->lcd->getImage().save(path, "PNG", 0)) {
        QMessageBox::critical(this, MSG_ERROR, tr("Failed to save screenshot."));
    }

    screenshotSave(tr("PNG images (*.png)"), QStringLiteral("png"), path);
}

void MainWindow::lcdCopy() {
    QApplication::clipboard()->setImage(ui->lcd->getImage(), QClipboard::Clipboard);
}

#ifdef PNG_WRITE_APNG_SUPPORTED
void MainWindow::recordAnimated() {
    static QString path;

    if (guiDebug || guiSend) {
        return;
    }

    if (path.isEmpty()) {
        if (m_recording) {
            return;
        }
        path = QDir::tempPath() + QDir::separator() + QStringLiteral("apng_tmp.png");
        apng_start(path.toStdString().c_str(), ui->apngSkip->value());
        showStatusMsg(tr("Recording..."));
    } else {
        showStatusMsg(tr("Saving Recording..."));
        if (apng_stop()) {
            int res;

            QFileDialog dialog(this);

            dialog.setAcceptMode(QFileDialog::AcceptSave);
            dialog.setFileMode(QFileDialog::AnyFile);
            dialog.setDirectory(m_dir);
            dialog.setNameFilter(tr("PNG images (*.png)"));
            dialog.setWindowTitle(tr("Save Recorded PNG"));
            dialog.setDefaultSuffix(QStringLiteral("png"));
            res = dialog.exec();

            QFile(path).remove();
            m_dir = dialog.directory();
            path.clear();

            if (res == QDialog::Accepted) {
                QStringList selected = dialog.selectedFiles();
                QString filename = selected.first();
                recordSave(filename);
            } else {
                recordControlUpdate();
            }
        } else {
            QMessageBox::critical(this, MSG_ERROR, tr("A failure occured during PNG recording."));
            m_msgLabel.clear();
            path.clear();
        }
        return;
    }

    m_recording = true;
    ui->apngSkip->setEnabled(false);
    ui->actionRecordAnimated->setChecked(true);
    ui->buttonRecordAnimated->setText(tr("Stop Recording"));
    ui->actionRecordAnimated->setText(tr("Stop Recording..."));
}

void MainWindow::recordSave(const QString &filename) {
    ui->apngSkip->setEnabled(false);
    ui->actionRecordAnimated->setEnabled(false);
    ui->buttonRecordAnimated->setEnabled(false);
    ui->buttonRecordAnimated->setText(tr("Saving..."));
    ui->actionRecordAnimated->setText(tr("Saving Animated PNG..."));

    RecordingThread *thread = new RecordingThread();
    connect(thread, &RecordingThread::done, this, &MainWindow::recordControlUpdate);
    connect(thread, &RecordingThread::finished, thread, &QObject::deleteLater);
    thread->m_filename = filename;
    thread->m_optimize = m_optimizeRecording;
    thread->start();
}

void MainWindow::recordControlUpdate() {
    m_recording = false;
    ui->apngSkip->setEnabled(true);
    ui->actionRecordAnimated->setEnabled(true);
    ui->buttonRecordAnimated->setEnabled(true);
    ui->actionRecordAnimated->setChecked(false);
    ui->buttonRecordAnimated->setText(tr("Record"));
    ui->actionRecordAnimated->setText(tr("Record animated PNG..."));
    m_msgLabel.clear();
}

void RecordingThread::run() {
    apng_save(m_filename.toStdString().c_str(), m_optimize);
    emit done();
}
#endif

void MainWindow::showAbout() {
    QMessageBox *aboutBox = new QMessageBox(this);

    aboutBox->setStyleSheet("QLabel{min-width: 620px;}");
    aboutBox->setWindowTitle(tr("About CEmu"));

    QAbstractButton *buttonUpdateCheck = aboutBox->addButton(tr("Check for updates"), QMessageBox::ActionRole);
    connect(buttonUpdateCheck, &QAbstractButton::clicked, this, [this](){ this->checkUpdate(true); });

    QAbstractButton *buttonCopyVersion = aboutBox->addButton(tr("Copy version"), QMessageBox::ActionRole);
    // Needed to prevent the button from closing the dialog
    buttonCopyVersion->disconnect();
    connect(buttonCopyVersion, &QAbstractButton::clicked, this, [this, buttonCopyVersion](){ QApplication::clipboard()->setText("CEmu " CEMU_VERSION " (git: " CEMU_GIT_SHA ")", QClipboard::Clipboard); buttonCopyVersion->setEnabled(false); buttonCopyVersion->setText(tr("Version copied!")); });

    QAbstractButton *okButton = aboutBox->addButton(QMessageBox::Ok);
    okButton->setFocus();

    aboutBox->setText(tr("%1<h3>CEmu %2</h3>"
                         "<a href='https://github.com/CE-Programming/CEmu'>On GitHub</a><br>"
                         "<br>Main authors:<br>%3"
                         "<br>Other contributors include:<br>%4"
                         "<br>Translations provided by:<br>%5"
                         "<br>Many thanks to the following projects: %6<br>In-program icons are courtesy of %7.<br>"
                         "<br>CEmu is licensed under the %8, and is not a TI product nor is it affiliated to/endorsed by TI.<br><br>")
                         .arg(QStringLiteral("<img src=':/icons/resources/icons/icon.png' align='right'>"),
                              QStringLiteral(CEMU_VERSION " <small><i>(git: " CEMU_GIT_SHA ")</i></small>"),
                              QStringLiteral("- Matt Waltz (<a href='https://github.com/mateoconlechuga'>MateoConLechuga</a>)<br>"
                                             "- Jacob Young (<a href='https://github.com/jacobly0'>jacobly0</a>)<br>"
                                             "- Brendan Fletcher (<a href='https://github.com/calc84maniac'>calc84maniac</a>)<br>"
                                             "- Adrien Bertrand (<a href='https://github.com/adriweb'>adriweb</a>)<br>"),
                              QStringLiteral("- Zachary Wassall (<a href='https://github.com/runer112'>Runer112</a>)<br>"
                                             "- Albert Huang (<a href='https://github.com/alberthdev'>alberthdev</a>)<br>"
                                             "- Lionel Debroux (<a href='https://github.com/debrouxl'>debrouxl</a>)<br>"
                                             "- Fabian Vogt (<a href='https://github.com/Vogtinator'>Vogtinator</a>)<br>"),
                              QStringLiteral("Matt Waltz (ES), Adrien Bertrand (FR), Stephan Paternotte &amp; Peter Tillema (NL), Jerry23011 (ZH)<br>"),
                              QStringLiteral("<a href='https://github.com/KnightOS/z80e'>z80e</a>, "
                                             "<a href='https://github.com/nspire-emus/firebird'>Firebird Emu</a>, "
                                             "<a href='https://github.com/debrouxl/tilibs'>tilibs</a>, "
                                             "<a href='https://github.com/adriweb/tivars_lib_cpp'>tivars_lib_cpp</a>."),
                              QStringLiteral("<a href='https://www.fatcow.com/free-icons'>FatCow's 'Farm-Fresh Web Icons'</a>"),
                              QStringLiteral("<a href='https://www.gnu.org/licenses/gpl-3.0.html'>GPLv3</a>")
                          ));

    aboutBox->setTextInteractionFlags(Qt::TextSelectableByMouse);
    aboutBox->setTextFormat(Qt::RichText);
    aboutBox->setWindowModality(Qt::NonModal);
    aboutBox->setAttribute(Qt::WA_DeleteOnClose);
    aboutBox->show();
}

void MainWindow::contextLcd(const QPoint &posa) {
    QMenu menu;
    QPoint globalPos = ui->lcd->mapToGlobal(posa);
    menu.addMenu(ui->menuFile);
    menu.addMenu(ui->menuCalculator);
    menu.addMenu(ui->menuCapture);
    menu.addMenu(m_menuDocks);
    menu.addMenu(ui->menuExtras);
    menu.exec(globalPos);
}

void MainWindow::consoleModified() {
    ui->console->clear();
    if (ui->radioConsole->isChecked()) {
        ui->console->setEnabled(false);
        console(tr("[CEmu] Dock output redirected to stdout. Use the radio button to enable dock."));
    } else {
        ui->console->setEnabled(true);
    }
    m_nativeConsole = ui->radioConsole->isChecked();
    m_config->setValue(SETTING_NATIVE_CONSOLE, m_nativeConsole);
}

void MainWindow::pauseEmu(Qt::ApplicationState state) {
    if (m_pauseOnFocus) {
        if (state == Qt::ApplicationInactive) {
            emu.setSpeed(0);
        }
        if (state == Qt::ApplicationActive) {
            setEmuSpeed(m_config->value(SETTING_EMUSPEED).toInt());
        }
    }
}

// ------------------------------------------------
//  Linking things
// ------------------------------------------------

void MainWindow::varReceive(std::function<void(bool)> recvAction) {
    if (guiDebug) {
        recvAction(false);
    } else {
        m_recvAction = std::move(recvAction);
        emu.receive();
    }
}

void MainWindow::varToggle() {
    guiReceive = !guiReceive;
    varUpdate();
    varShow();
}

void MainWindow::varSelect() {
    if (guiDebug) {
       return;
    }

    QStringList fileNames = varDialog(QFileDialog::AcceptOpen, tr("TI Variable (*.8xp *.8xv *.8xl *.8xn *.8xm *.8xy *.8xg *.8xs *.8xd *.8xw *.8xc *.8xl *.8xz *.8xt *.8ca *.8cg *.8ci *.8ek *.8eu *.8pu *.b84 *.b83);;All Files (*.*)"), Q_NULLPTR);

    sendingHandler->sendFiles(fileNames, LINK_FILE);
    equatesRefresh();
}

QStringList MainWindow::varDialog(QFileDialog::AcceptMode mode, const QString &name_filter, const QString &defaultSuffix) {
    QFileDialog dialog(this);
    int good;

    dialog.setDefaultSuffix(defaultSuffix);
    dialog.setAcceptMode(mode);
    dialog.setFileMode(mode == QFileDialog::AcceptOpen ? QFileDialog::ExistingFiles : QFileDialog::AnyFile);
    dialog.setDirectory(m_dir);
    dialog.setNameFilter(name_filter);
    good = dialog.exec();

    m_dir.setPath(dialog.directory().absolutePath());

    if (good) {
        return dialog.selectedFiles();
    }

    return QStringList();
}

void MainWindow::varPressed(const QModelIndex &index) {
    QModelIndex nameIndex = index.sibling(index.row(), VarTableModel::VAR_NAME_COL);
    if (!nameIndex.isValid()) {
        return;
    }
    calc_var_t var = nameIndex.data(Qt::UserRole).value<calc_var_t>();
    if (var.size <= 2 || calc_var_is_asmprog(&var)) {
        return;
    } else if (!calc_var_is_internal(&var) || var.name[0] == '#') {
        std::string str;
        try {
            str = calc_var_content_string(var);
        } catch(...) {
            return;
        }
        bool isHexAppVar = var.type == CALC_VAR_TYPE_APP_VAR && !calc_var_is_python_appvar(&var);
        BasicCodeViewerWindow *codePopup = new BasicCodeViewerWindow(this, !isHexAppVar);
        codePopup->setVariableName(nameIndex.data().toString());
        codePopup->setWindowModality(Qt::NonModal);
        codePopup->setAttribute(Qt::WA_DeleteOnClose);
        codePopup->setOriginalCode(QString::fromStdString(str), calc_var_is_tokenized(&var));
        codePopup->show();
    }
}

void MainWindow::emuBlocked(int req) {
    switch (req) {
        default:
        case EmuThread::RequestNone:
        case EmuThread::RequestPause:
            break;
        case EmuThread::RequestReceive:
            if (m_recvAction) {
                m_recvAction(true);
                m_recvAction = nullptr;
            } else {
                emu.unblock();
            }
            break;
    }
}

void MainWindow::varUpdate() {
    ui->buttonEnableVarList->setText(guiReceive ? tr("Hide Calculator Variables") : tr("View Calculator Variables"));
    ui->buttonRefreshVarList->setEnabled(guiReceive);
    ui->filterVarList->setEnabled(guiReceive);
    ui->buttonReceiveFiles->setEnabled(guiReceive);
    ui->buttonReceiveFile->setEnabled(guiReceive);
    ui->emuVarView->setEnabled(guiReceive);
}

void MainWindow::varShow() {
    if (!guiReceive) {
        m_recvAction = nullptr;

        m_varTableModel->clear();
    } else {
        if (m_recvAction) {
            return;
        }

        varReceive([this](bool isBlocked) {
            m_varTableModel->refresh();
            if (isBlocked) {
                emu.unblock();
            }
        });
    }
}

void MainWindow::varSaveSelected() {
    QVector<calc_var_t> selectedVars;
    QStringList fileNames;
    for (int currRow = 0; currRow < ui->emuVarView->model()->rowCount(); currRow++) {
        QModelIndex nameIndex = ui->emuVarView->model()->index(currRow, VarTableModel::VAR_NAME_COL);
        if (nameIndex.data(Qt::CheckStateRole) == Qt::Checked) {
            selectedVars.append(nameIndex.data(Qt::UserRole).value<calc_var_t>());
        }
    }
    if (selectedVars.size() < 2) {
        QMessageBox::warning(this, MSG_WARNING, tr("Select at least two files to group"));
    } else {
        fileNames = varDialog(QFileDialog::AcceptSave, tr("TI Group (*.8cg);;All Files (*.*)"), QStringLiteral("8cg"));
        if (fileNames.size() == 1) {
            varReceive([this, fileName = std::move(fileNames.first()), selectedVars = std::move(selectedVars)](bool isBlocked) {
                int status = emu_receive_variable(fileName.toUtf8(), selectedVars.constData(), selectedVars.size());
                if (isBlocked) {
                    emu.unblock();
                }
                if (status != LINK_GOOD) {
                    QMessageBox::critical(this, MSG_ERROR, tr("Transfer error, see console for information:\nFile: ") + fileName);
                } else {
                    QMessageBox::information(this, MSG_INFORMATION, tr("Transfer completed successfully."));
                }
            });
        }
    }
}

void MainWindow::varSaveSelectedFiles() {
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);

    dialog.setDirectory(m_dir);
    int good = 0;

    for (int currRow = 0; currRow < ui->emuVarView->model()->rowCount(); currRow++) {
        if (ui->emuVarView->model()->index(currRow, VarTableModel::VAR_NAME_COL).data(Qt::CheckStateRole) == Qt::Checked) {
            good = 1;
            break;
        }
    }

    if (!good) {
        QMessageBox::warning(this, MSG_WARNING, tr("Select at least one file to transfer"));
        return;
    }

    good = dialog.exec();
    m_dir.setPath(dialog.directory().absolutePath());

    if (!good) {
        return;
    }

    varReceive([this, dir = dialog.directory().absolutePath()](bool isBlocked) {
        QString name;
        QString filename;
        QString errMsg;

        for (int currRow = 0; currRow < ui->emuVarView->model()->rowCount(); currRow++) {
            QModelIndex nameIndex = ui->emuVarView->model()->index(currRow, VarTableModel::VAR_NAME_COL);
            if (nameIndex.data(Qt::CheckStateRole) == Qt::Checked) {
                calc_var_t var, varName = nameIndex.data(Qt::UserRole).value<calc_var_t>();
                // Find the variable first to ensure the current type is used for the file extension
                if (!vat_search_find(&varName, &var)) {
                    if (calc_var_is_list(&varName)) {
                        // Remove any linked formula before generating filename
                        varName.name[varName.namelen - 1] = 0;
                    }
                    name = QString(calc_var_name_to_utf8(varName.name, varName.namelen, varName.named));
                    errMsg = tr("Transfer error, variable no longer exists: ") + name;
                    break;
                }
                if (calc_var_is_list(&var)) {
                    // Remove any linked formula before generating filename
                    var.name[var.namelen - 1] = 0;
                }

                name = QString(calc_var_name_to_utf8(var.name, var.namelen, var.named));
                filename = dir + "/" + name + "." + m_varExtensions[var.type];

                if (emu_receive_variable(filename.toStdString().c_str(), &var, 1) != LINK_GOOD) {
                    errMsg = tr("Transfer error, see console for information:\nFile: ") + filename;
                    break;
                }
            }
        }
        if (isBlocked) {
            emu.unblock();
        }

        if (errMsg.isEmpty()) {
            QMessageBox::information(this, MSG_INFORMATION, tr("Transfer completed successfully."));
        } else {
            QMessageBox::critical(this, MSG_ERROR, errMsg);
        }
    });
}

void MainWindow::varResend() {
    sendingHandler->resendSelected();
}

// ------------------------------------------------
// Autotester things
// ------------------------------------------------

void MainWindow::pressKeyFromName(const std::string& key)
{
    autotester::pressKeyFromName(key);
}

void MainWindow::autotesterErr(int errCode) {
    QString errMsg;
    switch (errCode) {
        case 0:
            return; // no error
        case -1:
            errMsg = tr("Error. No config loaded");
            break;
        case 1:
            errMsg = tr("Error. Couldn't follow the test sequence defined in the configuration");
            break;
        default:
            errMsg = tr("Error. Unknown one - wat?");
            break;
    }
    QMessageBox::warning(this, MSG_ERROR, errMsg);
}


int MainWindow::autotesterOpen(const QString& jsonPath) {
    if (jsonPath.length() == 0) {
        QMessageBox::warning(this, MSG_ERROR, tr("Please choose a json file or type its path."));
        return 1;
    }
    std::string jsonContents;
    std::ifstream ifs(jsonPath.toStdString());

    if (ifs.good()) {
        const std::string absJsonDirPath = QDir::toNativeSeparators(QFileInfo(jsonPath).absoluteDir().path()).toStdString();
        // TODO: fix me (don't use chdir, or at the very least, go back to the previous path afterwards)
        if (chdir(absJsonDirPath.c_str()) != 0) {
            QMessageBox::warning(this, MSG_ERROR, tr("Couldn't go to where the JSON file is."));
            return 0;
        }
        std::getline(ifs, jsonContents, '\0');
        if (!ifs.eof()) {
            QMessageBox::critical(this, MSG_ERROR, tr("Couldn't read JSON file."));
            return 0;
        }
    } else {
        QMessageBox::critical(this, MSG_ERROR, tr("Unable to open the file."));
        return 1;
    }

    autotester::ignoreROMfield = true;
    autotester::debugMode = false;
    if (autotester::loadJSONConfig(jsonContents))
    {
        ui->JSONconfigPath->setText(jsonPath);
        ui->buttonLaunchTest->setEnabled(true);
        std::cout << jsonPath.toStdString() << " loaded and verified. " << autotester::config.hashes.size() << " unique tests found." << std::endl;
    } else {
        QMessageBox::critical(this, MSG_ERROR, tr("See the test config file format and make sure values are correct and referenced files are there."));
        return 1;
    }
    return 0;
}

void MainWindow::autotesterLoad() {
    QFileDialog dialog(this);
    int good;

    ui->buttonLaunchTest->setEnabled(false);

    dialog.setDirectory(m_dir);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(QStringLiteral("JSON config (*.json)"));

    good = dialog.exec();
    m_dir.setPath(dialog.directory().absolutePath());

    if (!good) {
        return;
    }

    QStringList selected = dialog.selectedFiles();
    autotesterOpen(selected.first());
}

void MainWindow::autotesterReload() {
    autotesterOpen(ui->JSONconfigPath->text());
}

void MainWindow::autotesterLaunch() {
    if (!autotester::configLoaded) {
        autotesterErr(-1);
        return;
    }

    if (ui->checkBoxTestReset->isChecked()) {
        resetEmu();
        guiDelay(4000);
    }

    if (ui->checkBoxTestClear->isChecked()) {
        sendEmuKey(CE_KEY_CLEAR);
    }

    QStringList filesList;
    for (const auto &file : autotester::config.transfer_files) {
        filesList << QString::fromStdString(file);
    }

    sendingHandler->sendFiles(filesList, LINK_FILE);
    equatesRefresh();
    while (guiSend) {
        guiDelay(10);
    }

    // Follow the sequence
    ui->buttonLaunchTest->setEnabled(false);
    emu.test(ui->JSONconfigPath->text(), true);
}

void MainWindow::autotesterTested(int status) {
    autotesterErr(status);
    if (!opts.suppressTestDialog) {
        QMessageBox::information(this, tr("Test results"), QString(tr("Out of %2 tests attempted:\n%4 passed\n%6 failed")).arg(QString::number(autotester::hashesTested), QString::number(autotester::hashesPassed), QString::number(autotester::hashesFailed)));
    }
    ui->buttonLaunchTest->setEnabled(true);
}

void MainWindow::autotesterUpdatePresets(int comboBoxIndex) {
    // The order matters, here! (See the combobox in the GUI)
    static const std::pair<unsigned int, unsigned int> mapIdConsts[] = {
        std::make_pair(autotester::hash_consts.at("vram_start"),     autotester::hash_consts.at("vram_16_size")),
        std::make_pair(autotester::hash_consts.at("vram_start"),     autotester::hash_consts.at("vram_8_size")),
        std::make_pair(autotester::hash_consts.at("vram2_start"),    autotester::hash_consts.at("vram_8_size")),
        std::make_pair(autotester::hash_consts.at("textShadow"),     autotester::hash_consts.at("textShadow_size")),
        std::make_pair(autotester::hash_consts.at("cmdShadow"),      autotester::hash_consts.at("cmdShadow_size")),
        std::make_pair(autotester::hash_consts.at("pixelShadow"),    autotester::hash_consts.at("pixelShadow_size")),
        std::make_pair(autotester::hash_consts.at("pixelShadow2"),   autotester::hash_consts.at("pixelShadow2_size")),
        std::make_pair(autotester::hash_consts.at("cmdPixelShadow"), autotester::hash_consts.at("cmdPixelShadow_size")),
        std::make_pair(autotester::hash_consts.at("plotSScreen"),    autotester::hash_consts.at("plotSScreen_size")),
        std::make_pair(autotester::hash_consts.at("saveSScreen"),    autotester::hash_consts.at("saveSScreen_size")),
        std::make_pair(autotester::hash_consts.at("lcdPalette"),     autotester::hash_consts.at("lcdPalette_size")),
        std::make_pair(autotester::hash_consts.at("cursorImage"),    autotester::hash_consts.at("cursorImage_size")),
        std::make_pair(autotester::hash_consts.at("ram_start"),      autotester::hash_consts.at("ram_size"))
    };
    if (comboBoxIndex >= 1 && comboBoxIndex <= static_cast<int>((sizeof(mapIdConsts)/sizeof(mapIdConsts[0])))) {
        char buf[10] = {0};
        snprintf(buf, sizeof(buf), "0x%X", mapIdConsts[comboBoxIndex-1].first);
        ui->startCRC->setText(buf);
        snprintf(buf, sizeof(buf), "0x%X", mapIdConsts[comboBoxIndex-1].second);
        ui->sizeCRC->setText(buf);
        autotesterRefreshCRC();
    }
}

void MainWindow::autotesterRefreshCRC() {
    QLineEdit *startCRC = ui->startCRC;
    QLineEdit *sizeCRC = ui->sizeCRC;

    startCRC->setText(startCRC->text().trimmed());
    sizeCRC->setText(sizeCRC->text().trimmed());

    if (startCRC->text().isEmpty() || sizeCRC->text().isEmpty()) {
        QMessageBox::critical(this, MSG_ERROR, tr("Make sure you have entered a valid start/size pair or preset."));
        return;
    }

    // Get GUI values
    bool ok1, ok2; // catch conversion issues
    uint32_t tmp_start = startCRC->text().toUInt(&ok1, 0);
    int32_t  crc_size  = sizeCRC->text().toInt(&ok2, 0);
    if (!ok1 || !ok2) {
        QMessageBox::critical(this, MSG_ERROR, tr("Could not convert those values into numbers"));
        return;
    }

    // Get real start pointer
    void* start = virt_mem_dup(tmp_start, crc_size);
    if (!start) {
        QMessageBox::critical(this, MSG_ERROR, tr("Could not retrieve this memory chunk"));
        return;
    }

    // Compute and display CRC
    ui->valueCRC->setText(int2hex(crc32(start, static_cast<size_t>(crc_size)), 8));
    ui->valueCRC->repaint();
    free(start);
}

void MainWindow::resetEmu() {
    guiReset = true;

    if (guiReceive) {
        varToggle();
    }
    if (guiDebugBasic) {
        debugBasic(false);
    }
    if (guiDebug) {
        debugToggle();
    }

    emu.reset();
}

void MainWindow::emuCheck(emu_state_t state, emu_data_t type) {

    /* don't need to do anything if just loading ram */
    if (type == EMU_DATA_RAM && state == EMU_STATE_VALID) {
        guiEmuValid = true;
        return;
    }

    /* verify emulation state */
    switch (state) {
        case EMU_STATE_VALID:
            break;
        case EMU_STATE_NOT_A_CE:
            if (QMessageBox::Yes == QMessageBox::question(this, MSG_WARNING, tr("Image does not appear to be from a CE. Do you want to attempt to load it anyway? "
                                                          "This may cause instability."), QMessageBox::Yes|QMessageBox::No)) {
                state = EMU_STATE_VALID;
            }
            break;
        case EMU_STATE_INVALID:
            if (type == EMU_DATA_IMAGE) {
                console(QStringLiteral("[CEmu] Failed loading image, falling back to ROM.\n"), EmuThread::ConsoleErr);
                emu.load(EMU_DATA_ROM, m_pathRom);
            }
            break;
    }

    if (state == EMU_STATE_VALID) {
        ui->lcd->setMain();
        setKeypadColor(m_config->value(SETTING_KEYPAD_COLOR, get_device_type() ? KEYPAD_WHITE : KEYPAD_BLACK).toUInt());
        for (const auto &dock : findChildren<DockWidget*>()) {
            if (dock->windowTitle() == TXT_VISUALIZER_DOCK) {
                static_cast<VisualizerWidget*>(dock->widget())->forceUpdate();
            }
        }
        emu.start();
        guiEmuValid = true;
    }

    guiReset = false;
}

void MainWindow::emuLoad(emu_data_t type) {
    guiEmuValid = false;
    QString path;

    if (guiReceive) {
        varToggle();
    }
    if (guiDebugBasic) {
        debugBasic(false);
    }
    if (guiDebug) {
        debugToggle();
    }

    switch (type) {
        case EMU_DATA_ROM:
            path = m_pathRom;
            break;
        case EMU_DATA_IMAGE:
            path = m_pathImage;
            break;
        case EMU_DATA_RAM:
            path = m_pathRam;
            break;
    }

    emu.load(type, path);
}

void MainWindow::disasmLine() {
    bool useLabel = false;
    map_t::iterator sit;
    std::pair<map_t::iterator, map_t::iterator> range;
    unsigned int numLines = 1;

    if (disasm.base != disasm.next) {
        disasm.base = disasm.next;
        if (disasm.map.count(static_cast<uint32_t>(disasm.next))) {
            range = disasm.map.equal_range(static_cast<uint32_t>(disasm.next));

            numLines = 0;
            for (sit = range.first;  sit != range.second;  ++sit) {
               numLines++;
            }

            disasm.highlight.watchR = false;
            disasm.highlight.watchW = false;
            disasm.highlight.breakP = false;
            disasm.highlight.pc = false;

            disasm.instr.data.clear();
            disasm.instr.opcode.clear();
            disasm.instr.operands.clear();
            disasm.instr.size = 0;

            useLabel = true;
        } else {
            disasmGet();
        }
    } else {
        disasmGet();
    }

    if (useLabel) {
        range = disasm.map.equal_range(static_cast<uint32_t>(disasm.next));
        sit = range.first;
    }

    for (unsigned int j = 0; j < numLines; j++) {

        QString line;
        QString symbols;

        if (useLabel) {
            if (disasm.base > 511 || (disasm.base < 512 && sit->second[0] == '_')) {
                line = QStringLiteral("%1  %2:")
                       .arg(disasm.addr ? int2hex(static_cast<uint32_t>(disasm.base), 6) : QString(),
                            QString::fromStdString(sit->second));

                m_disasm->appendPlainText(line);
            }

            if (numLines == j + 1) {
                useLabel = false;
            }
            sit++;
        } else {
            line = QString(QStringLiteral("%1 %2%3%4 %5  %6%7"))
                           .arg(disasm.addr ? int2hex(static_cast<uint32_t>(disasm.base), 6) : QString(),
                                disasm.highlight.watchR ? QStringLiteral("R") : QStringLiteral(" "),
                                disasm.highlight.watchW ? QStringLiteral("W") : QStringLiteral(" "),
                                disasm.highlight.breakP ? QStringLiteral("X") : QStringLiteral(" "),
                                disasm.bytes ? QString::fromStdString(disasm.instr.data).leftJustified(12, ' ') : QStringLiteral(" "),
                                QString::fromStdString(disasm.instr.opcode),
                                QString::fromStdString(disasm.instr.operands));

            m_disasm->appendPlainText(line);
        }
    }

    if (!m_disasmOffsetSet && disasm.next > m_disasmAddr) {
        m_disasmOffsetSet = true;
        m_disasmOffset = m_disasm->textCursor();
        m_disasmOffset.movePosition(QTextCursor::StartOfLine);
    }

    if (disasm.highlight.pc == true) {
        m_disasm->addHighlight(QColor(Qt::blue).lighter(180), QColor(Qt::blue).darker(160));
    }
}

void MainWindow::contextDisasm(const QPoint &posa) {
    m_disasm->setTextCursor(m_disasm->cursorForPosition(posa));
    QPoint globalPos = m_disasm->mapToGlobal(posa);
    QString addrStr = m_disasm->getSelectedAddr();
    uint32_t addr = static_cast<uint32_t>(hex2int(addrStr));

    QMenu menu;
    QAction *runUntil = menu.addAction(ACTION_RUN_UNTIL);
    menu.addSeparator();
    QAction *toggleBreak = menu.addAction(ACTION_TOGGLE_BREAK);
    QAction *toggleRead = menu.addAction(ACTION_TOGGLE_READ);
    QAction *toggleWrite = menu.addAction(ACTION_TOGGLE_WRITE);
    QAction *toggleRw = menu.addAction(ACTION_TOGGLE_RW);
    menu.addSeparator();
    QAction *gotoMem = gotoMemAction(&menu);
    QAction *setPc = menu.addAction(tr("Set PC"));

    QAction *item = menu.exec(globalPos);
    if (item == setPc) {
        ui->pcregView->setText(addrStr);
        debug_set_pc(addr);
        disasmUpdateAddr(static_cast<int>(cpu.registers.PC), true);
    } else if (item == toggleBreak) {
        breakAddGui();
        memUpdate();
    } else if (item == toggleRead) {
        watchAddGuiR();
        memUpdate();
    } else if (item == toggleWrite) {
        watchAddGuiW();
        memUpdate();
    } else if (item == toggleRw) {
        watchAddGuiRW();
        memUpdate();
    } else if (item == runUntil) {
        m_runUntilAddr = addr;
        debugToggle();
        debugStep(DBG_RUN_UNTIL);
    } else if (item == gotoMem) {
        gotoMemAddr(addr);
    }
}

void MainWindow::varLaunch(const calc_var_t *prgm) {
    keypadBridge->releaseAll();
    ui->lcd->setFocus();
    guiDelay(50);

    sendEmuKey(CE_KEY_CLEAR);
    if (calc_var_is_asmprog(prgm)) {
        sendEmuKey(CE_KEY_ASM);
    }
    sendEmuKey(CE_KEY_PRGM);
    for (const uint8_t *c = prgm->name; *c; c++) {
        sendEmuLetterKey(static_cast<char>(*c)); // type program name
    }
    sendEmuKey(CE_KEY_ENTER);
    ui->lcd->setFocus();
}

void MainWindow::contextVars(const QPoint& posa) {
    QModelIndex nameIndex = ui->emuVarView->indexAt(posa);
    nameIndex = nameIndex.sibling(nameIndex.row(), VarTableModel::VAR_NAME_COL);
    if (!nameIndex.isValid()) {
        return;
    }

    const calc_var_t var = nameIndex.data(Qt::UserRole).value<calc_var_t>();

    QMenu contextMenu;
    QAction *launch = contextMenu.addAction(tr("Launch program"));
    launch->setVisible(calc_var_is_prog(&var) && !calc_var_is_internal(&var));

    QAction *selectedItem = contextMenu.exec(ui->emuVarView->mapToGlobal(posa));
    if (selectedItem == launch) {
        varLaunch(&var);
    }
}

void MainWindow::contextConsole(const QPoint &posa) {
    bool ok = true;

    QPoint globalp = ui->console->mapToGlobal(posa);
    QTextCursor cursor = ui->console->cursorForPosition(posa);
    ui->console->setTextCursor(cursor);
    cursor.select(QTextCursor::WordUnderCursor);

    QString equ = getAddressOfEquate(cursor.selectedText().toUpper().toStdString());
    uint32_t address;

    if (!equ.isEmpty()) {
        address = static_cast<uint32_t>(hex2int(equ));
    } else {
        address = cursor.selectedText().toUInt(&ok, 16);
    }

    if (ok) {
        ui->console->setTextCursor(cursor);

        QMenu menu;
        QAction *gotoMem = gotoMemAction(&menu);
        QAction *gotoDisasm = gotoDisasmAction(&menu);
        menu.addSeparator();
        QAction *toggleBreak = menu.addAction(ACTION_TOGGLE_BREAK);
        QAction *toggleRead = menu.addAction(ACTION_TOGGLE_READ);
        QAction *toggleWrite = menu.addAction(ACTION_TOGGLE_WRITE);
        QAction *toggleRw = menu.addAction(ACTION_TOGGLE_RW);

        QAction *item = menu.exec(globalp);
        if (item) {
            if (item == gotoMem) {
                debugForce();
                gotoMemAddr(address);
            } else if (item == gotoDisasm) {
                debugForce();
                gotoDisasmAddr(address);
            } else if (item == toggleBreak) {
                breakAdd(breakNextLabel(), address, true, true, false);
            } else if (item == toggleRead) {
                watchAdd(watchNextLabel(), address, address, DBG_MASK_READ, true, false);
            } else if (item == toggleWrite) {
                watchAdd(watchNextLabel(), address, address, DBG_MASK_WRITE, true, false);
            } else if (item == toggleRw) {
                watchAdd(watchNextLabel(), address, address, DBG_MASK_READ | DBG_MASK_WRITE, true, false);
            }
            memDocksUpdate();
        }
    }
}

// ------------------------------------------------
// GUI IPC things
// ------------------------------------------------

bool MainWindow::ipcSetup() {
    // start the main communictions
    if (com.ipcSetup(opts.idString, opts.pidString)) {
        return true;
    }

    // if failure, then send a command to the other process with the command options
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_5);
    unsigned int type = IPC_CLI;

    stream << type
           << opts.useUnthrottled
           << opts.suppressTestDialog
           << opts.deforceReset
           << opts.forceReloadRom
           << opts.romFile
           << opts.autotesterFile
           << opts.imageFile
           << opts.sendFiles
           << opts.sendArchFiles
           << opts.sendRAMFiles
           << opts.restoreOnOpen
           << opts.speed
           << opts.launchPrgm
           << opts.screenshotFile;

    // blocking call
    com.send(byteArray);
    return false;
}

void MainWindow::ipcCli(QDataStream &stream) {
    console(QStringLiteral("[CEmu] Received IPC: command line options\n"));
    CEmuOpts o;

    stream >> o.useUnthrottled
           >> o.suppressTestDialog
           >> o.deforceReset
           >> o.forceReloadRom
           >> o.romFile
           >> o.autotesterFile
           >> o.imageFile
           >> o.sendFiles
           >> o.sendArchFiles
           >> o.sendRAMFiles
           >> o.restoreOnOpen
           >> o.speed
           >> o.launchPrgm
           >> o.screenshotFile;

    optLoadFiles(o);
    optAttemptLoad(o);
    optSend(o);
    if (o.speed != -1) {
        setEmuSpeed(o.speed);
    }
    if (!o.screenshotFile.isEmpty()) {
        ui->lcd->getImage().save(o.screenshotFile, "PNG", 0);
    }
}

void MainWindow::ipcCloseConnected() {
    QString idPath = configPath + "/id/";
    QDir dir(idPath);
    QStringList clients = dir.entryList(QDir::Filter::Files);
    int delay = 100;

    foreach (const QString &id, clients) {
        QFile file(idPath + id);
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream stream(&file);
            QString pid = stream.readLine();
            if (!isProcRunning(static_cast<pid_t>(pid.toLongLong()))) {
                file.close();
                file.remove();
                continue;
            } else {
                if (opts.pidString != pid) {
                    QByteArray byteArray;
                    QDataStream dataStream(&byteArray, QIODevice::WriteOnly);
                    dataStream.setVersion(QDataStream::Qt_5_5);
                    unsigned int type = IPC_CLOSE;
                    stream << type;

                    com.clientSetup(pid);
                    com.send(byteArray);
                    delay += 100;
                }
            }
        }
        file.close();
    }

    // wait for the settings to be written by the other processes
    guiDelay(delay);
}

void MainWindow::ipcReceived() {
    QByteArray byteArray(com.getData());

    QDataStream stream(byteArray);
    stream.setVersion(QDataStream::Qt_5_5);
    unsigned int type;

    stream >> type;

    switch (type) {
        case IPC_CLI:
           if (m_setup) {
               show();
               raise();
           }
           ipcCli(stream);
           break;
        case IPC_CLOSE:
            close();
            break;
        default:
           console(QStringLiteral("[CEmu] IPC Unknown\n"), EmuThread::ConsoleErr);
           break;
    }
}

void MainWindow::ipcSetId() {
    bool ok = true;
    QString text = QInputDialog::getText(this, tr("CEmu Change ID"), tr("New ID:"), QLineEdit::Normal, opts.idString, &ok);
    if (ok && !text.isEmpty() && text != opts.idString) {
        if (!InterCom::idOpen(text)) {
            com.idClose();
            com.ipcSetup(opts.idString = text, opts.pidString);
            console(QStringLiteral("[CEmu] Initialized Server [") + opts.idString + QStringLiteral(" | ") + com.getServerName() + QStringLiteral("]\n"));
            setWindowTitle(QStringLiteral("CEmu | ") + opts.idString);
        }
    }
}

void MainWindow::ipcSpawn() {
    QStringList arguments;
    arguments << QStringLiteral("--id") << randomString(15);

    QProcess *myProcess = new QProcess(this);
    myProcess->startDetached(execPath, arguments);
}

void MainWindow::stateAddNew() {
    QString name = randomString(6);
    QString path = QDir::cleanPath(QFileInfo(m_config->fileName()).absoluteDir().absolutePath() + QStringLiteral("/") + name + QStringLiteral(".ce"));
    stateAdd(name, path);
}

void MainWindow::stateAdd(QString &name, QString &path) {
    const int row = ui->slotView->rowCount();
    ui->slotView->setSortingEnabled(false);

    QToolButton *btnLoad   = new QToolButton();
    QToolButton *btnSave   = new QToolButton();
    QToolButton *btnEdit   = new QToolButton();
    QToolButton *btnRemove = new QToolButton();

    btnLoad->setIcon(m_iconLoad);
    btnSave->setIcon(m_iconSave);
    btnEdit->setIcon(m_iconEdit);
    btnRemove->setIcon(m_iconRemove);

    connect(btnRemove, &QToolButton::clicked, this, &MainWindow::stateRemove);
    connect(btnLoad, &QToolButton::clicked, this, &MainWindow::stateLoad);
    connect(btnSave, &QToolButton::clicked, this, &MainWindow::stateSave);
    connect(btnEdit, &QToolButton::clicked, this, &MainWindow::stateEdit);

    QTableWidgetItem *itemName   = new QTableWidgetItem(name);
    QTableWidgetItem *itemLoad   = new QTableWidgetItem();
    QTableWidgetItem *itemSave   = new QTableWidgetItem();
    QTableWidgetItem *itemEdit   = new QTableWidgetItem();
    QTableWidgetItem *itemRemove = new QTableWidgetItem();

    itemEdit->setData(Qt::UserRole, path);
    stateToPath(itemEdit->data(Qt::UserRole).toString());

    ui->slotView->setRowCount(row + 1);
    ui->slotView->setItem(row, SLOT_NAME_COL, itemName);
    ui->slotView->setItem(row, SLOT_LOAD_COL, itemLoad);
    ui->slotView->setItem(row, SLOT_SAVE_COL, itemSave);
    ui->slotView->setItem(row, SLOT_EDIT_COL, itemEdit);
    ui->slotView->setItem(row, SLOT_REMOVE_COL, itemRemove);

    ui->slotView->setCellWidget(row, SLOT_LOAD_COL, btnLoad);
    ui->slotView->setCellWidget(row, SLOT_SAVE_COL, btnSave);
    ui->slotView->setCellWidget(row, SLOT_EDIT_COL, btnEdit);
    ui->slotView->setCellWidget(row, SLOT_REMOVE_COL, btnRemove);

    ui->slotView->setCurrentCell(row, SLOT_NAME_COL);

    stateSaveInfo();

    ui->slotView->setVisible(false);
    ui->slotView->resizeColumnsToContents();
    ui->slotView->setVisible(true);
    ui->slotView->setSortingEnabled(true);
}

int MainWindow::stateGet(QObject *obj, int col) {
    int row;

    for (row = 0; row < ui->slotView->rowCount(); row++){
        if (obj == ui->slotView->cellWidget(row, col)) {
            break;
        }
    }

    return row;
}

void MainWindow::stateEdit() {
    bool ok;
    int row = stateGet(sender(), SLOT_EDIT_COL);
    QString old = ui->slotView->item(row, SLOT_EDIT_COL)->data(Qt::UserRole).toString();
    QString path = QInputDialog::getText(this, tr("Enter image path"), Q_NULLPTR, QLineEdit::Normal, old, &ok);
    if (ok && !path.isEmpty()) {
        QFile(old).rename(path);
        ui->slotView->item(row, SLOT_EDIT_COL)->setData(Qt::UserRole, path);
    }
}

void MainWindow::stateRemove() {
    int row = stateGet(sender(), SLOT_REMOVE_COL);
    QFile(ui->slotView->item(row, SLOT_EDIT_COL)->data(Qt::UserRole).toString()).remove();
    ui->slotView->removeRow(row);
}

void MainWindow::stateSave() {
    int row = stateGet(sender(), SLOT_SAVE_COL);
    QString path = ui->slotView->item(row, SLOT_EDIT_COL)->data(Qt::UserRole).toString();
    stateToPath(path);
}

void MainWindow::stateLoad() {
    int row = stateGet(sender(), SLOT_LOAD_COL);
    QString path = ui->slotView->item(row, SLOT_EDIT_COL)->data(Qt::UserRole).toString();
    stateFromPath(path);
}

const char *MainWindow::m_varExtensions[] = {
        "8xn",  // 00
        "8xl",
        "8xm",
        "8xy",
        "8xs",
        "8xp",
        "8xp",
        "8ci",
        "8xd",  // 08
        "",
        "",
        "8xw",  // 0B
        "8xc",
        "8xl",  // 0D
        "",
        "8xw",  // 0F
        "8xz",  // 10
        "8xt",  // 11
        "",
        "",
        "",
        "8xv",  // 15
        "",
        "8cg",  // 17
        "8xn",  // 18
        "",
        "8ca",  // 1A
        "8xc",
        "8xn",
        "8xc",
        "8xc",
        "8xc",
        "8xn",
        "8xn",  // 21
        "",
        "8pu",  // 23
        "8ek",  // 24
        "",
        "",
        "",
        "",
    };
