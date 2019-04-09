#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils.h"
#include "sendinghandler.h"
#include "visualizerwidget.h"
#include "dockwidget.h"
#include "searchwidget.h"
#include "basiccodeviewerwindow.h"
#include "capture/animated-png.h"
#include "tivars_lib_cpp/src/TIModels.h"
#include "tivars_lib_cpp/src/TIVarTypes.h"
#include "tivars_lib_cpp/src/TypeHandlers/TypeHandlers.h"
#include "../../core/emu.h"
#include "../../core/asic.h"
#include "../../core/cpu.h"
#include "../../core/misc.h"
#include "../../core/mem.h"
#include "../../core/extras.h"
#include "../../core/interrupt.h"
#include "../../core/keypad.h"
#include "../../core/control.h"
#include "../../core/flash.h"
#include "../../core/lcd.h"
#include "../../core/spi.h"
#include "../../core/backlight.h"
#include "../../core/timers.h"
#include "../../core/usb/usb.h"
#include "../../core/realclock.h"
#include "../../core/sha256.h"
#include "../../core/link.h"
#include "../../tests/autotester/crc32.hpp"
#include "../../tests/autotester/autotester.h"

#include <QtCore/QBuffer>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QSettings>
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtGui/QWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QProgressDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QProgressDialog>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QShortcut>

#include <fstream>
#include <iostream>
#include <math.h>

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
    m_appTranslator.load(QLocale::system().name(), QStringLiteral(":/i18n/i18n/"));
    qApp->installTranslator(&m_appTranslator);

    // setting metatypes
    qRegisterMetaTypeStreamOperators<QList<int>>("QList<int>");
    qRegisterMetaTypeStreamOperators<QList<bool>>("QList<bool>");

    m_isInDarkMode = isRunningInDarkMode();

    ui->setupUi(this);

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
    tivars::TH_Tokenized::initTokens();

    ui->centralWidget->hide();
    ui->statusBar->addWidget(&m_speedLabel);
    ui->statusBar->addPermanentWidget(&m_msgLabel);
    ui->statusBar->addPermanentWidget(&m_fpsLabel);

    m_watchpoints = ui->watchpoints;
    m_breakpoints = ui->breakpoints;
    m_ports = ui->ports;
    m_disasm = ui->disasm;

    m_disasmOpcodeColor = m_isInDarkMode ? "darkorange" : "darkblue";

    ui->console->setMaximumBlockCount(1000);

    varPreviewCEFont = QFont(QStringLiteral("TICELarge"), 11);
    varPreviewItalicFont.setItalic(true);

    setWindowTitle(QStringLiteral("CEmu | ") + opts.idString);

    connect(keypadBridge, &QtKeypadBridge::keyStateChanged, ui->keypadWidget, &KeypadWidget::changeKeyState);
    connect(keypadBridge, &QtKeypadBridge::sendKeys, &emu, &EmuThread::enqueueKeys);
    installEventFilter(keypadBridge);
    for (const auto &tab : ui->tabWidget->children()[0]->children()) {
        tab->installEventFilter(keypadBridge);
    }

    m_progressBar = new QProgressBar(this);
    m_progressBar->setMaximumHeight(ui->statusBar->height() / 2);
    ui->statusBar->addWidget(m_progressBar);
    sendingHandler = new SendingHandler(this, m_progressBar, ui->varLoadedView);

    // emulator -> gui (Should be queued)
    connect(&emu, &EmuThread::consoleStr, this, &MainWindow::consoleStr, Qt::UniqueConnection);
    connect(&emu, &EmuThread::consoleClear, this, &MainWindow::consoleClear, Qt::QueuedConnection);
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
    connect(ui->checkDisableSoftCommands, &QCheckBox::toggled, this, &MainWindow::setDebugSoftCommands);
    connect(ui->buttonZero, &QPushButton::clicked, this, &MainWindow::debugZeroCycles);
    connect(ui->buttonCertID, &QPushButton::clicked, this, &MainWindow::setCalcId);
    connect(m_disasm, &DataWidget::gotoDisasmAddress, this, &MainWindow::gotoDisasmAddr);
    connect(m_disasm, &DataWidget::gotoMemoryAddress, this, &MainWindow::gotoMemAddr);
    connect(ui->sources, &SourcesWidget::runUntilTriggered, this, &MainWindow::runUntil);
    connect(ui->sources, &SourcesWidget::breakToggled, this, &MainWindow::breakToggle);
    connect(this, &MainWindow::debugPointChanged, ui->sources, &SourcesWidget::updateAddr);

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
    connect(ui->checkDma, &QCheckBox::toggled, this, &MainWindow::setLcdDma);
    connect(ui->textSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::setFont);

    // debug files
    connect(ui->actionImportDebugger, &QAction::triggered, [this]{ debugImportFile(debugGetFile(false)); });
    connect(ui->actionExportDebugger, &QAction::triggered, [this]{ debugExportFile(debugGetFile(true)); });

    // linking
    connect(ui->buttonSend, &QPushButton::clicked, this, &MainWindow::varSelect);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::varSelect);
    connect(ui->buttonRefreshList, &QPushButton::clicked, this, &MainWindow::varToggle);
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
    connect(this, &MainWindow::setLcdMode, ui->lcd, &LCDWidget::setMode);
    connect(this, &MainWindow::setLcdFrameskip, ui->lcd, &LCDWidget::setFrameskip);
    connect(ui->statusInterval, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::setStatusInterval);
    connect(&m_timerEmu, &QTimer::timeout, [this]{ m_timerEmuTriggered = true; });
    connect(&m_timerFps, &QTimer::timeout, [this]{ m_timerFpsTriggered = true; });

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
    connect(ui->guiSkip, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::setGuiSkip);
    connect(ui->checkSkin, &QCheckBox::stateChanged, this, &MainWindow::setSkinToggle);
    connect(ui->checkSpi, &QCheckBox::toggled, this, &MainWindow::setLcdSpi);
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
    connect(ui->checkPreI, &QCheckBox::stateChanged, this, &MainWindow::setPreRevisionI);
    connect(ui->checkNormOs, &QCheckBox::stateChanged, this, &MainWindow::setNormalOs);
    connect(ui->flashBytes, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui->flashEdit, &HexWidget::setBytesPerLine);
    connect(ui->ramBytes, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui->ramEdit, &HexWidget::setBytesPerLine);
    connect(ui->ramAscii, &QToolButton::toggled, [this](bool set){ ui->ramEdit->setAsciiArea(set); });
    connect(ui->flashAscii, &QToolButton::toggled, [this](bool set){ ui->flashEdit->setAsciiArea(set); });
    connect(ui->emuVarView, &QTableWidget::itemDoubleClicked, this, &MainWindow::varPressed);
    connect(ui->emuVarView, &QTableWidget::customContextMenuRequested, this, &MainWindow::contextVars);
    connect(ui->buttonAddSlot, &QPushButton::clicked, this, &MainWindow::stateAddNew);
    connect(ui->actionExportCEmuImage, &QAction::triggered, this, &MainWindow::bootImageExport);
    connect(ui->lcd, &LCDWidget::sendROM, this, &MainWindow::setRom);
    connect(ui->lcd, &LCDWidget::customContextMenuRequested, this, &MainWindow::contextLcd);
    connect(ui->checkUpdates, &QCheckBox::stateChanged, this, &MainWindow::setAutoUpdates);

    // languages
    connect(ui->actionEnglish,  &QAction::triggered, [this]{ translateSwitch(QLocale::English); });
    connect(ui->actionFran_ais, &QAction::triggered, [this]{ translateSwitch(QLocale::French); });
    connect(ui->actionDutch,    &QAction::triggered, [this]{ translateSwitch(QLocale::Dutch); });
    connect(ui->actionEspanol,  &QAction::triggered, [this]{ translateSwitch(QLocale::Spanish); });

    // sending handler
    connect(sendingHandler, &SendingHandler::send, &emu, &EmuThread::send, Qt::QueuedConnection);
    connect(&emu, &EmuThread::sentFile, sendingHandler, &SendingHandler::sentFile, Qt::QueuedConnection);
    connect(sendingHandler, &SendingHandler::loadEquateFile, this, &MainWindow::equatesAddFile);

    // memory editors
    connect(ui->buttonFlashSearch, &QPushButton::clicked, [this]{ memSearchEdit(ui->flashEdit); });
    connect(ui->buttonRamSearch, &QPushButton::clicked, [this]{ memSearchEdit(ui->ramEdit); });
    connect(ui->buttonFlashGoto, &QPushButton::clicked, this, &MainWindow::flashGotoPressed);
    connect(ui->buttonRamGoto, &QPushButton::clicked, this, &MainWindow::ramGotoPressed);
    connect(ui->buttonFlashSync, &QToolButton::clicked, this, &MainWindow::flashSyncPressed);
    connect(ui->buttonRamSync, &QToolButton::clicked, this, &MainWindow::ramSyncPressed);
    connect(ui->flashEdit, &HexWidget::customContextMenuRequested, this, &MainWindow::contextMem);
    connect(ui->ramEdit, &HexWidget::customContextMenuRequested, this, &MainWindow::contextMem);

    // keymap
    connect(ui->radioNaturalKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radioCEmuKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radioTilEmKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radioWabbitemuKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radiojsTIfiedKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);
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
    connect(ui->buttonCrystalClear, &QPushButton::clicked, this, &MainWindow::keypadChanged);

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
    m_shortcutResend = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_X), this);

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

    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    // configure table font
    const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
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
        m_config = new QSettings;
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
    setSkinToggle(m_config->value(SETTING_SCREEN_SKIN, true).toBool());
    setLcdSpi(m_config->value(SETTING_SCREEN_SPI, true).toBool());
    setGuiSkip(m_config->value(SETTING_SCREEN_FRAMESKIP, 0).toInt());
    setEmuSpeed(m_config->value(SETTING_EMUSPEED, 100).toInt());
    setAutoSave(m_config->value(SETTING_RESTORE_ON_OPEN, true).toBool());
    setFont(m_config->value(SETTING_DEBUGGER_TEXT_SIZE, 9).toInt());
    setDebugDisasmSpace(m_config->value(SETTING_DEBUGGER_DISASM_SPACE, false).toBool());
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
    setPreRevisionI(m_config->value(SETTING_DEBUGGER_PRE_I, false).toBool());
    setNormalOs(m_config->value(SETTING_DEBUGGER_NORM_OS, true).toBool());
    setLcdDma(m_config->value(SETTING_DEBUGGER_IGNORE_DMA, true).toBool());
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

    m_cBack.setColor(QPalette::Base, QColor(isRunningInDarkMode() ? Qt::blue : Qt::yellow).lighter(160));
    m_consoleFormat = ui->console->currentCharFormat();
}

