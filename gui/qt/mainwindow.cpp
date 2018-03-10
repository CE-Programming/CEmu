#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils.h"
#include "sendinghandler.h"
#include "memoryvisualizer.h"
#include "dockwidget.h"
#include "searchwidget.h"
#include "basiccodeviewerwindow.h"
#include "capture/animated-png.h"
#include "tivarslib/TIModels.h"
#include "tivarslib/TIVarTypes.h"
#include "tivarslib/TypeHandlers/TypeHandlers.h"
#include "../../core/emu.h"
#include "../../core/asic.h"
#include "../../core/cpu.h"
#include "../../core/misc.h"
#include "../../core/mem.h"
#include "../../core/interrupt.h"
#include "../../core/keypad.h"
#include "../../core/control.h"
#include "../../core/flash.h"
#include "../../core/lcd.h"
#include "../../core/spi.h"
#include "../../core/backlight.h"
#include "../../core/timers.h"
#include "../../core/usb.h"
#include "../../core/realclock.h"
#include "../../core/sha256.h"
#include "../../core/link.h"
#include "../../tests/autotester/crc32.hpp"
#include "../../tests/autotester/autotester.h"

#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QBuffer>
#include <QtCore/QProcess>
#include <QtGui/QWindow>
#include <QtGui/QDesktopServices>
#include <QtGui/QClipboard>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QProgressDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QScrollBar>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <fstream>
#include <iostream>

#ifdef _MSC_VER
    #include <direct.h>
    #define chdir _chdir
#else
    #include <unistd.h>
#endif

MainWindow::MainWindow(CEmuOpts &cliOpts, QWidget *p) : QMainWindow(p), ui(new Ui::MainWindow), opts(cliOpts) {

    // Setup translations
    m_appTranslator.load(QLocale::system().name(), QStringLiteral(":/i18n/i18n/"));
    qApp->installTranslator(&m_appTranslator);

    // Register setting metatypes
    qRegisterMetaTypeStreamOperators<QList<int>>("QList<int>");
    qRegisterMetaTypeStreamOperators<QList<bool>>("QList<bool>");

    // Setup the UI
    ui->setupUi(this);

    // init IPC
    if (!ipcSetup()) {
        m_initPassed = false;
        return;
    }

    // Init tivars_lib stuff
    tivars::TIModels::initTIModelsArray();
    tivars::TIVarTypes::initTIVarTypesArray();
    tivars::TH_0x05::initTokens();

    ui->centralWidget->hide();
    ui->statusBar->addWidget(&m_speedLabel);
    ui->statusBar->addPermanentWidget(&m_msgLabel);
    ui->statusBar->addPermanentWidget(&m_fpsLabel);

    // Allow for 2018 lines of logging
    ui->console->setMaximumBlockCount(2018);

    setWindowTitle(QStringLiteral("CEmu | ") + opts.idString);

    // Register QtKeypadBridge for the virtual keyboard functionality
    keypadBridge = new QtKeypadBridge(this);
    connect(keypadBridge, &QtKeypadBridge::keyStateChanged, ui->keypadWidget, &KeypadWidget::changeKeyState);

    installEventFilter(keypadBridge);
    for (const auto &tab : ui->tabWidget->children()[0]->children()) {
        tab->installEventFilter(keypadBridge);
    }

    // Setup the file sending handler
    m_progressBar = new QProgressBar(this);
    ui->statusBar->addWidget(m_progressBar);
    sendingHandler = new SendingHandler(this, m_progressBar, ui->varLoadedView);

    // Emulator -> GUI (Should be queued)
    connect(&emu, &EmuThread::consoleStr, this, &MainWindow::consoleStr, Qt::UniqueConnection);
    connect(&emu, &EmuThread::actualSpeedChanged, this, &MainWindow::showEmuSpeed, Qt::QueuedConnection);
    connect(&emu, &EmuThread::raiseDebugger, this, &MainWindow::debuggerRaise, Qt::QueuedConnection);
    connect(&emu, &EmuThread::disableDebugger, this, &MainWindow::debuggerGUIDisable, Qt::QueuedConnection);
    connect(&emu, &EmuThread::sendDebugCommand, this, &MainWindow::debuggerProcessCommand, Qt::QueuedConnection);
    connect(&emu, &EmuThread::saved, this, &MainWindow::savedEmu, Qt::QueuedConnection);
    connect(&emu, &EmuThread::locked, this, &MainWindow::lockedEmu, Qt::QueuedConnection);

    // Console actions
    connect(ui->buttonConsoleclear, &QPushButton::clicked, ui->console, &QPlainTextEdit::clear);
    connect(ui->radioConsole, &QRadioButton::clicked, this, &MainWindow::consoleOutputChanged);
    connect(ui->radioStderr, &QRadioButton::clicked, this, &MainWindow::consoleOutputChanged);

    // Debugger
    connect(ui->buttonRun, &QPushButton::clicked, this, &MainWindow::debuggerChangeState);
    connect(ui->checkADLDisasm, &QCheckBox::stateChanged, this, &MainWindow::updateDisasm);
    connect(ui->checkADLStack, &QCheckBox::stateChanged, this, &MainWindow::updateStack);
    connect(ui->checkADL, &QCheckBox::stateChanged, [this]{ updateDisasm(); updateStack(); });

    connect(ui->buttonAddPort, &QPushButton::clicked, this, &MainWindow::portSlotAdd);
    connect(ui->buttonAddBreakpoint, &QPushButton::clicked, this, &MainWindow::breakpointSlotAdd);
    connect(ui->buttonAddWatchpoint, &QPushButton::clicked, this, &MainWindow::watchpointSlotAdd);
    connect(ui->buttonStepIn, &QPushButton::clicked, this, &MainWindow::stepIn);
    connect(ui->buttonStepOver, &QPushButton::clicked, this, &MainWindow::stepOver);
    connect(ui->buttonStepNext, &QPushButton::clicked, this, &MainWindow::stepNext);
    connect(ui->buttonStepOut, &QPushButton::clicked, this, &MainWindow::stepOut);
    connect(ui->buttonGoto, &QPushButton::clicked, this, &MainWindow::gotoPressed);
    connect(ui->console, &QWidget::customContextMenuRequested, this, &MainWindow::consoleContextMenu);
    connect(ui->disassemblyView, &QWidget::customContextMenuRequested, this, &MainWindow::disasmContextMenu);
    connect(ui->vatView, &QWidget::customContextMenuRequested, this, &MainWindow::vatContextMenu);
    connect(ui->opView, &QWidget::customContextMenuRequested, this, &MainWindow::opContextMenu);
    connect(ui->portView, &QTableWidget::itemChanged, this, &MainWindow::portDataChanged);
    connect(ui->portView, &QTableWidget::itemPressed, this, &MainWindow::portSetPreviousAddress);
    connect(ui->breakpointView, &QTableWidget::itemChanged, this, &MainWindow::breakpointDataChanged);
    connect(ui->breakpointView, &QTableWidget::itemPressed, this, &MainWindow::breakpointSetPreviousAddress);
    connect(ui->watchpointView, &QTableWidget::itemChanged, this, &MainWindow::watchpointDataChanged);
    connect(ui->watchpointView, &QTableWidget::itemPressed, this, &MainWindow::watchpointSetPreviousAddress);
    connect(ui->checkCharging, &QCheckBox::toggled, this, &MainWindow::batteryIsCharging);
    connect(ui->sliderBattery, &QSlider::valueChanged, this, &MainWindow::batteryChangeStatus);
    connect(ui->checkAddSpace, &QCheckBox::toggled, this, &MainWindow::setSpaceDisasm);
    connect(ui->checkDisableSoftCommands, &QCheckBox::toggled, this, &MainWindow::setDebugSoftCommands);
    connect(ui->buttonZero, &QPushButton::clicked, this, &MainWindow::debuggerZeroClockCounter);
    connect(ui->buttonCertID, &QPushButton::clicked, this, &MainWindow::changeCalcID);
    connect(ui->disassemblyView, &DataWidget::gotoDisasmAddress, this, &MainWindow::gotoDisasmAddr);
    connect(ui->disassemblyView, &DataWidget::gotoMemoryAddress, this, &MainWindow::gotoMemAddr);

    // Ctrl + Click
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

    // Debugger Options
    connect(ui->buttonAddEquateFile, &QToolButton::clicked, this, &MainWindow::equatesAddDialog);
    connect(ui->buttonClearEquates, &QToolButton::clicked, this, &MainWindow::equatesClear);
    connect(ui->buttonRefreshEquates, &QToolButton::clicked, this, &MainWindow::equatesRefresh);
    connect(ui->buttonToggleBreakpoints, &QToolButton::toggled, this, &MainWindow::setDebugIgnoreBreakpoints);
    connect(ui->checkDebugResetTrigger, &QCheckBox::toggled, this, &MainWindow::setDebugResetTrigger);
    connect(ui->checkDataCol, &QCheckBox::toggled, this, &MainWindow::setDataCol);
    connect(ui->checkDma, &QCheckBox::toggled, this, &MainWindow::setLcdDma);
    connect(ui->textSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::setFont);

    // Debugging files
    connect(ui->actionImportDebugger, &QAction::triggered, this, &MainWindow::debuggerImport);
    connect(ui->actionExportDebugger, &QAction::triggered, this, &MainWindow::debuggerExport);

    // Linking
    connect(ui->buttonSend, &QPushButton::clicked, this, &MainWindow::selectFiles);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::selectFiles);
    connect(ui->buttonRefreshList, &QPushButton::clicked, this, &MainWindow::receiveChangeState);
    connect(ui->buttonReceiveFile, &QPushButton::clicked, this, &MainWindow::saveSelectedFile);
    connect(ui->buttonReceiveFiles, &QPushButton::clicked, this, &MainWindow::saveSelectedFiles);
    connect(ui->buttonResendFiles, &QPushButton::clicked, this, &MainWindow::resendFiles);
    connect(ui->buttonRmAllVars, &QPushButton::clicked, this, &MainWindow::removeAllSentVars);
    connect(ui->buttonRmSelectedVars, &QPushButton::clicked, this, &MainWindow::removeSentVars);
    connect(ui->buttonDeselectVars, &QPushButton::clicked, this, &MainWindow::deselectAllVars);
    connect(ui->buttonSelectVars, &QPushButton::clicked, this, &MainWindow::selectAllVars);
    connect(ui->varLoadedView, &QWidget::customContextMenuRequested, this, &MainWindow::resendContextMenu);

    // Autotester
    connect(ui->buttonOpenJSONconfig, &QPushButton::clicked, this, &MainWindow::prepareAndOpenJSONConfig);
    connect(ui->buttonReloadJSONconfig, &QPushButton::clicked, this, &MainWindow::reloadJSONConfig);
    connect(ui->buttonLaunchTest, &QPushButton::clicked, this, &MainWindow::launchTest);
    connect(ui->comboBoxPresetCRC, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::updateCRCParamsFromPreset);
    connect(ui->buttonRefreshCRC, &QPushButton::clicked, this, &MainWindow::refreshCRC);

    // Menubar Actions
    connect(ui->actionSetup, &QAction::triggered, this, &MainWindow::runSetup);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionScreenshot, &QAction::triggered, this, &MainWindow::screenshot);