void MainWindow::translateSwitch(const QLocale& locale) {
#ifdef HAS_LIBUSB
    static const QHash<QLocale, quint16> langIDMap = {
        {QLocale::Afrikaans, 0x0436},
        {QLocale::Albanian, 0x041c},
        {{QLocale::Arabic, QLocale::SaudiArabia}, 0x0401},
        {{QLocale::Arabic, QLocale::Iraq}, 0x0801},
        {{QLocale::Arabic, QLocale::Egypt}, 0x0c01},
        {{QLocale::Arabic, QLocale::Libya}, 0x1001},
        {{QLocale::Arabic, QLocale::Algeria}, 0x1401},
        {{QLocale::Arabic, QLocale::Morocco}, 0x1801},
        {{QLocale::Arabic, QLocale::Tunisia}, 0x1c01},
        {{QLocale::Arabic, QLocale::Oman}, 0x2001},
        {{QLocale::Arabic, QLocale::Yemen}, 0x2401},
        {{QLocale::Arabic, QLocale::Syria}, 0x2801},
        {{QLocale::Arabic, QLocale::Jordan}, 0x2c01},
        {{QLocale::Arabic, QLocale::Lebanon}, 0x3001},
        {{QLocale::Arabic, QLocale::Kuwait}, 0x3401},
        {{QLocale::Arabic, QLocale::UnitedArabEmirates}, 0x3801},
        {{QLocale::Arabic, QLocale::Bahrain}, 0x3c01},
        {{QLocale::Arabic, QLocale::Qatar}, 0x4001},
        {QLocale::Armenian, 0x042b},
        {QLocale::Assamese, 0x044d},
        {{QLocale::Azerbaijani, QLocale::LatinScript, QLocale::Azerbaijan}, 0x042c},
        {{QLocale::Azerbaijani, QLocale::CyrillicScript, QLocale::Azerbaijan}, 0x082c},
        {QLocale::Basque, 0x042d},
        {QLocale::Belarusian, 0x0423},
        {QLocale::Bengali, 0x0445},
        {QLocale::Bulgarian, 0x0402},
        {QLocale::Burmese, 0x0455},
        {QLocale::Catalan, 0x0403},
        {{QLocale::Chinese, QLocale::Taiwan}, 0x0404},
        {{QLocale::Chinese, QLocale::China}, 0x0804},
        {{QLocale::Chinese, QLocale::HongKong}, 0x0c04},
        {{QLocale::Chinese, QLocale::Singapore}, 0x1004},
        {{QLocale::Chinese, QLocale::Macau}, 0x1404},
        {QLocale::Croatian, 0x041a},
        {QLocale::Czech, 0x0405},
        {QLocale::Danish, 0x0406},
        {{QLocale::Dutch, QLocale::Netherlands}, 0x0413},
        {{QLocale::Dutch, QLocale::Belgium}, 0x0813},
        {{QLocale::English, QLocale::UnitedStates}, 0x0409},
        {{QLocale::English, QLocale::UnitedKingdom}, 0x0809},
        {{QLocale::English, QLocale::Australia}, 0x0c09},
        {{QLocale::English, QLocale::Canada}, 0x1009},
        {{QLocale::English, QLocale::NewZealand}, 0x1409},
        {{QLocale::English, QLocale::Ireland}, 0x1809},
        {{QLocale::English, QLocale::SouthAfrica}, 0x1c09},
        {{QLocale::English, QLocale::Jamaica}, 0x2009},
        //{{QLocale::English, QLocale::Caribbean}, 0x2409},
        {{QLocale::English, QLocale::Belize}, 0x2809},
        {{QLocale::English, QLocale::TrinidadAndTobago}, 0x2c09},
        {{QLocale::English, QLocale::Zimbabwe}, 0x3009},
        {{QLocale::English, QLocale::Philippines}, 0x3409},
        {QLocale::Estonian, 0x0425},
        {QLocale::Faroese, 0x0438},
        {QLocale::Persian, 0x0429},
        {QLocale::Finnish, 0x040b},
        {QLocale::French, 0x040c},
        {{QLocale::French, QLocale::Belgium}, 0x080c},
        {{QLocale::French, QLocale::Canada}, 0x0c0c},
        {{QLocale::French, QLocale::Switzerland}, 0x100c},
        {{QLocale::French, QLocale::Luxembourg}, 0x140c},
        {{QLocale::French, QLocale::Monaco}, 0x180c},
        {QLocale::Georgian, 0x0437},
        {QLocale::German, 0x0407},
        {{QLocale::German, QLocale::Switzerland}, 0x0807},
        {{QLocale::German, QLocale::Austria}, 0x0c07},
        {{QLocale::German, QLocale::Luxembourg}, 0x1007},
        {{QLocale::German, QLocale::Liechtenstein}, 0x1407},
        {QLocale::Greek, 0x0408},
        {QLocale::Gujarati, 0x0447},
        {QLocale::Hebrew, 0x040d},
        {QLocale::Hindi, 0x0439},
        {QLocale::Hungarian, 0x040e},
        {QLocale::Icelandic, 0x040f},
        {QLocale::Indonesian, 0x0421},
        {QLocale::Italian, 0x0410},
        {{QLocale::Italian, QLocale::Switzerland}, 0x0810},
        {QLocale::Japanese, 0x0411},
        {QLocale::Kannada, 0x044b},
        {{QLocale::Kashmiri, QLocale::India}, 0x0860},
        {QLocale::Kazakh, 0x043f},
        {QLocale::Konkani, 0x0457},
        {QLocale::Korean, 0x0412},
        //{{QLocale::Korean, QLocale::Johab}, 0x0812},
        {QLocale::Latvian, 0x0426},
        {QLocale::Lithuanian, 0x0427},
        //{{QLocale::Lithuanian, QLocale::Classic}, 0x0827},
        {QLocale::Macedonian, 0x042f},
        {{QLocale::Malay, QLocale::Malaysia}, 0x043e},
        {{QLocale::Malay, QLocale::Brunei}, 0x083e},
        {QLocale::Malayalam, 0x044c},
        {QLocale::Manipuri, 0x0458},
        {QLocale::Marathi, 0x044e},
        {{QLocale::Nepali, QLocale::India}, 0x0861},
        {{QLocale::NorwegianBokmal}, 0x0414},
        {{QLocale::NorwegianNynorsk}, 0x0814},
        {QLocale::Oriya, 0x0448},
        {QLocale::Polish, 0x0415},
        {{QLocale::Portuguese, QLocale::Brazil}, 0x0416},
        {QLocale::Portuguese, 0x0816},
        {QLocale::Punjabi, 0x0446},
        {QLocale::Romanian, 0x0418},
        {QLocale::Russian, 0x0419},
        {QLocale::Sanskrit, 0x044f},
        {{QLocale::Serbian, QLocale::CyrillicScript, QLocale::Serbia}, 0x0c1a},
        {{QLocale::Serbian, QLocale::LatinScript, QLocale::Serbia}, 0x081a},
        {QLocale::Sindhi, 0x0459},
        {QLocale::Slovak, 0x041b},
        {QLocale::Slovenian, 0x0424},
        {QLocale::Spanish, 0x040a},
        {{QLocale::Spanish, QLocale::Mexico}, 0x080a},
        //{{QLocale::Spanish, QLocale::ModernSort}, 0x0c0a},
        {{QLocale::Spanish, QLocale::Guatemala}, 0x100a},
        {{QLocale::Spanish, QLocale::CostaRica}, 0x140a},
        {{QLocale::Spanish, QLocale::Panama}, 0x180a},
        {{QLocale::Spanish, QLocale::DominicanRepublic}, 0x1c0a},
        {{QLocale::Spanish, QLocale::Venezuela}, 0x200a},
        {{QLocale::Spanish, QLocale::Colombia}, 0x240a},
        {{QLocale::Spanish, QLocale::Peru}, 0x280a},
        {{QLocale::Spanish, QLocale::Argentina}, 0x2c0a},
        {{QLocale::Spanish, QLocale::Ecuador}, 0x300a},
        {{QLocale::Spanish, QLocale::Chile}, 0x340a},
        {{QLocale::Spanish, QLocale::Uruguay}, 0x380a},
        {{QLocale::Spanish, QLocale::Paraguay}, 0x3c0a},
        {{QLocale::Spanish, QLocale::Bolivia}, 0x400a},
        {{QLocale::Spanish, QLocale::ElSalvador}, 0x440a},
        {{QLocale::Spanish, QLocale::Honduras}, 0x480a},
        {{QLocale::Spanish, QLocale::Nicaragua}, 0x4c0a},
        {{QLocale::Spanish, QLocale::PuertoRico}, 0x500a},
        {QLocale::SouthernSotho, 0x0430},
        {{QLocale::Swahili, QLocale::Kenya}, 0x0441},
        {QLocale::Swedish, 0x041d},
        {{QLocale::Swedish, QLocale::Finland}, 0x081d},
        {QLocale::Tamil, 0x0449},
        //{{QLocale::Tatar, QLocale::Tatarstan}, 0x0444},
        {QLocale::Telugu, 0x044a},
        {QLocale::Thai, 0x041e},
        {QLocale::Turkish, 0x041f},
        {QLocale::Ukrainian, 0x0422},
        {{QLocale::Urdu, QLocale::Pakistan}, 0x0420},
        {{QLocale::Urdu, QLocale::India}, 0x0820},
        {{QLocale::Uzbek, QLocale::LatinScript, QLocale::Uzbekistan}, 0x0443},
        {{QLocale::Uzbek, QLocale::CyrillicScript, QLocale::Uzbekistan}, 0x0843},
        {QLocale::Vietnamese, 0x042a},
    };
    m_usbLangID = langIDMap.value(locale, langIDMap.value(locale.language(), 0x0409));
    libusb_setlocale(locale.name().toUtf8().constData());
#endif
    qApp->removeTranslator(&m_appTranslator);
    // For English, nothing to load after removing the translator.
    if (locale.language() == QLocale::English ||
        (m_appTranslator.load(locale, QStringLiteral(":/i18n/i18n/"))
         && qApp->installTranslator(&m_appTranslator))) {
        m_config->setValue(SETTING_PREFERRED_LANG, locale);
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
    }

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
        action->setText(TXT_DEBUG_CONTROL);
        action = m_menuDebug->actions().at(1);
        action->setText(TXT_CPU_STATUS);
        action = m_menuDebug->actions().at(2);
        action->setText(TXT_DISASSEMBLY);
        action = m_menuDebug->actions().at(3);
        action->setText(TXT_MEMORY);
        action = m_menuDebug->actions().at(4);
        action->setText(TXT_TIMERS);
        action = m_menuDebug->actions().at(5);
        action->setText(TXT_BREAKPOINTS);
        action = m_menuDebug->actions().at(6);
        action->setText(TXT_WATCHPOINTS);
        action = m_menuDebug->actions().at(7);
        action->setText(TXT_PORTMON);
        action = m_menuDebug->actions().at(8);
        action->setText(TXT_OS_VIEW);
        action = m_menuDebug->actions().at(9);
        action->setText(TXT_OS_STACKS);
        action = m_menuDebug->actions().at(10);
        action->setText(TXT_MISC);
        action = m_menuDebug->actions().at(11);
        action->setText(TXT_AUTOTESTER);

#ifdef _WIN32
        actionToggleConsole->setText(TXT_TOGGLE_CONSOLE);
#endif
    }
}

void MainWindow::changeEvent(QEvent* event) {
    const auto eventType = event->type();
    if (eventType == QEvent::LanguageChange) {
        if (m_setup) {
            ui->retranslateUi(this);
            translateExtras(TRANSLATE_UPDATE);
        }
    } else if (eventType == QEvent::LocaleChange) {
        translateSwitch(QLocale::system());
    }
    QMainWindow::changeEvent(event);
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
        child = childAt({pos.x(), tabBar->mapTo(this, {}).y() - 1});
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

void MainWindow::semiExclusiveButtonPressed() {
    auto button = static_cast<QAbstractButton *>(sender());
    auto group = button->group();
    group->setExclusive(group->checkedButton() != button);
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
        setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));
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
        QMessageBox *info = new QMessageBox;
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

    translateSwitch(m_config->value(SETTING_PREFERRED_LANG).toLocale());
    translateExtras(TRANSLATE_UPDATE);
#ifdef HAS_LIBUSB
    m_usbConnectGroup = new QButtonGroup(this);
    if (!libusb_init(&usb.ctx)) {
        if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
            connect(this, &MainWindow::usbHotplug, this, &MainWindow::usbUpdate);
            libusb_hotplug_register_callback(usb.ctx,
                                             libusb_hotplug_event(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
                                                                  LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
                                             LIBUSB_HOTPLUG_ENUMERATE,
                                             LIBUSB_HOTPLUG_MATCH_ANY,
                                             LIBUSB_HOTPLUG_MATCH_ANY,
                                             LIBUSB_HOTPLUG_MATCH_ANY,
                                             MainWindow::usbHotplugCallback, this, nullptr);
        } else {
            usbRefresh();
        }
        connect(ui->buttonRefreshUSB, &QPushButton::clicked, this, &MainWindow::usbRefresh);
    }