#ifdef PNG_WRITE_APNG_SUPPORTED
    connect(ui->actionRecordAnimated, &QAction::triggered, this, &MainWindow::recordAPNG);
#endif
    connect(ui->actionSaveState, &QAction::triggered, this, &MainWindow::saveEmu);
    connect(ui->actionExportCalculatorState, &QAction::triggered, this, &MainWindow::saveToFile);
    connect(ui->actionExportRomImage, &QAction::triggered, this, &MainWindow::exportRom);
    connect(ui->actionImportCalculatorState, &QAction::triggered, this, &MainWindow::restoreFromFile);
    connect(ui->actionReloadROM, &QAction::triggered, [this]{ loadEmu(false); });
    connect(ui->actionRestoreState, &QAction::triggered, [this]{ this->loadEmu(true); });
    connect(ui->actionResetALL, &QAction::triggered, this, &MainWindow::resetCEmu);
    connect(ui->actionResetGUI, &QAction::triggered, this, &MainWindow::resetGui);
    connect(ui->actionResetCalculator, &QAction::triggered, this, &MainWindow::resetCalculator);
    connect(ui->actionMemoryVisualizer, &QAction::triggered, this, &MainWindow::newMemoryVisualizer);
    connect(ui->actionDisableMenuBar, &QAction::triggered, this, &MainWindow::setMenuBarState);
    connect(ui->buttonResetCalculator, &QPushButton::clicked, this, &MainWindow::resetCalculator);
    connect(ui->buttonReloadROM, &QPushButton::clicked, this, &MainWindow::resetCalculator);

    // LCD Update
    connect(ui->lcd, &LCDWidget::updateLcd, this, &MainWindow::updateLcd, Qt::QueuedConnection);
    connect(this, &MainWindow::updateMode, ui->lcd, &LCDWidget::setMode);
    connect(this, &MainWindow::updateFrameskip, ui->lcd, &LCDWidget::setFrameskip);
    connect(ui->statusInterval, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::setStatusInterval);

    // Capture
    connect(ui->buttonScreenshot, &QPushButton::clicked, this, &MainWindow::screenshot);
#ifdef PNG_WRITE_APNG_SUPPORTED
    connect(ui->buttonRecordAnimated, &QPushButton::clicked, this, &MainWindow::recordAPNG);
    connect(ui->apngSkip, &QSlider::valueChanged, this, &MainWindow::setFrameskip);
    connect(ui->checkOptimizeRecording, &QCheckBox::stateChanged, this, &MainWindow::setOptimizeRecording);
#else
    ui->actionRecordAnimated->setEnabled(false);
    ui->buttonRecordAnimated->setEnabled(false);
    ui->apngSkip->setEnabled(false);
    ui->checkOptimizeRecording->setEnabled(false);
#endif

    // About
    connect(ui->actionCheckForUpdates, &QAction::triggered, [this]{ checkForUpdates(true); });
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(ui->actionReportBug, &QAction::triggered, []{ QDesktopServices::openUrl(QUrl("https://github.com/CE-Programming/CEmu/issues")); });

    // Other GUI actions
    connect(ui->buttonRunSetup, &QPushButton::clicked, this, &MainWindow::runSetup);
    connect(ui->scaleLCD, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::setLcdScale);
    connect(ui->guiSkip, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::setGuiSkip);
    connect(ui->checkSkin, &QCheckBox::stateChanged, this, &MainWindow::setSkinToggle);
    connect(ui->checkSpi, &QCheckBox::toggled, this, &MainWindow::setLcdSpi);
    connect(ui->checkAlwaysOnTop, &QCheckBox::stateChanged, this, &MainWindow::setAlwaysOnTop);
    connect(ui->emulationSpeed, &QSlider::valueChanged, this, &MainWindow::setEmuSpeed);
    connect(ui->emulationSpeedSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::setEmuSpeed);
    connect(ui->checkThrottle, &QCheckBox::stateChanged, this, &MainWindow::setThrottle);
    connect(ui->checkAutoEquates, &QCheckBox::stateChanged, this, &MainWindow::setAutoEquates);
    connect(ui->checkSaveRestore, &QCheckBox::stateChanged, this, &MainWindow::setAutoSaveState);
    connect(ui->checkPortable, &QCheckBox::stateChanged, this, &MainWindow::setPortableConfig);
    connect(ui->checkSaveRecent, &QCheckBox::stateChanged, this, &MainWindow::setRecentSave);
    connect(ui->checkSaveLoadDebug, &QCheckBox::stateChanged, this, &MainWindow::setSaveDebug);
    connect(ui->buttonChangeSavedImagePath, &QPushButton::clicked, this, &MainWindow::setImagePath);
    connect(ui->buttonChangeSavedDebugPath, &QPushButton::clicked, this, &MainWindow::setDebugPath);
    connect(ui->checkFocus, &QCheckBox::stateChanged, this, &MainWindow::setFocusSetting);
    connect(ui->checkPreI, &QCheckBox::stateChanged, this, &MainWindow::setPreRevisionI);
    connect(ui->flashBytes, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui->flashEdit, &HexWidget::setBytesPerLine);
    connect(ui->ramBytes, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui->ramEdit, &HexWidget::setBytesPerLine);
    connect(ui->ramAscii, &QToolButton::toggled, [this](bool set){ ui->ramEdit->setAsciiArea(set); });
    connect(ui->flashAscii, &QToolButton::toggled, [this](bool set){ ui->flashEdit->setAsciiArea(set); });
    connect(ui->emuVarView, &QTableWidget::itemDoubleClicked, this, &MainWindow::variableDoubleClicked);
    connect(ui->emuVarView, &QTableWidget::customContextMenuRequested, this, &MainWindow::variablesContextMenu);
    connect(ui->buttonAddSlot, &QPushButton::clicked, this, &MainWindow::slotAddNew);
    connect(ui->actionExportCEmuImage, &QAction::triggered, this, &MainWindow::exportCEmuBootImage);
    connect(ui->lcd, &LCDWidget::sendROM, this, &MainWindow::setRom);
    connect(ui->lcd, &LCDWidget::customContextMenuRequested, this, &MainWindow::screenContextMenu);
    connect(ui->consoleLine, &QLineEdit::returnPressed, this, &MainWindow::consoleSubmission);
    connect(ui->consoleSubmit, &QPushButton::clicked, this, &MainWindow::consoleSubmission);

    // Lang switch
    connect(ui->actionEnglish,  &QAction::triggered, [this]{ switchTranslator(QStringLiteral("en_EN")); });
    connect(ui->actionFran_ais, &QAction::triggered, [this]{ switchTranslator(QStringLiteral("fr_FR")); });
    connect(ui->actionDutch,    &QAction::triggered, [this]{ switchTranslator(QStringLiteral("nl_NL")); });
    connect(ui->actionEspanol,  &QAction::triggered, [this]{ switchTranslator(QStringLiteral("es_ES")); });

    // Sending Handler
    connect(sendingHandler, &SendingHandler::send, &emu, &EmuThread::send, Qt::QueuedConnection);
    connect(&emu, &EmuThread::sentFile, sendingHandler, &SendingHandler::sentFile, Qt::QueuedConnection);
    connect(sendingHandler, &SendingHandler::loadEquateFile, this, &MainWindow::equatesAddFile);

    // Hex Editor
    connect(ui->buttonFlashSearch, &QPushButton::clicked, [this]{ memSearchEdit(ui->flashEdit); });
    connect(ui->buttonRamSearch, &QPushButton::clicked, [this]{ memSearchEdit(ui->ramEdit); });
    connect(ui->buttonFlashGoto, &QPushButton::clicked, this, &MainWindow::flashGotoPressed);
    connect(ui->buttonRamGoto, &QPushButton::clicked, this, &MainWindow::ramGotoPressed);
    connect(ui->buttonFlashSync, &QToolButton::clicked, this, &MainWindow::flashSyncPressed);
    connect(ui->buttonRamSync, &QToolButton::clicked, this, &MainWindow::ramSyncPressed);
    connect(ui->flashEdit, &HexWidget::customContextMenuRequested, this, &MainWindow::memContextMenu);
    connect(ui->ramEdit, &HexWidget::customContextMenuRequested, this, &MainWindow::memContextMenu);

    // Keybindings
    connect(ui->radioCEmuKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radioTilEmKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radioWabbitemuKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radiojsTIfiedKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);

    // Keypad Coloring
    connect(ui->buttonTrueBlue, &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonDenim, &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonPink, &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonPlum, &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonRed, &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonLightning, &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonGolden, &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonWhite, &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonBlack, &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonSilver, &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonSpaceGrey, &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonCoral, &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonMint, &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonRoseGold, &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonCrystalClear, &QPushButton::clicked, this, &MainWindow::selectKeypadColor);

    // Export / Import window config
    connect(ui->actionExportWindowConfig, &QAction::triggered, this, &MainWindow::exportWindowConfig);
    connect(ui->actionImportWindowConfig, &QAction::triggered, this, &MainWindow::importWindowConfig);

    // Application connections
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &MainWindow::pauseEmu);

    // Key History Window
    connect(ui->actionKeyHistory, &QAction::triggered, this, &MainWindow::toggleKeyHistory);

    // Auto Updates
    connect(ui->checkUpdates, &QCheckBox::stateChanged, this, &MainWindow::setAutoCheckForUpdates);

    // IPC
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::ipcSpawnRandom);
    connect(ui->buttonChangeID, &QPushButton::clicked, this, &MainWindow::ipcChangeID);
    connect(&com, &InterCom::readDone, this, &MainWindow::ipcReceived);

    // Clipboard copy
    connect(ui->actionClipScreen, &QAction::triggered, this, &MainWindow::saveScreenToClipboard);

    // Docks
    translateExtras(TRANSLATE_INIT);
    m_actionToggleUI = new QAction(MSG_EDIT_UI, this);
    m_actionToggleUI->setCheckable(true);
    connect(m_actionToggleUI, &QAction::triggered, this, &MainWindow::toggleUIEditMode);

    m_actionAddMemory = new QAction(MSG_ADD_MEMORY, this);
    connect(m_actionAddMemory, &QAction::triggered, [this]{ addMemoryDock(randomString(20), 8, true); });

    // Shortcut Connections
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
    m_shortcutStepIn->setAutoRepeat(false);
    m_shortcutStepOver->setAutoRepeat(false);
    m_shortcutStepNext->setAutoRepeat(false);
    m_shortcutStepOut->setAutoRepeat(false);
    m_shortcutAsm->setAutoRepeat(false);
    m_shortcutResend->setAutoRepeat(false);

    connect(m_shortcutFullscreen, &QShortcut::activated, this, &MainWindow::toggleFullscreen);
    connect(m_shortcutResend, &QShortcut::activated, this, &MainWindow::resendFiles);
    connect(m_shortcutAsm, &QShortcut::activated, this, &MainWindow::sendASMKey);
    connect(m_shortcutDebug, &QShortcut::activated, this, &MainWindow::debuggerChangeState);
    connect(m_shortcutStepIn, &QShortcut::activated, this, &MainWindow::stepIn);
    connect(m_shortcutStepOver, &QShortcut::activated, this, &MainWindow::stepOver);
    connect(m_shortcutStepNext, &QShortcut::activated, this, &MainWindow::stepNext);
    connect(m_shortcutStepOut, &QShortcut::activated, this, &MainWindow::stepOut);

    ui->portView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->breakpointView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->watchpointView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    autotester::stepCallback = [](){ qApp->processEvents(); };

    QString portableSettings = qApp->applicationDirPath() + SETTING_DEFAULT_FILE;
    QString localSettings = configPath + SETTING_DEFAULT_FILE;

    if (opts.settingsFile.isEmpty()) {
        if (checkForCEmuBootImage()) {
            m_settingsPath = localSettings;
        } else if (fileExists(portableSettings)) {
            m_settingsPath = portableSettings;
            m_portable = true;
        } else {
            m_settingsPath = localSettings;
        }
    } else {
        m_settingsPath = opts.settingsFile;
    }

    if (opts.useSettings) {
        m_settings = new QSettings(m_settingsPath, QSettings::IniFormat);
    } else {
        m_settings = new QSettings();
        m_settings->clear();
        saveSettings();
    }
    resetSettingsIfLoadedCEmuBootableImage();

    QFileInfo settingsFile(m_settingsPath);
    QFileInfo settingsDirectory(settingsFile.path());
    if(opts.useSettings && settingsDirectory.isDir() && !settingsDirectory.isWritable()){
        m_settingsPath = portableSettings;
        ui->settingsPath->setText(m_settingsPath);
        setPortableConfig(true);
        m_settingsPath = portableSettings;
        ui->settingsPath->setText(m_settingsPath);
        m_activatedPortable = true;
    }

    if (m_portable) {
        ui->checkPortable->setChecked(true);
        ui->buttonChangeSavedDebugPath->setEnabled(false);
        ui->buttonChangeSavedImagePath->setEnabled(false);
    }
    ui->settingsPath->setText(m_settingsPath);

    setAutoCheckForUpdates(m_settings->value(SETTING_AUTOUPDATE, CEMU_RELEASE).toBool());
    checkVersion();

#ifdef Q_OS_WIN
    installToggleConsole();
#endif

#ifdef Q_OS_MACX
    ui->actionDisableMenuBar->setVisible(false);
#endif

    m_iconStop.addPixmap(QPixmap(QStringLiteral(":/icons/resources/icons/stop.png")));
    m_iconRun.addPixmap(QPixmap(QStringLiteral(":/icons/resources/icons/run.png")));
    m_iconSave.addPixmap(QPixmap(QStringLiteral(":/icons/resources/icons/import.png")));
    m_iconLoad.addPixmap(QPixmap(QStringLiteral(":/icons/resources/icons/export.png")));
    m_iconEdit.addPixmap(QPixmap(QStringLiteral(":/icons/resources/icons/wizard.png")));
    m_iconRemove.addPixmap(QPixmap(QStringLiteral(":/icons/resources/icons/exit.png")));
    m_iconSearch.addPixmap(QPixmap(QStringLiteral(":/icons/resources/icons/search.png")));
    m_iconGoto.addPixmap(QPixmap(QStringLiteral(":/icons/resources/icons/goto.png")));
    m_iconSync.addPixmap(QPixmap(QStringLiteral(":/icons/resources/icons/refresh.png")));
    m_iconAddMem.addPixmap(QPixmap(QStringLiteral(":/icons/resources/icons/add_mem.png")));
    m_iconUiEdit.addPixmap(QPixmap(QStringLiteral(":/icons/resources/icons/ui_edit.png")));
    m_iconAscii.addPixmap(QPixmap(QStringLiteral(":/icons/resources/icons/characters.png")));
    m_actionAddMemory->setIcon(m_iconAddMem);
    m_actionToggleUI->setIcon(m_iconUiEdit);

    setMemoryDocks();
    setMemoryState();
    optLoadFiles(opts);
    setFrameskip(m_settings->value(SETTING_CAPTURE_FRAMESKIP, 1).toInt());
    setOptimizeRecording(m_settings->value(SETTING_CAPTURE_OPTIMIZE, true).toBool());
    setStatusInterval(m_settings->value(SETTING_STATUS_INTERVAL, 1).toInt());
    setGuiSkip(m_settings->value(SETTING_SCREEN_FRAMESKIP, 0).toInt());
    setEmuSpeed(m_settings->value(SETTING_EMUSPEED, 100).toInt());
    setFont(m_settings->value(SETTING_DEBUGGER_TEXT_SIZE, 9).toInt());
    setAutoSaveState(m_settings->value(SETTING_RESTORE_ON_OPEN, true).toBool());
    setSaveDebug(m_settings->value(SETTING_DEBUGGER_RESTORE_ON_OPEN, false).toBool());
    setSpaceDisasm(m_settings->value(SETTING_DEBUGGER_ADD_DISASM_SPACE, false).toBool());
    setAutoEquates(m_settings->value(SETTING_DEBUGGER_AUTO_EQUATES, false).toBool());
    setDebugResetTrigger(m_settings->value(SETTING_DEBUGGER_RESET_OPENS, false).toBool());
    setDebugIgnoreBreakpoints(m_settings->value(SETTING_DEBUGGER_BREAK_IGNORE, false).toBool());
    setDebugSoftCommands(m_settings->value(SETTING_DEBUGGER_ENABLE_SOFT, true).toBool());
    setDataCol(m_settings->value(SETTING_DEBUGGER_DATA_COL, true).toBool());
    setLcdSpi(m_settings->value(SETTING_SCREEN_SPI, true).toBool());
    setLcdDma(m_settings->value(SETTING_DEBUGGER_IGNORE_DMA, true).toBool());
    setFocusSetting(m_settings->value(SETTING_PAUSE_FOCUS, false).toBool());
    setRecentSave(m_settings->value(SETTING_RECENT_SAVE, true).toBool());
    setPreRevisionI(m_settings->value(SETTING_DEBUGGER_PRE_I, false).toBool());

    m_dir.setPath((m_settings->value(SETTING_CURRENT_DIR, QDir::homePath()).toString()));
    if (m_settings->value(SETTING_IMAGE_PATH, QStringLiteral("")).toString().isEmpty() || m_portable) {
        QString path = QDir::cleanPath(QFileInfo(m_settings->fileName()).absoluteDir().absolutePath() + SETTING_DEFAULT_IMAGE_FILE);
        m_settings->setValue(SETTING_IMAGE_PATH, path);
    }
    ui->savedImagePath->setText(m_settings->value(SETTING_IMAGE_PATH).toString());
    m_pathImage = ui->savedImagePath->text();

    if (m_settings->value(SETTING_DEBUGGER_IMAGE_PATH, QStringLiteral("")).toString().isEmpty() || m_portable) {
        QString path = QDir::cleanPath(QFileInfo(m_settings->fileName()).absoluteDir().absolutePath() + SETTING_DEFAULT_DEBUG_FILE);
        m_settings->setValue(SETTING_DEBUGGER_IMAGE_PATH, path);
    }
    ui->savedDebugPath->setText(m_settings->value(SETTING_DEBUGGER_IMAGE_PATH).toString());

    QString currKeyMap = m_settings->value(SETTING_KEYPAD_KEYMAP, SETTING_KEYPAD_CEMU).toString();
    if (SETTING_KEYPAD_CEMU.compare(currKeyMap, Qt::CaseInsensitive) == 0) {
        ui->radioCEmuKeys->setChecked(true);
    }
    else if (SETTING_KEYPAD_TILEM.compare(currKeyMap, Qt::CaseInsensitive) == 0) {
        ui->radioTilEmKeys->setChecked(true);
    }
    else if (SETTING_KEYPAD_WABBITEMU.compare(currKeyMap, Qt::CaseInsensitive) == 0) {
        ui->radioWabbitemuKeys->setChecked(true);
    }
    else if (SETTING_KEYPAD_JSTIFIED.compare(currKeyMap, Qt::CaseInsensitive) == 0) {
        ui->radiojsTIfiedKeys->setChecked(true);
    }
    setKeymap(currKeyMap);

    ui->rompathView->setText(m_pathRom);

    debugger_init();

    if (!fileExists(m_pathRom)) {
        if (!runSetup()) {
            m_initPassed = false;
        }
    } else {
        if (opts.useSettings && m_settings->value(SETTING_RESTORE_ON_OPEN, true).toBool()
                && fileExists(m_pathImage)
                && opts.restoreOnOpen) {
            loadEmu(true);
        } else {
            if (opts.forceReloadRom) {
                loadEmu(false);
            }
        }
    }

    m_cBack.setColor(QPalette::Base, QColor(Qt::yellow).lighter(160));
    m_consoleFormat = ui->console->currentCharFormat();
    m_consoleFormat.setForeground(Qt::black);

    debuggerInstall();
}