#endif

    optSend(opts);
    if (opts.speed != -1) {
        setEmuSpeed(opts.speed);
    }

    if (m_portableActivated) {
        QMessageBox *info = new QMessageBox;
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
    int retry = 200;
    do {
        if (sendKey(key)) {
            break;
        }
        guiDelay(50);
    } while(retry--);
}

void MainWindow::sendEmuLetterKey(char letter) {
    int retry = 200;
    do {
        if (sendLetterKeyPress(letter)) {
            break;
        }
        guiDelay(50);
    } while(retry--);
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

    setThrottle(o.useUnthrottled ? Qt::Unchecked : Qt::Checked);
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
    if (!m_shutdown) {
        m_shutdown = true;

        com.idClose();

        if (!m_initPassed) {
            QMainWindow::closeEvent(e);
            return;
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

    guiEmuValid = false;
    emu.stop();
    QMainWindow::closeEvent(e);
}

void MainWindow::console(const QString &str, const QColor &colorFg, const QColor &colorBg, int type) {
    if (m_nativeConsole) {
        fputs(str.toStdString().c_str(), type == EmuThread::ConsoleErr ? stderr : stdout);
    } else {
        if (type == EmuThread::ConsoleNorm) {
            m_consoleFormat.setFontWeight(QFont::Normal);
        } else {
            m_consoleFormat.setFontWeight(QFont::Black);
        }
        m_consoleFormat.setBackground(colorBg);
        m_consoleFormat.setForeground(colorFg);
        QTextCursor cur(ui->console->document());
        cur.movePosition(QTextCursor::End);
        cur.insertText(str, m_consoleFormat);
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
        static QColor sColorFg = ui->console->palette().color(QPalette::Text);
        static QColor sColorBg = ui->console->palette().color(QPalette::Base);
        const char *tok;
        const QColor lookup[8] = { Qt::black, Qt::red, Qt::green, Qt::yellow, Qt::blue, Qt::magenta, Qt::cyan, Qt::white };

        QColor colorFg = sColorFg;
        QColor colorBg = sColorBg;
        if ((tok = static_cast<const char*>(memchr(str, '\x1B', static_cast<size_t>(size))))) {
            if (tok != str) {
                console(QString::fromUtf8(str, static_cast<int>(tok - str)), sColorFg, sColorBg, type);
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
                                    colorBg = ui->console->palette().color(QPalette::Base);
                                    colorFg = ui->console->palette().color(QPalette::Text);
                                    break;
                                case '3':
                                    state = CONSOLE_FGCOLOR;
                                    break;
                                case '4':
                                    state = CONSOLE_BGCOLOR;
                                    break;
                                default:
                                    state = CONSOLE_ESC;
                                    sColorBg = ui->console->palette().color(QPalette::Base);
                                    sColorFg = ui->console->palette().color(QPalette::Text);
                                    break;
                            }
                            break;
                        case CONSOLE_FGCOLOR:
                            if (x >= '0' && x <= '7') {
                                state = CONSOLE_ENDVAL;
                                colorFg = lookup[x - '0'];
                            } else {
                                state = CONSOLE_ESC;
                            }
                            break;
                        case CONSOLE_BGCOLOR:
                            if (x >= '0' && x <= '7') {
                                state = CONSOLE_ENDVAL;
                                colorBg = lookup[x - '0'];
                            } else {
                                state = CONSOLE_ESC;
                            }
                            break;
                        case CONSOLE_ENDVAL:
                            if (x == ';') {
                                state = CONSOLE_PARSE;
                            } else if (x == 'm') {
                                sColorBg = colorBg;
                                sColorFg = colorFg;
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
                        console(QString::fromUtf8(tok, static_cast<int>(tokn - tok)), sColorFg, sColorBg, type);
                        size -= tokn - tok;
                    } else {
                        console(QString::fromUtf8(tok, static_cast<int>(size)), sColorFg, sColorBg, type);
                    }
                    tok = tokn;
                } else {
                    tok = nullptr;
                }
            } while (tok);
        } else {
            console(QString::fromUtf8(str, size), sColorFg, sColorBg, type);
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
            console(QStringLiteral("[CEmu] Error clearing console\n"));
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

void MainWindow::showEmuSpeed(int speed) {
    if (m_timerEmuTriggered) {
        m_speedLabel.setText(QStringLiteral("  ") + tr("Emulated Speed: ") + QString::number(speed) + QStringLiteral("%"));
        m_timerEmuTriggered = !m_timerEmuTriggerable;
    }
}

void MainWindow::showFpsSpeed(double emuFps, double guiFps) {
    static double guiFpsPrev = 0;
    static double emuFpsPrev = 0;
    if (emuFps < emuFpsPrev - 1 || emuFps > emuFpsPrev + 1) {
        ui->maxFps->setText(tr("Actual FPS: ") + QString::number(emuFps, 'f', 2));
        emuFpsPrev = emuFps;
    }
    if (guiFps < guiFpsPrev - 1 || guiFps > guiFpsPrev + 1) {
        m_fpsLabel.setText("FPS: " + QString::number(guiFps, 'f', 2));
        guiFpsPrev = guiFps;
    }
}

void MainWindow::lcdUpdate(double emuFps) {
    double guiFps = ui->lcd->refresh();
    if (m_timerFpsTriggered) {
        showFpsSpeed(emuFps, guiFps);
        m_timerFpsTriggered = !m_timerFpsTriggerable;
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
    RomSelection *romWizard = new RomSelection;
    romWizard->setWindowModality(Qt::NonModal);
    romWizard->exec();

    const QString path = romWizard->getRomPath();

    delete romWizard;

    if (path.isEmpty()) {
        m_initPassed = false;
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
    m_dir = dialog.directory().absolutePath();
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

    if (guiDebug || guiReceive || guiSend) {
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

    RecordingThread *thread = new RecordingThread;
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
    connect(buttonUpdateCheck, &QAbstractButton::clicked, this, [=](){ this->checkUpdate(true); });

    QAbstractButton *buttonCopyVersion = aboutBox->addButton(tr("Copy version"), QMessageBox::ActionRole);
    // Needed to prevent the button from closing the dialog
    buttonCopyVersion->disconnect();
    connect(buttonCopyVersion, &QAbstractButton::clicked, this, [=](){ QApplication::clipboard()->setText("CEmu " CEMU_VERSION " (git: " CEMU_GIT_SHA ")", QClipboard::Clipboard); buttonCopyVersion->setEnabled(false); buttonCopyVersion->setText("Version copied!"); });

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
                              QStringLiteral("Matt Waltz (<a href='https://github.com/mateoconlechuga'>MateoConLechuga</a>)<br>"
                                             "Jacob Young (<a href='https://github.com/jacobly0'>jacobly0</a>)<br>"
                                             "Adrien Bertrand (<a href='https://github.com/adriweb'>adriweb</a>)<br>"),
                              QStringLiteral("Zachary Wassall (<a href='https://github.com/runer112'>Runer112</a>)<br>"
                                             "Albert Huang (<a href='https://github.com/alberthdev'>alberthdev</a>)<br>"
                                             "Lionel Debroux (<a href='https://github.com/debrouxl'>debrouxl</a>)<br>"
                                             "Fabian Vogt (<a href='https://github.com/Vogtinator'>Vogtinator</a>)<br>"),
                              QStringLiteral("Matt Waltz (ES), Adrien Bertrand (FR), Stephan Paternotte &amp; Peter Tillema (NL)<br>"),
                              QStringLiteral("<a href='https://github.com/KnightOS/z80e'>z80e</a>, "
                                             "<a href='https://github.com/nspire-emus/firebird'>Firebird Emu</a>, "
                                             "<a href='https://github.com/debrouxl/tilibs'>tilibs</a>, "
                                             "<a href='https://github.com/adriweb/tivars_lib_cpp'>tivars_lib_cpp</a>."),
                              QStringLiteral("<a href='http://www.fatcow.com/free-icons'>FatCow's 'Farm-Fresh Web Icons'</a>"),
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
        console(tr("[CEmu] Dock output redirected to stdout. Use the radio button to enable dock."),
                ui->console->palette().color(QPalette::Text),
                ui->console->palette().color(QPalette::Base));
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

void MainWindow::varToggle() {
    if (guiReceive) {
        varShow();
        emu.unblock();
    } else {
        emu.receive();
    }
}

void MainWindow::varSelect() {
    if (guiDebug) {
       return;
    }

    QStringList fileNames = varDialog(QFileDialog::AcceptOpen, tr("TI Variable (*.8xp *.8xv *.8xl *.8xn *.8xm *.8xy *.8xg *.8xs *.8xd *.8xw *.8xc *.8xl *.8xz *.8xt *.8ca *.8cg *.8ci *.8ek *.b84 *.b83);;All Files (*.*)"), Q_NULLPTR);

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

    m_dir = dialog.directory().absolutePath();

    if (good) {
        return dialog.selectedFiles();
    }

    return QStringList();
}

void MainWindow::varPressed(QTableWidgetItem *item) {
    calc_var_t var = ui->emuVarView->item(item->row(), VAR_NAME_COL)->data(Qt::UserRole).value<calc_var_t>();
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
        codePopup->setVariableName(ui->emuVarView->item(item->row(), VAR_NAME_COL)->text());
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
            varShow();
            break;
    }
}

void MainWindow::varShow() {
    calc_var_t var;

    if (guiSend || guiDebug) {
        return;
    }

    guiReceive = !guiReceive;

    ui->emuVarView->setRowCount(0);
    ui->emuVarView->setSortingEnabled(false);

    if (!guiReceive) {
        ui->buttonRefreshList->setText(tr("View Calculator Variables"));
        ui->buttonReceiveFiles->setEnabled(false);
        ui->buttonReceiveFile->setEnabled(false);
        ui->buttonRun->setEnabled(true);
        ui->buttonSend->setEnabled(true);
        ui->emuVarView->setEnabled(false);
        ui->buttonResendFiles->setEnabled(true);
    } else {
        ui->buttonRefreshList->setText(tr("Resume emulation"));
        ui->buttonSend->setEnabled(false);
        ui->buttonReceiveFiles->setEnabled(true);
        ui->buttonReceiveFile->setEnabled(true);
        ui->buttonResendFiles->setEnabled(false);
        ui->buttonRun->setEnabled(false);
        ui->emuVarView->setEnabled(true);

        vat_search_init(&var);
        while (vat_search_next(&var)) {
            if (var.named || var.size > 2) {
                int row;

                row = ui->emuVarView->rowCount();
                ui->emuVarView->setRowCount(row + 1);

                bool var_preview_needs_gray = false;
                QString var_value;
                if (var.size <= 2) {
                    var_value = tr("Empty");
                    var_preview_needs_gray = true;
                } else if (calc_var_is_asmprog(&var)) {
                    var_value = tr("Can't preview this");
                    var_preview_needs_gray = true;
                } else if (calc_var_is_internal(&var) && var.name[0] != '#') { // # is previewable
                    var_value = tr("Can't preview this OS variable");
                    var_preview_needs_gray = true;
                } else {
                    try {
                        var_value = QString::fromStdString(calc_var_content_string(var)).trimmed().replace("\n", " \\ ");
                        if (var_value.size() > 50) {
                            var_value.truncate(50);
                            var_value += QStringLiteral(" [...]");
                        }
                    } catch(...) {
                        var_value = tr("Can't preview this");
                        var_preview_needs_gray = true;
                    }
                }

                // Do not translate - things rely on those names.
                QString var_type_str = calc_var_type_names[var.type];
                if (calc_var_is_asmprog(&var)) {
                    var_type_str += QStringLiteral(" (ASM)");
                }

                QTableWidgetItem *var_name = new QTableWidgetItem(calc_var_name_to_utf8(var.name));
                QTableWidgetItem *var_location = new QTableWidgetItem(var.archived ? tr("Archive") : QStringLiteral("RAM"));
                QTableWidgetItem *var_type = new QTableWidgetItem(var_type_str);
                QTableWidgetItem *var_preview = new QTableWidgetItem(var_value);
                QTableWidgetItem *var_size = new QTableWidgetItem;

                // Attach var index (hidden) to the name. Needed elsewhere
                var_name->setData(Qt::UserRole, QVariant::fromValue(var));
                var_size->setData(Qt::DisplayRole, var.size);

                var_name->setCheckState(Qt::Unchecked);

                if (var_preview_needs_gray) {
                    var_preview->setFont(varPreviewItalicFont);
                    var_preview->setForeground(Qt::gray);
                } else {
                    var_preview->setFont(varPreviewCEFont);
                }

                ui->emuVarView->setItem(row, VAR_NAME_COL, var_name);
                ui->emuVarView->setItem(row, VAR_LOCATION_COL, var_location);
                ui->emuVarView->setItem(row, VAR_TYPE_COL, var_type);
                ui->emuVarView->setItem(row, VAR_SIZE_COL, var_size);
                ui->emuVarView->setItem(row, VAR_PREVIEW_COL, var_preview);
            }
        }
        ui->emuVarView->resizeColumnToContents(VAR_NAME_COL);
        ui->emuVarView->resizeColumnToContents(VAR_LOCATION_COL);
        ui->emuVarView->resizeColumnToContents(VAR_TYPE_COL);
        ui->emuVarView->resizeColumnToContents(VAR_SIZE_COL);
    }

    ui->emuVarView->setSortingEnabled(true);
}

void MainWindow::varSaveSelected() {
    QVector<calc_var_t> selectedVars;
    QStringList fileNames;
    for (int currRow = 0; currRow < ui->emuVarView->rowCount(); currRow++) {
        if (ui->emuVarView->item(currRow, VAR_NAME_COL)->checkState() == Qt::Checked) {
            selectedVars.append(ui->emuVarView->item(currRow, VAR_NAME_COL)->data(Qt::UserRole).value<calc_var_t>());
        }
    }
    if (selectedVars.size() < 2) {
        QMessageBox::warning(this, MSG_WARNING, tr("Select at least two files to group"));
    } else {
         fileNames = varDialog(QFileDialog::AcceptSave, tr("TI Group (*.8cg);;All Files (*.*)"), QStringLiteral("8cg"));
        if (fileNames.size() == 1) {
            if (emu_receive_variable(fileNames.first().toUtf8(), selectedVars.constData(), selectedVars.size()) != LINK_GOOD) {
                QMessageBox::critical(this, MSG_ERROR, tr("Transfer error, see console for information:\nFile: ") + fileNames.first());
            } else {
                QMessageBox::information(this, MSG_INFORMATION, tr("Transfer completed successfully."));
            }
        }
    }
}

void MainWindow::varSaveSelectedFiles() {
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::DirectoryOnly);
    dialog.setOption(QFileDialog::ShowDirsOnly, false);

    dialog.setDirectory(m_dir);
    int good = 0;

    for (int currRow = 0; currRow < ui->emuVarView->rowCount(); currRow++) {
        if (ui->emuVarView->item(currRow, VAR_NAME_COL)->checkState() == Qt::Checked) {
            good = 1;
            break;
        }
    }

    if (!good) {
        QMessageBox::warning(this, MSG_WARNING, tr("Select at least one file to transfer"));
        return;
    }

    good = dialog.exec();
    m_dir = dialog.directory().absolutePath();

    if (!good) {
        return;
    }

    QString name;
    QString filename;

    for (int currRow = 0; currRow < ui->emuVarView->rowCount(); currRow++) {
        if (ui->emuVarView->item(currRow, VAR_NAME_COL)->checkState() == Qt::Checked) {
            calc_var_t var = ui->emuVarView->item(currRow, VAR_NAME_COL)->data(Qt::UserRole).value<calc_var_t>();

            name = QString(calc_var_name_to_utf8(var.name));
            filename = dialog.directory().absolutePath() + "/" + name + "." + m_varExtensions[var.type1];

            if (emu_receive_variable(filename.toStdString().c_str(), &var, 1) != LINK_GOOD) {
                good = 0;
                break;
            }
        }
    }

    if (good) {
        QMessageBox::information(this, MSG_INFORMATION, tr("Transfer completed successfully."));
    } else {
        QMessageBox::critical(this, MSG_ERROR, tr("Transfer error, see console for information:\nFile: ") + filename);
    }
}

void MainWindow::varResend() {
    sendingHandler->resendSelected();
}

// ------------------------------------------------
// Autotester things
// ------------------------------------------------

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
    m_dir = dialog.directory().absolutePath();

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
        sprintf(buf, "0x%X", mapIdConsts[comboBoxIndex-1].first);
        ui->startCRC->setText(buf);
        sprintf(buf, "0x%X", mapIdConsts[comboBoxIndex-1].second);
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
                console(QStringLiteral("[CEmu] Failed loading image, falling back to ROM.\n"));
                emu.load(EMU_DATA_ROM, m_pathRom);
            }
            break;
    }

    if (state == EMU_STATE_VALID) {
        ui->lcd->setMain();
        setCalcSkinTopFromType();
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
        QString highlighted;

        if (useLabel) {
            if (disasm.base > 511 || (disasm.base < 512 && sit->second[0] == '_')) {
                line = QString(QStringLiteral("<pre><b><font color='#444'>%1</font></b>     %2</pre>"))
                                        .arg(int2hex(static_cast<uint32_t>(disasm.base), 6),
                                             QString::fromStdString(disasm.bold_sym ? "<b>" + sit->second + "</b>" : sit->second) + ":");

                m_disasm->appendHtml(line);
            }

            if (numLines == j + 1) {
                useLabel = false;
            }
            sit++;
        } else {
            symbols = QString(QStringLiteral("<b><font color='#008000'>%1</font><font color='#808000'>%2</font><font color='#800000'>%3</font></b>"))
                             .arg((disasm.highlight.watchR  ? QStringLiteral("R") : QStringLiteral(" ")),
                                  (disasm.highlight.watchW ? QStringLiteral("W") : QStringLiteral(" ")),
                                  (disasm.highlight.breakP  ? QStringLiteral("X") : QStringLiteral(" ")));

            highlighted = QString::fromStdString(disasm.instr.operands)
                                  .replace(QRegularExpression(QStringLiteral("(\\$[0-9a-fA-F]+)")), QStringLiteral("<font color='green'>\\1</font>")) // hex numbers
                                  .replace(QRegularExpression(QStringLiteral("(^\\d)")), QStringLiteral("<font color='blue'>\\1</font>"))             // dec number
                                  .replace(QRegularExpression(QStringLiteral("([()])")), QStringLiteral("<font color='#600'>\\1</font>"));            // parentheses

            line = QString(QStringLiteral("<pre><b><font color='#444'>%1</font></b> %2 %3  <font color='%4'>%5</font> %6</pre>"))
                           .arg(disasm.addr ? int2hex(static_cast<uint32_t>(disasm.base), 6) : QString(),
                                symbols,
                                disasm.bytes ? QString::fromStdString(disasm.instr.data).leftJustified(12, ' ') : QStringLiteral(" "),
                                m_disasmOpcodeColor,
                                QString::fromStdString(disasm.instr.opcode),
                                highlighted);

            m_disasm->appendHtml(line);
        }
    }

    if (!m_disasmOffsetSet && disasm.next > m_disasmAddr) {
        m_disasmOffsetSet = true;
        m_disasmOffset = m_disasm->textCursor();
        m_disasmOffset.movePosition(QTextCursor::StartOfLine);
    }

    if (disasm.highlight.pc == true) {
        m_disasm->addHighlight(QColor(Qt::blue).lighter(160));
    }
}

void MainWindow::contextDisasm(const QPoint &posa) {
    QString setPc = tr("Set PC");
    QString toggleBreak = tr("Toggle Breakpoint");
    QString toggleWrite = tr("Toggle Write Watchpoint");
    QString toggleRead = tr("Toggle Read Watchpoint");
    QString toggleRw = tr("Toggle Read/Write Watchpoint");
    QString runUntilStr = tr("Run Until");
    QString gotoMem = tr("Goto Memory View");

    m_disasm->setTextCursor(m_disasm->cursorForPosition(posa));
    QPoint globalPos = m_disasm->mapToGlobal(posa);
    QString addrStr = m_disasm->getSelectedAddr();
    uint32_t addr = static_cast<uint32_t>(hex2int(addrStr));

    QMenu menu;
    menu.addAction(runUntilStr);
    menu.addSeparator();
    menu.addAction(toggleBreak);
    menu.addAction(toggleRead);
    menu.addAction(toggleWrite);
    menu.addAction(toggleRw);
    menu.addSeparator();
    menu.addAction(gotoMem);
    menu.addAction(setPc);

    QAction *item = menu.exec(globalPos);
    if (item) {
        if (item->text() == setPc) {
            ui->pcregView->setText(addrStr);
            debug_set_pc(addr);
            disasmUpdateAddr(static_cast<int>(cpu.registers.PC), true);
        } else if (item->text() == toggleBreak) {
            breakAddGui();
            memUpdate();
        } else if (item->text() == toggleRead) {
            watchAddGuiR();
            memUpdate();
        } else if (item->text() == toggleWrite) {
            watchAddGuiW();
            memUpdate();
        } else if (item->text() == toggleRw) {
            watchAddGuiRW();
            memUpdate();
        } else if (item->text() == runUntilStr) {
            runUntil(addr);
        } else if (item->text() == gotoMem) {
            gotoMemAddr(addr);
        }
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
    int row = ui->emuVarView->rowAt(posa.y());
    if (row < 0) {
        return;
    }

    const calc_var_t var = ui->emuVarView->item(row, VAR_NAME_COL)->data(Qt::UserRole).value<calc_var_t>();

    QString launch = tr("Launch program");

    QMenu contextMenu;
    if (calc_var_is_prog(&var) && !calc_var_is_internal(&var)) {
        contextMenu.addAction(launch);
    }

    QAction *selectedItem = contextMenu.exec(ui->emuVarView->mapToGlobal(posa));
    if (selectedItem) {
        if (selectedItem->text() == launch) {
            varToggle();
            varLaunch(&var);
        }
    }
}

void MainWindow::contextConsole(const QPoint &posa) {
    bool ok = true;

    QString gotoMem = tr("Goto Memory View");
    QString gotoDisasm = tr("Goto Disassembly View");
    QString toggleBreak = tr("Toggle Breakpoint");
    QString toggleWrite = tr("Toggle Write Watchpoint");
    QString toggleRead = tr("Toggle Read Watchpoint");
    QString toggleRw = tr("Toggle Read/Write Watchpoint");
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
        menu.addAction(gotoMem);
        menu.addAction(gotoDisasm);
        menu.addSeparator();
        menu.addAction(toggleBreak);
        menu.addAction(toggleRead);
        menu.addAction(toggleWrite);
        menu.addAction(toggleRw);

        QAction *item = menu.exec(globalp);
        if (item) {
            if (item->text() == gotoMem) {
                debugForce();
                gotoMemAddr(address);
            } else if (item->text() == gotoDisasm) {
                debugForce();
                gotoDisasmAddr(address);
            } else if (item->text() == toggleBreak) {
                breakAdd(breakNextLabel(), address, true, true, false);
            } else if (item->text() == toggleRead) {
                watchAdd(watchNextLabel(), address, address, DBG_MASK_READ, true, false);
            } else if (item->text() == toggleWrite) {
                watchAdd(watchNextLabel(), address, address, DBG_MASK_WRITE, true, false);
            } else if (item->text() == toggleRw) {
                watchAdd(watchNextLabel(), address, address, DBG_MASK_READ | DBG_MASK_WRITE, true, false);
            }
            memDocksUpdate();
        }
    }
}

// ------------------------------------------------
// USB things
// ------------------------------------------------

#ifdef HAS_LIBUSB
int LIBUSB_CALL MainWindow::usbHotplugCallback(libusb_context *context, libusb_device *device,
                                               libusb_hotplug_event event, void *user_data) {
    (void)context;
    if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED ||
        event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
        emit static_cast<MainWindow *>(user_data)->
            usbHotplug(device, event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED);
    }
    return 0;
}

void MainWindow::usbUpdate(void *opaqueDevice, bool attached) {
    int row;
    for (row = 0; row != ui->usbTable->rowCount(); ++row) {
        if (ui->usbTable->item(row, USB_CONNECT)->data(Qt::UserRole).value<void *>() == opaqueDevice) {
            break;
        }
    }
    libusb_device *device = static_cast<libusb_device *>(opaqueDevice);
    if (attached) {
        if (row != ui->usbTable->rowCount()) {
            return;
        }

        libusb_device_handle *handle;
        if (libusb_open(device, &handle)) {
            return;
        }

        libusb_device_descriptor desc;
        libusb_get_device_descriptor(device, &desc);

        unsigned char string[256];
        quint16 langid = 0x0409; {
            int size = libusb_get_string_descriptor(handle, 0, 0, string, sizeof(string));
            for (int i = 2; i < size; i += 2) {
                quint16 cur = string[i + 1] << 8 | string[i];
                if (cur == m_usbLangID) {
                    langid = cur;
                    break;
                }
                if (i == 2 || !((cur ^ m_usbLangID) & 0x3ff)) {
                    langid = cur;
                }
            }
        }

        ui->usbTable->setSortingEnabled(false);
        int row = ui->usbTable->rowCount();
        ui->usbTable->setRowCount(row + 1);

        QToolButton *btnConnect = new QToolButton;
        btnConnect->setCheckable(true);
        QIcon btnIcon(QStringLiteral(":/icons/resources/icons/disconnect.png"));
        btnIcon.addFile(QStringLiteral(":/icons/resources/icons/connect.png"), QSize(), QIcon::Normal, QIcon::On);
        btnConnect->setIcon(btnIcon);
        connect(btnConnect, &QToolButton::clicked, [this, device](bool checked) {
                                                       emu.setUsbDevice(checked ? device : nullptr);
                                                   });
        m_usbConnectGroup->addButton(btnConnect);
        connect(btnConnect, &QAbstractButton::pressed, this, &MainWindow::semiExclusiveButtonPressed);
        ui->usbTable->setCellWidget(row, USB_CONNECT, btnConnect);
        QTableWidgetItem *item = new QTableWidgetItem;
        libusb_ref_device(device);
        item->setData(Qt::UserRole, QVariant::fromValue(opaqueDevice));
        ui->usbTable->setItem(row, USB_CONNECT, item);

        for (int i = USB_VID; i <= USB_PID; i++) {
            item = new QTableWidgetItem(int2hex((&desc.idVendor)[i - USB_VID], 4));
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            ui->usbTable->setItem(row, i, item);
        }

        for (int i = USB_MANUFACTURER; i <= USB_SERIAL_NUMBER; i++) {
            if (uint8_t index = (&desc.iManufacturer)[i - USB_MANUFACTURER]) {
                int size = libusb_get_string_descriptor(handle, index, langid, reinterpret_cast<unsigned char *>(string), sizeof(string));
                item = new QTableWidgetItem;
                if (size >= 4) {
                    string[0] = QChar::ByteOrderMark & 0xFF; string[1] = QChar::ByteOrderMark >> 8;
                    item->setText(QString::fromUtf16(reinterpret_cast<char16_t *>(string), size >> 1));
                } else {
                    item->setText("N/A");
                    item->setTextColor(Qt::darkGray);
                }
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                ui->usbTable->setItem(row, i, item);
            }
        }
        ui->usbTable->setSortingEnabled(true);
        ui->usbTable->resizeColumnsToContents();

        libusb_close(handle);
    } else if (row != ui->usbTable->rowCount()) {
        if (static_cast<QAbstractButton *>(ui->usbTable->cellWidget(row, USB_CONNECT))->isChecked()) {
            emu.setUsbDevice(nullptr);
        }
        ui->usbTable->removeRow(row);
        ui->usbTable->resizeColumnsToContents();
        libusb_unref_device(device);
    }
}

void MainWindow::usbRefresh() {
    emu.setUsbDevice(nullptr);
    for (int row = 0; row != ui->usbTable->rowCount(); ++row) {
        libusb_unref_device(static_cast<libusb_device *>(ui->usbTable->item(row, USB_CONNECT)->data(Qt::UserRole).value<void *>()));
    }
    ui->usbTable->setRowCount(0);
    libusb_device **devices;
    if (libusb_get_device_list(usb.ctx, &devices) < 0) {
        return;
    }
    for (libusb_device **device = devices; device && *device; device++) {
        usbUpdate(*device);
    }
    libusb_free_device_list(devices, 0);
}
#endif

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
           << opts.launchPrgm;

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
           >> o.launchPrgm;

    optLoadFiles(o);
    optAttemptLoad(o);
    optSend(o);
    if (o.speed != -1) {
        setEmuSpeed(o.speed);
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
                    QDataStream stream(&byteArray, QIODevice::WriteOnly);
                    stream.setVersion(QDataStream::Qt_5_5);
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
           console(QStringLiteral("[CEmu] IPC Unknown\n"));
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

    QToolButton *btnLoad   = new QToolButton;
    QToolButton *btnSave   = new QToolButton;
    QToolButton *btnEdit   = new QToolButton;
    QToolButton *btnRemove = new QToolButton;

    btnLoad->setIcon(m_iconLoad);
    btnSave->setIcon(m_iconSave);
    btnEdit->setIcon(m_iconEdit);
    btnRemove->setIcon(m_iconRemove);

    connect(btnRemove, &QToolButton::clicked, this, &MainWindow::stateRemove);
    connect(btnLoad, &QToolButton::clicked, this, &MainWindow::stateLoad);
    connect(btnSave, &QToolButton::clicked, this, &MainWindow::stateSave);
    connect(btnEdit, &QToolButton::clicked, this, &MainWindow::stateEdit);

    QTableWidgetItem *itemName   = new QTableWidgetItem(name);
    QTableWidgetItem *itemLoad   = new QTableWidgetItem;
    QTableWidgetItem *itemSave   = new QTableWidgetItem;
    QTableWidgetItem *itemEdit   = new QTableWidgetItem;
    QTableWidgetItem *itemRemove = new QTableWidgetItem;

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