void MainWindow::switchTranslator(const QString& lang) {
    qApp->removeTranslator(&m_appTranslator);
    // For English, nothing to load after removing the translator.
    if (lang == QStringLiteral("en_EN") || (m_appTranslator.load(lang, QStringLiteral(":/i18n/i18n/"))
                                            && qApp->installTranslator(&m_appTranslator))) {
        m_settings->setValue(SETTING_PREFERRED_LANG, lang);
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
    MSG_ADD_MEMORY = tr("Add Memory View");
    MSG_EDIT_UI = tr("Enable UI edit mode");

    QString __TXT_MEM_DOCK = tr("Memory View");

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
    QString __TXT_BREAK_WATCH = tr("Break / Watch / Port");
    QString __TXT_OS_VIEW = tr("OS View");
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
            if (dock->windowTitle() == TXT_BREAK_WATCH) {
                dock->setWindowTitle(__TXT_BREAK_WATCH);
            }
            if (dock->windowTitle() == TXT_OS_VIEW) {
                dock->setWindowTitle(__TXT_OS_VIEW);
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
    TXT_BREAK_WATCH = __TXT_BREAK_WATCH;
    TXT_OS_VIEW = __TXT_OS_VIEW;
    TXT_MISC = __TXT_MISC;
    TXT_AUTOTESTER = __TXT_AUTOTESTER;

#ifdef _WIN32
    TXT_TOGGLE_CONSOLE = tr("Toggle Windows Console");
#endif

    if (init == TRANSLATE_UPDATE) {
        m_actionToggleUI->setText(MSG_EDIT_UI);
        m_actionAddMemory->setText(MSG_ADD_MEMORY);
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
        action->setText(TXT_BREAK_WATCH);
        action = m_menuDebug->actions().at(6);
        action->setText(TXT_OS_VIEW);
        action = m_menuDebug->actions().at(7);
        action->setText(TXT_MISC);
        action = m_menuDebug->actions().at(8);
        action->setText(TXT_AUTOTESTER);

#ifdef _WIN32
        actionToggleConsole->setText(TXT_TOGGLE_CONSOLE);
#endif
    }
}

void MainWindow::changeEvent(QEvent* event) {
    const auto eventType = event->type();
    if (eventType == QEvent::LanguageChange) {
        if (m_windowVisible) {
            ui->retranslateUi(this);
            translateExtras(TRANSLATE_UPDATE);
        }
    } else if (eventType == QEvent::LocaleChange) {
        switchTranslator(QLocale::system().name());
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::showEvent(QShowEvent *e) {
    if (!m_windowVisible) {
        if (!m_initPassed) {
            QFile(m_settingsPath).remove();
            close();
            e->accept();
            return;
        }
        ui->lcd->setFocus();
        m_progressBar->setMaximumHeight(ui->statusBar->height()/2);
        setLcdScale(m_settings->value(SETTING_SCREEN_SCALE, 100).toUInt());
        setSkinToggle(m_settings->value(SETTING_SCREEN_SKIN, true).toBool());
        setAlwaysOnTop(m_settings->value(SETTING_ALWAYS_ON_TOP, false).toBool());
        setMenuBarState(m_settings->value(SETTING_WINDOW_MENUBAR, false).toBool());
        m_windowVisible = true;
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

void MainWindow::setup() {
    const QByteArray geometry = m_settings->value(SETTING_WINDOW_GEOMETRY, QByteArray()).toByteArray();
    if (geometry.isEmpty()) {
        setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, minimumSize(), qApp->desktop()->availableGeometry()));
        setUIDocks(true);
        setUIEditMode(m_settings->value(SETTING_UI_EDIT_MODE, true).toBool());
    } else {
        restoreGeometry(geometry);
        setUIDocks(false);
        setUIEditMode(m_settings->value(SETTING_UI_EDIT_MODE, true).toBool());
        restoreState(m_settings->value(SETTING_WINDOW_STATE).toByteArray());
    }

    updateDocks();
    setSlotInfo();
    setRecentInfo();

    if (m_settings->value(SETTING_DEBUGGER_RESTORE_ON_OPEN, false).toBool()) {
        if (!opts.debugFile.isEmpty()) {
            debuggerImportFile(opts.debugFile);
        } else {
            debuggerImportFile(m_settings->value(SETTING_DEBUGGER_IMAGE_PATH).toString());
        }
    }

    if (opts.useSettings && isFirstRun() && m_initPassed && !m_needFullReset) {
        QMessageBox *info = new QMessageBox();
        m_settings->setValue(SETTING_FIRST_RUN, true);
        info->setWindowTitle(MSG_INFORMATION);
        info->setText(tr("Welcome!\nCEmu uses a customizable dock-style interface. "
                            "Drag and drop to move tabs and windows around on the screen, "
                            "and choose which docks are available in the 'Docks' menu in the topmost bar. "
                            "Be sure that 'Enable UI edit mode' is selected when laying out your interface. "
                            "Enjoy!"));
        info->setWindowModality(Qt::NonModal);
        info->setWindowFlags(info->windowFlags() | Qt::WindowStaysOnTopHint);
        info->setAttribute(Qt::WA_DeleteOnClose);
        info->show();
    }

    QString prefLang = m_settings->value(SETTING_PREFERRED_LANG, "none").toString();
    if (prefLang != QStringLiteral("none")) {
        switchTranslator(prefLang);
    }

    translateExtras(TRANSLATE_UPDATE);

    optSend(opts);
    if (opts.speed != -1) {
        setEmuSpeed(opts.speed);
    }

    saveSettings();
    if (m_activatedPortable) {
        QMessageBox *info = new QMessageBox();
        info->setWindowTitle(MSG_INFORMATION);
        info->setText(tr("CEmu was not able to write to the standard settings location.\n"
                         "Portable mode has been activated."));
        info->setWindowModality(Qt::NonModal);
        info->setWindowFlags(info->windowFlags() | Qt::WindowStaysOnTopHint);
        info->setAttribute(Qt::WA_DeleteOnClose);
        info->show();
    }
}

void MainWindow::toggleKeyHistory() {
    if (!m_windowKeys) {
        m_windowKeys = new KeyHistory();
        connect(m_windowKeys, &KeyHistory::closed, this, &MainWindow::closedKeyHistory);
        connect(ui->keypadWidget, &KeypadWidget::keyPressed, m_windowKeys, &KeyHistory::addEntry);
        m_windowKeys->setAttribute(Qt::WA_DeleteOnClose);
        m_windowKeys->show();
        ui->actionKeyHistory->setChecked(true);
    } else {
        disconnect(ui->keypadWidget, &KeypadWidget::keyPressed, m_windowKeys, &KeyHistory::addEntry);
        m_windowKeys->close();
        m_windowKeys = Q_NULLPTR;
        ui->actionKeyHistory->setChecked(false);
    }
}

void MainWindow::closedKeyHistory() {
    disconnect(ui->keypadWidget, &KeypadWidget::keyPressed, m_windowKeys, &KeyHistory::addEntry);
    ui->actionKeyHistory->setChecked(false);
    m_windowKeys = Q_NULLPTR;
}

void MainWindow::optSend(CEmuOpts &o) {
    int speed = m_settings->value(SETTING_EMUSPEED).toInt();
    if (!o.autotesterFile.isEmpty()) {
        if (!openJSONConfig(o.autotesterFile)) {
           if (!o.deforceReset) {
               resetCalculator();
           }
           setEmuSpeed(100);

           // Race condition requires this
           guiDelay(o.deforceReset ? 100 : 4000);
           launchTest();
        }
    }

    if (!o.sendFiles.isEmpty() || !o.sendArchFiles.isEmpty() || !o.sendRAMFiles.isEmpty()) {
        if (!o.deforceReset) {
            resetCalculator();
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
}

void MainWindow::optLoadFiles(CEmuOpts &o) {
    if (o.romFile.isEmpty()) {
        if (m_loadedBootImage) {
            m_pathRom = configPath + SETTING_DEFAULT_ROM_FILE;
            m_settings->setValue(SETTING_ROM_PATH, m_pathRom);
        } else {
            m_pathRom = m_settings->value(SETTING_ROM_PATH).toString();
        }
    } else {
        m_pathRom = o.romFile;
        if (!m_settings->contains(SETTING_ROM_PATH)) {
            m_settings->setValue(SETTING_ROM_PATH, m_pathRom);
        }
    }

    if (!o.imageFile.isEmpty()) {
        if (fileExists(o.imageFile)) {
            m_pathImage = o.imageFile;
        }
    }
}

void MainWindow::optAttemptLoad(CEmuOpts &o) {
    if (!fileExists(m_pathRom)) {
        if (!runSetup()) {
            m_initPassed = false;
            close();
        }
    } else {
        if (o.restoreOnOpen && !o.imageFile.isEmpty() && fileExists(m_pathImage)) {
            loadEmu(true);
        } else {
            if (o.forceReloadRom) {
                loadEmu(false);
                guiDelay(500);
            }
        }
    }
}

MainWindow::~MainWindow() {
    delete m_settings;
    delete ui;
}

bool MainWindow::isInitialized() {
    return m_initPassed;
}

void MainWindow::resetCEmu() {
    ipcCloseOthers();
    m_needReload = true;
    m_needFullReset = true;
    close();
}

void MainWindow::resetGui() {
    ipcCloseOthers();
    m_settings->remove(SETTING_SCREEN_SKIN);
    m_settings->remove(SETTING_WINDOW_GEOMETRY);
    m_settings->remove(SETTING_WINDOW_MEMORY_DOCKS);
    m_settings->remove(SETTING_UI_EDIT_MODE);
    m_settings->remove(SETTING_WINDOW_STATE);
    m_settings->remove(SETTING_WINDOW_MENUBAR);
    m_settings->remove(SETTING_WINDOW_SEPARATOR);
    m_needReload = true;
    close();
}

bool MainWindow::isReload() {
    return m_needReload;
}

bool MainWindow::isResetAll() {
    if (m_needFullReset) {
        QFile imageFile(m_settings->value(SETTING_IMAGE_PATH).toString());
        QFile debugFile(m_settings->value(SETTING_DEFAULT_DEBUG_FILE).toString());
        imageFile.remove();
        debugFile.remove();
        if (m_keepSetup) {
            m_settings->remove(SETTING_IMAGE_PATH);
            m_settings->remove(SETTING_DEFAULT_DEBUG_FILE);
            m_settings->remove(SETTING_SCREEN_SKIN);
            m_settings->remove(SETTING_WINDOW_GEOMETRY);
            m_settings->remove(SETTING_WINDOW_MEMORY_DOCKS);
            m_settings->remove(SETTING_UI_EDIT_MODE);
            m_settings->remove(SETTING_WINDOW_STATE);
            m_settings->remove(SETTING_WINDOW_MENUBAR);
            m_settings->remove(SETTING_WINDOW_SEPARATOR);
            saveSettings();
        } else {
            QFile settingsFile(m_settings->fileName());
            settingsFile.remove();
        }
    }
    return m_needFullReset;
}

void MainWindow::sendASMKey() {
    autotester::sendKey(0x9CFC); // "Asm("
}

void MainWindow::saveEmu() {
    emu.save(true, m_pathImage);
}

void MainWindow::saveToPath(const QString &path) {
    emu.save(true, path);
}

bool MainWindow::restoreFromPath(const QString &path) {
    QString prev = m_pathImage;
    m_pathImage = path;
    int ret = loadEmu(true);
    m_pathImage = prev;
    return ret == EMU_LOAD_OKAY;
}

void MainWindow::restoreFromFile() {
    QString path = QFileDialog::getOpenFileName(this, tr("Select saved image to restore from"),
                                                      m_dir.absolutePath(),
                                                      tr("CEmu images (*.ce);;All files (*.*)"));
    if (!path.isEmpty()) {
        m_dir = QFileInfo(path).absoluteDir();
        if (!restoreFromPath(path)) {
            QMessageBox::critical(this, MSG_ERROR, tr("Could not resume; try restarting CEmu"));
        }
    }
}

void MainWindow::saveToFile() {
    QString path = QFileDialog::getSaveFileName(this, tr("Set image to save to"),
                                                      m_dir.absolutePath(),
                                                      tr("CEmu images (*.ce);;All files (*.*)"));
    if (!path.isEmpty()) {
        m_dir = QFileInfo(path).absoluteDir();
        saveToPath(path);
    }
}
void MainWindow::exportRom() {
    QString path = QFileDialog::getSaveFileName(this, tr("Set Rom image to save to"),
                                                m_dir.absolutePath(),
                                                tr("ROM images (*.rom);;All files (*.*)"));
    if (!path.isEmpty()) {
        m_dir = QFileInfo(path).absoluteDir();
        emu.save(false, path);
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

void MainWindow::savedEmu(bool success) {
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
            debuggerChangeState();
        }

        if (guiReceive) {
            receiveChangeState();
        }

        saveSettings();

        if (m_settings->value(SETTING_DEBUGGER_SAVE_ON_CLOSE, false).toBool()) {
            debuggerExportFile(m_settings->value(SETTING_DEBUGGER_IMAGE_PATH).toString());
        }

        if (m_settings->value(SETTING_SAVE_ON_CLOSE).toBool()) {
            saveEmu();
            e->ignore();
            return;
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
    static QColor sColorFg = Qt::black;
    static QColor sColorBg = Qt::white;
    const QColor lookup[8] = { Qt::black, Qt::red, Qt::green, Qt::yellow, Qt::blue, Qt::magenta, Qt::cyan, Qt::white };
    static int state = CONSOLE_ESC;
    if (size == -1) {
        size = strlen(str);
    }
    if (m_nativeConsole) {
        fwrite(str, sizeof(char), size, type == EmuThread::ConsoleErr ? stderr : stdout);
    } else {
        const char *tok;
        QColor colorFg = sColorFg;
        QColor colorBg = sColorBg;
        if ((tok = static_cast<const char*>(memchr(str, '\x1B', size)))) {
            if (tok != str) {
                console(QString::fromUtf8(str, tok - str), sColorFg, sColorBg, type);
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
                                    colorBg = Qt::white;
                                    colorFg = Qt::black;
                                    break;
                                case '3':
                                    state = CONSOLE_FGCOLOR;
                                    break;
                                case '4':
                                    state = CONSOLE_BGCOLOR;
                                    break;
                                default:
                                    state = CONSOLE_ESC;
                                    sColorBg = Qt::white;
                                    sColorFg = Qt::black;
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
                    const char *tokn = static_cast<const char*>(memchr(tok, '\x1B', size));
                    if (tokn) {
                        console(QString::fromUtf8(tok, tokn - tok), sColorFg, sColorBg, type);
                        size -= tokn - tok;
                    } else {
                        console(QString::fromUtf8(tok, size), sColorFg, sColorBg, type);
                    }
                    tok = tokn;
                } else {
                    tok = NULL;
                }
            } while (tok);
        } else {
            console(QString::fromUtf8(str, size), sColorFg, sColorBg, type);
        }
    }
}

void MainWindow::consoleStr(int type) {
    if (int available = emu.read.available()) {
        int remaining = CONSOLE_BUFFER_SIZE - emu.readPos;
        emu.read.acquire(available);

        console(type, emu.buffer + emu.readPos, available < remaining ? available : remaining);
        if (available < remaining) {
            emu.readPos += available;
        } else if (available == remaining) {
            emu.readPos = 0;
        } else {
            emu.readPos = available - remaining;
            console(type, emu.buffer, emu.readPos);
        }

        emu.write.release(available);
    }
}

void MainWindow::consoleSubmission() {
    QString string = ui->consoleLine->text().trimmed().toUpper();
    if (string == QStringLiteral("DBG")) {
        debuggerChangeState();
    }
    if (guiDebug) {
        if (string == QStringLiteral("C") || string == QStringLiteral("CONTINUE")) {
            debuggerChangeState();
        }
        if (string == QStringLiteral("N") || string == QStringLiteral("NEXT")) {
            stepNext();
        }
        if (string == QStringLiteral("S") || string == QStringLiteral("STEP")) {
            stepIn();
        }
        if (string == QStringLiteral("Q") || string == QStringLiteral("QUIT")) {
            debuggerChangeState();
        }
    }
    ui->consoleLine->clear();
}

void MainWindow::emuTimerSlot() {
    connect(&emu, &EmuThread::actualSpeedChanged, this, &MainWindow::showEmuSpeed, Qt::QueuedConnection);
}

void MainWindow::fpsTimerSlot() {
    m_timerFpsTriggered = true;
}

void MainWindow::showEmuSpeed(int speed) {
    static int speedPrev = 0;
    if (speedPrev != speed) {
        m_speedLabel.setText(tr("Emulated Speed: ") + QString::number(speed) + QStringLiteral("%"));
        speedPrev = speed;
    }
    if (m_timerEmuTriggerable) {
        disconnect(&emu, &EmuThread::actualSpeedChanged, this, &MainWindow::showEmuSpeed);
    }
}

void MainWindow::showFpsSpeed(double emuFps, double guiFps) {
    static double guiFpsPrev = 0;
    static double emuFpsPrev = 0;
    if (emuFpsPrev != emuFps) {
        ui->maxFps->setText(tr("Actual FPS: ") + QString::number(emuFps, 'f', 2));
        emuFpsPrev = emuFps;
    }
    if (guiFps < guiFpsPrev - 1 || guiFps > guiFpsPrev + 1) {
        m_fpsLabel.setText("FPS: " + QString::number(guiFps, 'f', 2));
        guiFpsPrev = guiFps;
    }
}

void MainWindow::updateLcd(double emuFps) {
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
    m_pathRom = path;
    if (m_portable) {
        QDir dir(qApp->applicationDirPath());
        m_pathRom = dir.relativeFilePath(m_pathRom);
    }
    loadEmu(false);
    ui->rompathView->setText(m_pathRom);
    m_settings->setValue(SETTING_ROM_PATH, m_pathRom);
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
            QFile(temppath).remove();
        } else {
            QFile(filename).remove();
            QFile(temppath).rename(filename);
        }
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

void MainWindow::saveScreenToClipboard() {
    QApplication::clipboard()->setImage(ui->lcd->getImage(), QClipboard::Clipboard);
}

#ifdef PNG_WRITE_APNG_SUPPORTED
void MainWindow::recordAPNG() {
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
                saveAnimated(filename);
            } else {
                updateAnimatedControls();
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

void MainWindow::saveAnimated(QString &filename) {
    ui->apngSkip->setEnabled(false);
    ui->actionRecordAnimated->setEnabled(false);
    ui->buttonRecordAnimated->setEnabled(false);
    ui->buttonRecordAnimated->setText(tr("Saving..."));
    ui->actionRecordAnimated->setText(tr("Saving Animated PNG..."));

    RecordingThread *thread = new RecordingThread();
    connect(thread, &RecordingThread::done, this, &MainWindow::updateAnimatedControls);
    connect(thread, &RecordingThread::finished, thread, &QObject::deleteLater);
    thread->m_filename = filename;
    thread->m_optimize = m_optimizeRecording;
    thread->start();
}

void MainWindow::updateAnimatedControls() {
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
    connect(buttonUpdateCheck, &QAbstractButton::clicked, this, [=](){ this->checkForUpdates(true); });

    QAbstractButton *okButton = aboutBox->addButton(QMessageBox::Ok);
    okButton->setFocus();

    QByteArray iconByteArray;
    QBuffer buffer(&iconByteArray);
    QPixmap(QStringLiteral(":/icons/resources/icons/icon.png")).save(&buffer, "PNG");

    aboutBox->setText(tr("%1<h3>CEmu %2</h3>"
                         "<a href='https://github.com/CE-Programming/CEmu'>On GitHub</a><br>"
                         "<br>Main authors:<br>%3"
                         "<br>Other contributors include:<br>%4"
                         "<br>Translations provided by:<br>%5"
                         "<br>Many thanks to the following projects: %6<br>In-program icons are courtesy of %7.<br>"
                         "<br>CEmu is licensed under the %8, and is not a TI product nor is it affiliated to/endorsed by TI.<br><br>")
                         .arg(QStringLiteral("<img src='data:image/png;base64,") + iconByteArray.toBase64() + "' align='right'/>",
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

    aboutBox->setTextFormat(Qt::RichText);
    aboutBox->setWindowModality(Qt::NonModal);
    aboutBox->setAttribute(Qt::WA_DeleteOnClose);
    aboutBox->show();
}

void MainWindow::screenContextMenu(const QPoint &posa) {
    QMenu menu;
    QPoint globalPos = ui->lcd->mapToGlobal(posa);
    menu.addMenu(ui->menuFile);
    menu.addMenu(ui->menuCalculator);
    menu.addMenu(m_menuDocks);
    menu.addMenu(ui->menuAbout);
    menu.exec(globalPos);
}

void MainWindow::consoleOutputChanged() {
    m_nativeConsole = ui->radioStderr->isChecked();
}

void MainWindow::pauseEmu(Qt::ApplicationState state) {
    if (m_pauseOnFocus) {
        if (state == Qt::ApplicationInactive) {
            emu.setEmuSpeed(0);
        }
        if (state == Qt::ApplicationActive) {
            setEmuSpeed(m_settings->value(SETTING_EMUSPEED).toInt());
        }
    }
}

// ------------------------------------------------
//  Linking things
// ------------------------------------------------

void MainWindow::receiveChangeState() {
    if (guiReceive) {
        changeVariableState();
        emu.unlock();
    } else {
        emu.receive();
    }
}

void MainWindow::selectFiles() {
    if (guiDebug) {
       return;
    }

    QStringList fileNames = showVariableFileDialog(QFileDialog::AcceptOpen, tr("TI Variable (*.8xp *.8xv *.8xl *.8xn *.8xm *.8xy *.8xg *.8xs *.8xd *.8xw *.8xc *.8xl *.8xz *.8xt *.8ca *.8cg *.8ci *.8ek);;All Files (*.*)"), Q_NULLPTR);

    sendingHandler->sendFiles(fileNames, LINK_FILE);
    equatesRefresh();
}

QStringList MainWindow::showVariableFileDialog(QFileDialog::AcceptMode mode, const QString &name_filter, const QString &defaultSuffix) {
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

void MainWindow::variableDoubleClicked(QTableWidgetItem *item) {
    calc_var_t var;
    memcpy(&var, ui->emuVarView->item(item->row(), VAR_NAME)->data(Qt::UserRole).toByteArray().data(), sizeof(calc_var_t));
    if (calc_var_is_asmprog(&var)) {
        return;
    } else if (var.type != CALC_VAR_TYPE_APP_VAR && (!calc_var_is_internal(&var) || var.name[0] == '#')) {
        std::string str;
        try {
            str = calc_var_content_string(var);
        } catch(std::exception e) {
            QMessageBox::warning(this, tr("Variable preview error"), tr("Could not get preview of variable. Error: ") + e.what());
            return;
        }
        BasicCodeViewerWindow *codePopup = new BasicCodeViewerWindow(this);
        codePopup->setVariableName(ui->emuVarView->item(item->row(), VAR_NAME)->text());
        codePopup->setWindowModality(Qt::NonModal);
        codePopup->setAttribute(Qt::WA_DeleteOnClose);
        codePopup->setOriginalCode(QString::fromStdString(str));
        codePopup->show();
    }
}

void MainWindow::lockedEmu(int req) {
    switch (req) {
        default:
        case EmuThread::RequestNone:
        case EmuThread::RequestPause:
            break;
        case EmuThread::RequestReceive:
            changeVariableState();
            break;
    }
}

void MainWindow::changeVariableState() {
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
            if (var.size > 2) {
                int row;

                row = ui->emuVarView->rowCount();
                ui->emuVarView->setRowCount(row + 1);

                bool var_preview_needs_gray = false;
                QString var_value;
                if (calc_var_is_asmprog(&var)) {
                    var_value = tr("Can't preview this");
                    var_preview_needs_gray = true;
                } else if (calc_var_is_internal(&var) && var.name[0] != '#') { // # is previewable
                    var_value = tr("Can't preview this OS variable");
                    var_preview_needs_gray = true;
                }  else if (var.type == CALC_VAR_TYPE_APP_VAR) { // nothing to do for now (or ever)
                    var_value = tr("Can't preview this");
                    var_preview_needs_gray = true;
                } else if (var.size > 500) {
                    var_value = tr("[Double-click to view...]");
                } else {
                    var_value = QString::fromStdString(calc_var_content_string(var));
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
                QTableWidgetItem *var_size = new QTableWidgetItem();

                // Attach var index (hidden) to the name. Needed elsewhere
                var_name->setData(Qt::UserRole, QByteArray(reinterpret_cast<const char*>(&var), sizeof(calc_var_t)));
                var_size->setData(Qt::DisplayRole, var.size);

                var_name->setCheckState(Qt::Unchecked);

                if (var_preview_needs_gray) {
                    var_preview->setForeground(Qt::gray);
                }

                ui->emuVarView->setItem(row, VAR_NAME, var_name);
                ui->emuVarView->setItem(row, VAR_LOCATION, var_location);
                ui->emuVarView->setItem(row, VAR_TYPE, var_type);
                ui->emuVarView->setItem(row, VAR_SIZE, var_size);
                ui->emuVarView->setItem(row, VAR_PREVIEW, var_preview);
            }
        }
        ui->emuVarView->resizeColumnToContents(VAR_NAME);
        ui->emuVarView->resizeColumnToContents(VAR_LOCATION);
        ui->emuVarView->resizeColumnToContents(VAR_TYPE);
        ui->emuVarView->resizeColumnToContents(VAR_SIZE);
    }

    ui->emuVarView->setSortingEnabled(true);
}

void MainWindow::saveSelectedFile() {
    QVector<calc_var_t> selectedVars;
    QStringList fileNames;
    for (int currRow = 0; currRow < ui->emuVarView->rowCount(); currRow++) {
        if (ui->emuVarView->item(currRow, VAR_NAME)->checkState() == Qt::Checked) {
            calc_var_t var;
            memcpy(&var, reinterpret_cast<const calc_var_t*>(ui->emuVarView->item(currRow, VAR_NAME)->data(Qt::UserRole).toByteArray().data()), sizeof(calc_var_t));
            selectedVars.append(var);
        }
    }
    if (selectedVars.size() < 2) {
        QMessageBox::warning(this, MSG_WARNING, tr("Select at least two files to group"));
    } else {
         fileNames = showVariableFileDialog(QFileDialog::AcceptSave, tr("TI Group (*.8cg);;All Files (*.*)"), QStringLiteral("8cg"));
        if (fileNames.size() == 1) {
            if (!receiveVariableLink(selectedVars.size(), selectedVars.constData(), fileNames.first().toUtf8())) {
                QMessageBox::critical(this, MSG_ERROR, tr("Transfer error, see console for information:\nFile: ") + fileNames.first());
            } else {
                QMessageBox::information(this, MSG_INFORMATION, tr("Transfer completed successfully."));
            }
        }
    }
}

void MainWindow::saveSelectedFiles() {
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::DirectoryOnly);
    dialog.setOption(QFileDialog::ShowDirsOnly, false);

    dialog.setDirectory(m_dir);
    int good = 0;

    for (int currRow = 0; currRow < ui->emuVarView->rowCount(); currRow++) {
        if (ui->emuVarView->item(currRow, VAR_NAME)->checkState() == Qt::Checked) {
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
        if (ui->emuVarView->item(currRow, VAR_NAME)->checkState() == Qt::Checked) {
            calc_var_t var;
            memcpy(&var, reinterpret_cast<const calc_var_t*>(ui->emuVarView->item(currRow, VAR_NAME)->data(Qt::UserRole).toByteArray().data()), sizeof(calc_var_t));

            name = QString(calc_var_name_to_utf8(var.name));
            filename = dialog.directory().absolutePath() + "/" + name + "." + m_varExtensions[var.type1];

            if (!receiveVariableLink(1, &var, filename.toStdString().c_str())) {
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

void MainWindow::resendFiles() {
    sendingHandler->resendSelected();
}

void MainWindow::resendContextMenu(const QPoint& posa) {
    int row = ui->varLoadedView->rowAt(posa.y());

    if (row == -1) {
        return;
    }

    int check = ui->varLoadedView->item(row, RECENT_PATH)->checkState();
    ui->varLoadedView->item(row, RECENT_SELECT)->setCheckState(check == Qt::Checked ? Qt::Unchecked : Qt::Checked);
}

void MainWindow::removeAllSentVars() {
    ui->varLoadedView->setRowCount(0);
}

void MainWindow::removeSentVars() {
    for (int row = ui->varLoadedView->rowCount() - 1; row >= 0; row--) {
        if (ui->varLoadedView->item(row, RECENT_SELECT)->checkState() == Qt::Checked) {
            ui->varLoadedView->removeRow(row);
        }
    }
}

void MainWindow::deselectAllVars() {
    for (int row = 0; row < ui->varLoadedView->rowCount(); row++) {
        ui->varLoadedView->item(row, RECENT_SELECT)->setCheckState(Qt::Unchecked);
    }
}

void MainWindow::selectAllVars() {
    for (int row = 0; row < ui->varLoadedView->rowCount(); row++) {
        ui->varLoadedView->item(row, RECENT_SELECT)->setCheckState(Qt::Checked);
    }
}

// ------------------------------------------------
// Autotester things
// ------------------------------------------------

void MainWindow::dispAutotesterError(int errCode) {
    QString errMsg;
    switch (errCode) {
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


int MainWindow::openJSONConfig(const QString& jsonPath) {
    if (jsonPath.length() == 0) {
        QMessageBox::warning(this, MSG_ERROR, tr("Please choose a json file or type its path."));
        return 1;
    }
    std::string jsonContents;
    std::ifstream ifs(jsonPath.toStdString());

    if (ifs.good()) {
        int ok = chdir(QDir::toNativeSeparators(QFileInfo(jsonPath).absoluteDir().path()).toStdString().c_str());
        if (ok != 0) {
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
    autotester::debugLogs = false;
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

void MainWindow::prepareAndOpenJSONConfig() {
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
    openJSONConfig(selected.first());
}

void MainWindow::reloadJSONConfig() {
    openJSONConfig(ui->JSONconfigPath->text());
}

void MainWindow::launchTest() {
    if (!autotester::configLoaded) {
        dispAutotesterError(-1);
        return;
    }

    if (ui->checkBoxTestReset->isChecked()) {
        resetCalculator();
        guiDelay(4000);
    }

    if (ui->checkBoxTestClear->isChecked()) {
        autotester::sendKey(0x09);
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
    if (!autotester::doTestSequence()) {
        dispAutotesterError(1);
        return;
    }
    if (!opts.suppressTestDialog) {
        QMessageBox::information(this, tr("Test results"), QString(tr("Out of %2 tests attempted:\n%4 passed\n%6 failed")).arg(QString::number(autotester::hashesTested), QString::number(autotester::hashesPassed), QString::number(autotester::hashesFailed)));
    }
}

void MainWindow::updateCRCParamsFromPreset(int comboBoxIndex) {
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
    if (comboBoxIndex >= 1 && comboBoxIndex <= (int)(sizeof(mapIdConsts)/sizeof(mapIdConsts[0]))) {
        char buf[10] = {0};
        sprintf(buf, "0x%X", mapIdConsts[comboBoxIndex-1].first);
        ui->startCRC->setText(buf);
        sprintf(buf, "0x%X", mapIdConsts[comboBoxIndex-1].second);
        ui->sizeCRC->setText(buf);
        refreshCRC();
    }
}

void MainWindow::refreshCRC() {
    uint32_t tmp_start = 0;
    int32_t crc_size = 0;
    void *start;
    char *endptr1, *endptr2; // catch strtoul issues
    QLineEdit *startCRC = ui->startCRC;
    QLineEdit *sizeCRC = ui->sizeCRC;

    startCRC->setText(startCRC->text().trimmed());
    sizeCRC->setText(sizeCRC->text().trimmed());

    if (startCRC->text().isEmpty() || sizeCRC->text().isEmpty()) {
        goto errCRCret;
    }

    // Get GUI values
    tmp_start = (uint32_t)strtoul(startCRC->text().toStdString().c_str(), &endptr1, 0);
    crc_size = (size_t)strtoul(sizeCRC->text().toStdString().c_str(), &endptr2, 0);
    if (*endptr1 || *endptr2) {
        goto errCRCret;
    }

    // Get real start pointer
    start = virt_mem_dup(tmp_start, crc_size);

    // Compute and display CRC
    char buf[10];
    sprintf(buf, "%X", crc32(start, crc_size));
    free(start);
    ui->valueCRC->setText(buf);
    return;

errCRCret:
    QMessageBox::critical(this, MSG_ERROR, tr("Make sure you have entered a valid start/size pair or preset."));
    return;
}

void MainWindow::resetCalculator() {
    guiReset = true;

    if (guiReceive) {
        receiveChangeState();
    }
    if (guiDebug) {
        debuggerChangeState();
    }

    emu.reset();
}

int MainWindow::loadEmu(bool image) {
    guiEmuValid = false;

    if (guiReceive) {
        receiveChangeState();
    }
    if (guiDebug) {
        debuggerChangeState();
    }

    int success = emu.load(image, m_pathRom, m_pathImage);

    switch (success) {
        case EMU_LOAD_OKAY:
            ui->lcd->setMain();
            setCalcSkinTopFromType();
            setKeypadColor(m_settings->value(SETTING_KEYPAD_COLOR, get_device_type() ? KEYPAD_WHITE : KEYPAD_BLACK).toUInt());
            break;
        case EMU_LOAD_NOTROM:
            if (QMessageBox::Yes == QMessageBox::question(this, MSG_WARNING, tr("Image does not appear to be from a CE. Do you want to attempt to load it anyway? "
                                                          "This may cause instability."), QMessageBox::Yes|QMessageBox::No)) {
                ui->lcd->setMain();
                setCalcSkinTopFromType();
                setKeypadColor(m_settings->value(SETTING_KEYPAD_COLOR, get_device_type() ? KEYPAD_WHITE : KEYPAD_BLACK).toUInt());
                success = EMU_LOAD_OKAY;
            }
            break;
        default:
        case EMU_LOAD_FAIL:
            QMessageBox::critical(this, MSG_ERROR, tr("Could not load image. Please see console for more information."));
            break;
    }

    if (success == EMU_LOAD_OKAY) {
        emu.start();
        guiEmuValid = true;
    }

    guiReset = false;
    return success;
}

void MainWindow::drawNextDisasmLine() {
    bool useLabel = false;
    map_t::iterator sit;
    std::pair<map_t::iterator, map_t::iterator> range;
    unsigned int numLines = 1;

    if (disasm.baseAddress != disasm.newAddress) {
        disasm.baseAddress = disasm.newAddress;
        if (disasm.map.count(disasm.newAddress)) {
            range = disasm.map.equal_range(disasm.newAddress);

            numLines = 0;
            for (sit = range.first;  sit != range.second;  ++sit) {
               numLines++;
            }

            disasmHighlight.rWatch = false;
            disasmHighlight.wWatch = false;
            disasmHighlight.xBreak = false;
            disasmHighlight.pc = false;

            disasm.instruction.data.clear();
            disasm.instruction.opcode.clear();
            disasm.instruction.modeSuffix.clear();
            disasm.instruction.arguments.clear();
            disasm.instruction.size = 0;

            useLabel = true;
        } else {
            disassembleInstruction();
        }
    } else {
        disassembleInstruction();
    }

    if (useLabel) {
        range = disasm.map.equal_range(disasm.newAddress);
        sit = range.first;
    }

    for (unsigned int j = 0; j < numLines; j++) {

        QString formattedLine;
        QString breakpointSymbols;
        QString instructionArgsHighlighted;

        if (useLabel) {
            if (disasm.baseAddress > 511 || (disasm.baseAddress < 512 && sit->second[0] == '_')) {
                formattedLine = QString(QStringLiteral("<pre><b><font color='#444'>%1</font></b>     %2</pre>"))
                                        .arg(int2hex(disasm.baseAddress, 6),
                                             QString::fromStdString(sit->second) + ":");

                ui->disassemblyView->appendHtml(formattedLine);
            }

            if (numLines == j + 1) {
                useLabel = false;
            }
            sit++;
        } else {
            // Some round symbol things
            breakpointSymbols = QString(QStringLiteral("<font color='#A3FFA3'>%1</font><font color='#A3A3FF'>%2</font><font color='#FFA3A3'>%3</font>"))
                                        .arg((disasmHighlight.rWatch  ? QStringLiteral("&#9679;") : QStringLiteral(" ")),
                                             (disasmHighlight.wWatch ? QStringLiteral("&#9679;") : QStringLiteral(" ")),
                                             (disasmHighlight.xBreak  ? QStringLiteral("&#9679;") : QStringLiteral(" ")));

            // Simple syntax highlighting
            instructionArgsHighlighted = QString::fromStdString(disasm.instruction.arguments)
                                                  .replace(QRegularExpression(QStringLiteral("(\\$[0-9a-fA-F]+)")), QStringLiteral("<font color='green'>\\1</font>")) // hex numbers
                                                  .replace(QRegularExpression(QStringLiteral("(^\\d)")), QStringLiteral("<font color='blue'>\\1</font>"))             // dec number
                                                  .replace(QRegularExpression(QStringLiteral("([()])")), QStringLiteral("<font color='#600'>\\1</font>"));            // parentheses

            formattedLine = QString(QStringLiteral("<pre><b><font color='#444'>%1</font></b> %2 %3  <font color='darkblue'>%4%5</font>%6</pre>"))
                                    .arg(int2hex(disasm.baseAddress, 6),
                                        breakpointSymbols,
                                        QString::fromStdString(disasm.instruction.data).leftJustified(12, ' '),
                                        QString::fromStdString(disasm.instruction.opcode),
                                        QString::fromStdString(disasm.instruction.modeSuffix),
                                        instructionArgsHighlighted);

            ui->disassemblyView->appendHtml(formattedLine);
        }
    }

    if (!m_disasmOffsetSet && disasm.newAddress > m_addressPane) {
        m_disasmOffsetSet = true;
        m_disasmOffset = ui->disassemblyView->textCursor();
        m_disasmOffset.movePosition(QTextCursor::StartOfLine);
    }

    if (disasmHighlight.pc == true) {
        ui->disassemblyView->addHighlight(QColor(Qt::blue).lighter(160));
    }
}

void MainWindow::disasmContextMenu(const QPoint& posa) {
    QString setPc = tr("Set PC");
    QString toggleBreak = tr("Toggle Breakpoint");
    QString toggleWrite = tr("Toggle Write Watchpoint");
    QString toggleRead = tr("Toggle Read Watchpoint");
    QString toggleRw = tr("Toggle Read/Write Watchpoint");
    QString runUntil = tr("Run Until");
    QString gotoMem = tr("Goto Memory View");
    ui->disassemblyView->setTextCursor(ui->disassemblyView->cursorForPosition(posa));
    QPoint globalPos = ui->disassemblyView->mapToGlobal(posa);
    QString addressStr = ui->disassemblyView->getSelectedAddress();
    uint32_t address = static_cast<uint32_t>(hex2int(addressStr));

    QMenu menu;
    menu.addAction(runUntil);
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
            ui->pcregView->setText(addressStr);
            debug_set_pc_address(address);
            updateDisasmAddr(cpu.registers.PC, true);
        } else if (item->text() == toggleBreak) {
            breakpointGUIAdd();
        } else if (item->text() == toggleRead) {
            watchpointReadGUIAdd();
        } else if (item->text() == toggleWrite) {
            watchpointWriteGUIAdd();
        } else if (item->text() == toggleRw) {
            watchpointReadWriteGUIAdd();
        } else if (item->text() == runUntil) {
            debugger.runUntilAddress = address;
            debuggerChangeState();
            debuggerStep(DBG_RUN_UNTIL);
        } else if (item->text() == gotoMem) {
            gotoMemAddr(address);
        }
    }
}

void MainWindow::launchPrgm(const calc_var_t *prgm) {
    // Reset keypad state
    keypad_reset();

    // Restore focus to catch keypresses.
    ui->lcd->setFocus();
    guiDelay(100);

    // Launch the program, assuming we're at the home screen...
    autotester::sendKey(0x09); // Clear
    if (calc_var_is_asmprog(prgm)) {
        autotester::sendKey(0x9CFC); // Asm(
    }
    autotester::sendKey(0xDA); // prgm
    for (const char& c : prgm->name) {
        if (!c) { break; }
        autotester::sendLetterKeyPress(c); // type program name
    }
    autotester::sendKey(0x05); // Enter

    // Restore focus to catch keypresses.
    ui->lcd->setFocus();
}

void MainWindow::variablesContextMenu(const QPoint& posa) {
    int itemRow = ui->emuVarView->rowAt(posa.y());
    if (itemRow == -1) {
        return;
    }

    const calc_var_t var = *reinterpret_cast<const calc_var_t*>(ui->emuVarView->item(itemRow, VAR_NAME)->data(Qt::UserRole).toByteArray().constData());

    QString launch = tr("Launch program");

    QMenu contextMenu;
    if (calc_var_is_prog(&var) && !calc_var_is_internal(&var)) {
        contextMenu.addAction(launch);
    }

    QAction *selectedItem = contextMenu.exec(ui->emuVarView->mapToGlobal(posa));
    if (selectedItem) {
        if (selectedItem->text() == launch) {
            // resume emulation and launch
            receiveChangeState();
            launchPrgm(&var);
        }
    }
}

void MainWindow::newMemoryVisualizer() {
    MemoryVisualizer *p = new MemoryVisualizer(this);
    p->setAttribute(Qt::WA_DeleteOnClose);
    p->show();
}

void MainWindow::consoleContextMenu(const QPoint &posa) {
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
        address = hex2int(equ);
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
                forceEnterDebug();
                gotoMemAddr(address);
            } else if (item->text() == gotoDisasm) {
                forceEnterDebug();
                gotoDisasmAddr(address);
            } else if (item->text() == toggleBreak) {
                breakpointAdd(breakpointNextLabel(), address, true, true);
            } else if (item->text() == toggleRead) {
                watchpointAdd(watchpointNextLabel(), address, 1, DBG_MASK_READ, true);
            } else if (item->text() == toggleWrite) {
                watchpointAdd(watchpointNextLabel(), address, 1, DBG_MASK_WRITE, true);
            } else if (item->text() == toggleRw) {
                watchpointAdd(watchpointNextLabel(), address, 1, DBG_MASK_READ | DBG_MASK_WRITE, true);
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
        console(QStringLiteral("[CEmu] Initialized Server [") + opts.idString + QStringLiteral(" | ") + com.getServerName() + QStringLiteral("]\n"));
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
           << opts.speed;

    // blocking call
    com.send(byteArray);
    return false;
}

void MainWindow::ipcHandleCommandlineReceive(QDataStream &stream) {
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
           >> o.speed;

    optLoadFiles(o);
    optAttemptLoad(o);
    optSend(o);
    if (o.speed != -1) {
        setEmuSpeed(o.speed);
    }
}

void MainWindow::ipcCloseOthers() {
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
           show();
           raise();
           ipcHandleCommandlineReceive(stream);
           break;
        case IPC_CLOSE:
            close();
            break;
        default:
           console(QStringLiteral("[CEmu] IPC Unknown\n"));
           break;
    }
}

void MainWindow::ipcChangeID() {
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

void MainWindow::ipcSpawnRandom() {
    QStringList arguments;
    arguments << QStringLiteral("--id") << randomString(15);

    QProcess *myProcess = new QProcess(this);
    myProcess->startDetached(execPath, arguments);
}

void MainWindow::slotAddNew() {
    QString name = randomString(6);
    QString path = QDir::cleanPath(QFileInfo(m_settings->fileName()).absoluteDir().absolutePath() + QStringLiteral("/") + name + QStringLiteral(".ce"));
    slotAdd(name, path);
}

void MainWindow::slotAdd(QString &name, QString &path) {
    const int row = ui->slotView->rowCount();

    QToolButton *btnLoad   = new QToolButton();
    QToolButton *btnSave   = new QToolButton();
    QToolButton *btnEdit   = new QToolButton();
    QToolButton *btnRemove = new QToolButton();

    btnLoad->setIcon(m_iconLoad);
    btnSave->setIcon(m_iconSave);
    btnEdit->setIcon(m_iconEdit);
    btnRemove->setIcon(m_iconRemove);

    connect(btnRemove, &QToolButton::clicked, this, &MainWindow::slotRemove);
    connect(btnLoad, &QToolButton::clicked, this, &MainWindow::slotLoad);
    connect(btnSave, &QToolButton::clicked, this, &MainWindow::slotSave);
    connect(btnEdit, &QToolButton::clicked, this, &MainWindow::slotEdit);

    QTableWidgetItem *itemName   = new QTableWidgetItem(name);
    QTableWidgetItem *itemLoad   = new QTableWidgetItem();
    QTableWidgetItem *itemSave   = new QTableWidgetItem();
    QTableWidgetItem *itemEdit   = new QTableWidgetItem();
    QTableWidgetItem *itemRemove = new QTableWidgetItem();

    itemEdit->setData(Qt::UserRole, path);
    saveToPath(itemEdit->data(Qt::UserRole).toString());

    ui->slotView->setRowCount(row + 1);
    ui->slotView->setItem(row, SLOT_NAME, itemName);
    ui->slotView->setItem(row, SLOT_LOAD, itemLoad);
    ui->slotView->setItem(row, SLOT_SAVE, itemSave);
    ui->slotView->setItem(row, SLOT_EDIT, itemEdit);
    ui->slotView->setItem(row, SLOT_REMOVE, itemRemove);

    ui->slotView->setCellWidget(row, SLOT_LOAD, btnLoad);
    ui->slotView->setCellWidget(row, SLOT_SAVE, btnSave);
    ui->slotView->setCellWidget(row, SLOT_EDIT, btnEdit);
    ui->slotView->setCellWidget(row, SLOT_REMOVE, btnRemove);

    ui->slotView->setCurrentCell(row, SLOT_NAME);
    saveSlotInfo();
}

int MainWindow::slotGet(QObject *obj, int col) {
    int row;

    for (row = 0; row < ui->slotView->rowCount(); row++){
        if (obj == ui->slotView->cellWidget(row, col)) {
            break;
        }
    }

    return row;
}

void MainWindow::slotEdit() {
    bool ok;
    int row = slotGet(sender(), SLOT_EDIT);
    QString old = ui->slotView->item(row, SLOT_EDIT)->data(Qt::UserRole).toString();
    QString path = QInputDialog::getText(this, tr("Enter image path"), Q_NULLPTR, QLineEdit::Normal, old, &ok);
    if (ok && !path.isEmpty()) {
        QFile(old).rename(path);
        ui->slotView->item(row, SLOT_EDIT)->setData(Qt::UserRole, path);
    }
}

void MainWindow::slotRemove() {
    int row = slotGet(sender(), SLOT_REMOVE);
    QFile(ui->slotView->item(row, SLOT_EDIT)->data(Qt::UserRole).toString()).remove();
    ui->slotView->removeRow(row);
}

void MainWindow::slotSave() {
    int row = slotGet(sender(), SLOT_SAVE);
    QString path = ui->slotView->item(row, SLOT_EDIT)->data(Qt::UserRole).toString();
    saveToPath(path);
}

void MainWindow::slotLoad() {
    int row = slotGet(sender(), SLOT_LOAD);
    QString path = ui->slotView->item(row, SLOT_EDIT)->data(Qt::UserRole).toString();
    restoreFromPath(path);
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
