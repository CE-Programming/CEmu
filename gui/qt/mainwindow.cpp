#include <QtWidgets/QDesktopWidget>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtGui/QWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QProgressDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QScrollBar>
#include <QtNetwork/QNetworkReply>
#include <QClipboard>
#include <fstream>
#include <stdio.h>

#ifdef _MSC_VER
    #include <direct.h>
    #define chdir _chdir
#else
    #include <unistd.h>
#endif

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "utils.h"
#include "sendinghandler.h"
#include "memoryvisualizer.h"
#include "dockwidget.h"
#include "searchwidget.h"
#include "basiccodeviewerwindow.h"
#include "capture/animated-png.h"

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

MainWindow::MainWindow(CEmuOpts cliOpts, QWidget *p) : QMainWindow(p), ui(new Ui::MainWindow), opts(cliOpts) {

    // Setup translations
    appTranslator.load(QLocale::system().name(), QStringLiteral(":/i18n/i18n/"));
    qApp->installTranslator(&appTranslator);

    // start up ipc
    com = new ipc();
    qsrand(time(NULL));

    // Setup the UI
    ui->setupUi(this);

    // init IPC
    if (!ipcSetup()) {
        initPassed = false;
        return;
    }

    ui->centralWidget->hide();
    ui->statusBar->addWidget(&speedLabel);
    ui->statusBar->addPermanentWidget(&msgLabel);

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
    progressBar = new QProgressBar(this);
    ui->statusBar->addWidget(progressBar);
    sendingHandler = new SendingHandler(this, progressBar, ui->varLoadedView);

    memory.append(ui->flashEdit);
    memory.append(ui->ramEdit);
    memory.append(ui->memEdit);

    // Emulator -> GUI
    connect(&emu, &EmuThread::consoleStr, this, &MainWindow::consoleStr);
    connect(&emu, &EmuThread::consoleErrStr, this, &MainWindow::consoleErrStr);
    connect(&emu, &EmuThread::started, this, &MainWindow::started, Qt::QueuedConnection);
    connect(&emu, &EmuThread::stopped, this, &MainWindow::emuStopped, Qt::QueuedConnection);
    connect(&emu, &EmuThread::restored, this, &MainWindow::restored, Qt::QueuedConnection);
    connect(&emu, &EmuThread::saved, this, &MainWindow::saved, Qt::QueuedConnection);
    connect(&emu, &EmuThread::sendGuiUpdates, this, &MainWindow::showEmuUpdates, Qt::QueuedConnection);

    // Console actions
    connect(ui->buttonConsoleclear, &QPushButton::clicked, ui->console, &QPlainTextEdit::clear);
    connect(ui->radioConsole, &QRadioButton::clicked, this, &MainWindow::consoleOutputChanged);
    connect(ui->radioStderr, &QRadioButton::clicked, this, &MainWindow::consoleOutputChanged);

    // Debugger
    connect(&emu, &EmuThread::raiseDebugger, this, &MainWindow::debuggerRaise, Qt::QueuedConnection);
    connect(&emu, &EmuThread::disableDebugger, this, &MainWindow::debuggerGUIDisable, Qt::QueuedConnection);
    connect(&emu, &EmuThread::sendDebugCommand, this, &MainWindow::debuggerProcessCommand, Qt::QueuedConnection);
    connect(this, &MainWindow::setDebugState, &emu, &EmuThread::setDebugMode);
    connect(this, &MainWindow::setDebugStepInMode, &emu, &EmuThread::setDebugStepInMode);
    connect(this, &MainWindow::setRunUntilMode, &emu, &EmuThread::setRunUntilMode);
    connect(this, &MainWindow::setDebugStepOverMode, &emu, &EmuThread::setDebugStepOverMode);
    connect(this, &MainWindow::setDebugStepNextMode, &emu, &EmuThread::setDebugStepNextMode);
    connect(this, &MainWindow::setDebugStepOutMode, &emu, &EmuThread::setDebugStepOutMode);
    connect(ui->buttonRun, &QPushButton::clicked, this, &MainWindow::debuggerChangeState);
    connect(ui->checkADLDisasm, &QCheckBox::stateChanged, this, &MainWindow::toggleADLDisasm);
    connect(ui->checkADLStack, &QCheckBox::stateChanged, this, &MainWindow::toggleADLStack);
    connect(ui->checkADL, &QCheckBox::stateChanged, this, &MainWindow::toggleADL);

    connect(ui->buttonAddPort, &QPushButton::clicked, this, &MainWindow::portSlotAdd);
    connect(ui->buttonAddBreakpoint, &QPushButton::clicked, this, &MainWindow::breakpointSlotAdd);
    connect(ui->buttonAddWatchpoint, &QPushButton::clicked, this, &MainWindow::watchpointSlotAdd);
    connect(ui->buttonStepIn, &QPushButton::clicked, this, &MainWindow::stepInPressed);
    connect(ui->buttonStepOver, &QPushButton::clicked, this, &MainWindow::stepOverPressed);
    connect(ui->buttonStepNext, &QPushButton::clicked, this, &MainWindow::stepNextPressed);
    connect(ui->buttonStepOut, &QPushButton::clicked, this, &MainWindow::stepOutPressed);
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
    connect(ui->disassemblyView, &DataWidget::gotoDisasmAddress, this, &MainWindow::forceGotoDisasm);
    connect(ui->disassemblyView, &DataWidget::gotoMemoryAddress, this, &MainWindow::forceGotoMemory);

    // Ctrl + Click
    connect(ui->console, &QPlainTextEdit::cursorPositionChanged, this, [this]{ handleCtrlClickText(ui->console); });
    connect(ui->stackView, &QPlainTextEdit::cursorPositionChanged, this, [this]{ handleCtrlClickText(ui->stackView); });
    connect(ui->hlregView, &QLineEdit::cursorPositionChanged, this, [this]{ handleCtrlClickLine(ui->hlregView); });
    connect(ui->bcregView, &QLineEdit::cursorPositionChanged, this, [this]{ handleCtrlClickLine(ui->bcregView); });
    connect(ui->deregView, &QLineEdit::cursorPositionChanged, this, [this]{ handleCtrlClickLine(ui->deregView); });
    connect(ui->ixregView, &QLineEdit::cursorPositionChanged, this, [this]{ handleCtrlClickLine(ui->ixregView); });
    connect(ui->iyregView, &QLineEdit::cursorPositionChanged, this, [this]{ handleCtrlClickLine(ui->iyregView); });
    connect(ui->pcregView, &QLineEdit::cursorPositionChanged, this, [this]{ handleCtrlClickLine(ui->pcregView); });
    connect(ui->splregView, &QLineEdit::cursorPositionChanged, this, [this]{ handleCtrlClickLine(ui->splregView); });
    connect(ui->spsregView, &QLineEdit::cursorPositionChanged, this, [this]{ handleCtrlClickLine(ui->spsregView); });
    connect(ui->hl_regView, &QLineEdit::cursorPositionChanged, this, [this]{ handleCtrlClickLine(ui->hl_regView); });
    connect(ui->bc_regView, &QLineEdit::cursorPositionChanged, this, [this]{ handleCtrlClickLine(ui->bc_regView); });
    connect(ui->de_regView, &QLineEdit::cursorPositionChanged, this, [this]{ handleCtrlClickLine(ui->de_regView); });

    // Debugger Options
    connect(ui->buttonAddEquateFile, &QPushButton::clicked, this, &MainWindow::equatesAddDialog);
    connect(ui->buttonClearEquates, &QPushButton::clicked, this, &MainWindow::equatesClear);
    connect(ui->buttonRefreshEquates, &QPushButton::clicked, this, &MainWindow::equatesRefresh);
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
    connect(&emu, &EmuThread::receiveReady, this, &MainWindow::changeVariableList, Qt::QueuedConnection);
    connect(this, &MainWindow::receive, &emu, &EmuThread::receive);
    connect(this, &MainWindow::receiveDone, &emu, &EmuThread::receiveDone);

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
    connect(ui->actionRestoreState, &QAction::triggered, this, &MainWindow::restoreEmuState);
    connect(ui->actionSaveState, &QAction::triggered, this, &MainWindow::saveEmuState);
    connect(ui->actionExportCalculatorState, &QAction::triggered, this, &MainWindow::saveToFile);
    connect(ui->actionExportRomImage, &QAction::triggered, this, &MainWindow::exportRom);
    connect(ui->actionImportCalculatorState, &QAction::triggered, this, &MainWindow::restoreFromFile);
    connect(ui->actionReloadROM, &QAction::triggered, this, &MainWindow::reloadROM);
    connect(ui->actionResetALL, &QAction::triggered, this, &MainWindow::reloadAll);
    connect(ui->actionResetGUI, &QAction::triggered, this, &MainWindow::reloadGui);
    connect(ui->actionResetCalculator, &QAction::triggered, this, &MainWindow::resetCalculator);
    connect(ui->actionMemoryVisualizer, &QAction::triggered, this, &MainWindow::newMemoryVisualizer);
    connect(ui->actionDisableMenuBar, &QAction::triggered, this, &MainWindow::setMenuBarState);
    connect(ui->buttonResetCalculator, &QPushButton::clicked, this, &MainWindow::resetCalculator);
    connect(ui->buttonReloadROM, &QPushButton::clicked, this, &MainWindow::resetCalculator);

    // Reset and reload
    connect(this, &MainWindow::reset, &emu, &EmuThread::reset);
    connect(this, &MainWindow::load, &emu, &EmuThread::load);

    // LCD Update
    connect(&emu, &EmuThread::updateLcd, ui->lcd, static_cast<void (LCDWidget::*)(void)>(&LCDWidget::update), Qt::QueuedConnection);
    connect(this, &MainWindow::updateMode, &emu, &EmuThread::setMode);
    connect(this, &MainWindow::updateFrameskip, &emu, &EmuThread::setFrameskip);

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
    connect(ui->actionCheckForUpdates, &QAction::triggered, this, [=](){ this->checkForUpdates(true); });
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);

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
    connect(ui->lcd, &LCDWidget::customContextMenuRequested, this, &MainWindow::screenContextMenu);
    connect(ui->checkSaveRestore, &QCheckBox::stateChanged, this, &MainWindow::setAutoSaveState);
    connect(ui->checkPortable, &QCheckBox::stateChanged, this, &MainWindow::setPortableConfig);
    connect(ui->checkSaveRecent, &QCheckBox::stateChanged, this, &MainWindow::setRecentSave);
    connect(ui->checkSaveLoadDebug, &QCheckBox::stateChanged, this, &MainWindow::setSaveDebug);
    connect(ui->buttonChangeSavedImagePath, &QPushButton::clicked, this, &MainWindow::setImagePath);
    connect(ui->buttonChangeSavedDebugPath, &QPushButton::clicked, this, &MainWindow::setDebugPath);
    connect(ui->checkFocus, &QCheckBox::stateChanged, this, &MainWindow::setFocusSetting);
    connect(this, &MainWindow::changedEmuSpeed, &emu, &EmuThread::setEmuSpeed);
    connect(this, &MainWindow::changedThrottleMode, &emu, &EmuThread::setThrottleMode);
    connect(ui->checkPreI, &QCheckBox::stateChanged, this, &MainWindow::setPreRevisionI);
    connect(ui->flashBytes, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui->flashEdit, &QHexEdit::setBytesPerLine);
    connect(ui->ramBytes, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui->ramEdit, &QHexEdit::setBytesPerLine);
    connect(ui->memBytes, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui->memEdit, &QHexEdit::setBytesPerLine);
    connect(ui->emuVarView, &QTableWidget::itemDoubleClicked, this, &MainWindow::variableDoubleClicked);
    connect(ui->emuVarView, &QTableWidget::customContextMenuRequested, this, &MainWindow::variablesContextMenu);
    connect(ui->buttonAddSlot, &QPushButton::clicked, this, &MainWindow::slotAddNew);
    connect(ui->actionExportCEmuImage, &QAction::triggered, this, &MainWindow::exportCEmuBootImage);
    connect(ui->lcd, &LCDWidget::sendROM, this, &MainWindow::setRom);

    // Lang switch
    connect(ui->actionEnglish,  &QAction::triggered, this, [this]{ switchTranslator(QStringLiteral("en_EN")); });
    connect(ui->actionFran_ais, &QAction::triggered, this, [this]{ switchTranslator(QStringLiteral("fr_FR")); });
    connect(ui->actionDutch,    &QAction::triggered, this, [this]{ switchTranslator(QStringLiteral("nl_NL")); });
    connect(ui->actionEspanol,  &QAction::triggered, this, [this]{ switchTranslator(QStringLiteral("es_ES")); });

    // Hex Editor
    connect(ui->buttonFlashSearch, &QPushButton::clicked, this, [this]{ memSearchPressed(MEM_FLASH); });
    connect(ui->buttonRamSearch, &QPushButton::clicked, this, [this]{ memSearchPressed(MEM_RAM); });
    connect(ui->buttonMemSearch, &QPushButton::clicked, this, [this]{ memSearchPressed(MEM_MEM); });
    connect(ui->buttonMemGoto, &QPushButton::clicked, this, [this]{ memGotoPressed(MEM_MEM); });
    connect(ui->buttonFlashGoto, &QPushButton::clicked, this, &MainWindow::flashGotoPressed);
    connect(ui->buttonRamGoto, &QPushButton::clicked, this, &MainWindow::ramGotoPressed);
    connect(ui->buttonFlashSync, &QPushButton::clicked, this, &MainWindow::flashSyncPressed);
    connect(ui->buttonRamSync, &QPushButton::clicked, this, &MainWindow::ramSyncPressed);
    connect(ui->buttonMemSync, &QPushButton::clicked, this, [this]{ memSyncPressed(MEM_MEM); });
    connect(ui->memEdit, &QHexEdit::customContextMenuRequested, this, &MainWindow::memContextMenu);
    connect(ui->flashEdit, &QHexEdit::customContextMenuRequested, this, &MainWindow::memContextMenu);
    connect(ui->ramEdit, &QHexEdit::customContextMenuRequested, this, &MainWindow::memContextMenu);

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

    // Application connections
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &MainWindow::pauseEmu);

    // Key History Window
    connect(ui->actionKeyHistory, &QAction::triggered, this, &MainWindow::toggleKeyHistory);

    // Auto Updates
    connect(ui->checkUpdates, &QCheckBox::stateChanged, this, &MainWindow::setAutoCheckForUpdates);

    // IPC
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::ipcSpawnRandom);
    connect(ui->buttonChangeID, &QPushButton::clicked, this, &MainWindow::ipcChangeID);
    connect(com, &ipc::readDone, this, &MainWindow::ipcReceived);

    // Clipboard copy
    connect(ui->actionClipScreen, &QAction::triggered, this, &MainWindow::saveScreenToClipboard);

    // Docks
    translateExtras(TRANSLATE_INIT);
    actionToggleUI = new QAction(MSG_EDIT_UI, this);
    actionToggleUI->setCheckable(true);
    connect(actionToggleUI, &QAction::triggered, this, &MainWindow::toggleUIEditMode);

    actionAddMemory = new QAction(MSG_ADD_MEMORY, this);
    connect(actionAddMemory, &QAction::triggered, this, [this]{ createMemoryDock(TXT_MEM_DOCK); });

    // Shortcut Connections
    stepInShortcut = new QShortcut(QKeySequence(Qt::Key_F6), this);
    stepOverShortcut = new QShortcut(QKeySequence(Qt::Key_F7), this);
    stepNextShortcut = new QShortcut(QKeySequence(Qt::Key_F8), this);
    stepOutShortcut = new QShortcut(QKeySequence(Qt::Key_F9), this);
    debuggerShortcut = new QShortcut(QKeySequence(Qt::Key_F10), this);
    asmShortcut = new QShortcut(QKeySequence(Qt::Key_Pause), this);
    resendshortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_X), this);

    debuggerShortcut->setAutoRepeat(false);
    stepInShortcut->setAutoRepeat(false);
    stepOverShortcut->setAutoRepeat(false);
    stepNextShortcut->setAutoRepeat(false);
    stepOutShortcut->setAutoRepeat(false);
    asmShortcut->setAutoRepeat(false);
    resendshortcut->setAutoRepeat(false);

    connect(resendshortcut, &QShortcut::activated, this, &MainWindow::resendFiles);
    connect(asmShortcut, &QShortcut::activated, this, &MainWindow::sendASMKey);
    connect(debuggerShortcut, &QShortcut::activated, this, &MainWindow::debuggerChangeState);
    connect(stepInShortcut, &QShortcut::activated, this, &MainWindow::stepInPressed);
    connect(stepOverShortcut, &QShortcut::activated, this, &MainWindow::stepOverPressed);
    connect(stepNextShortcut, &QShortcut::activated, this, &MainWindow::stepNextPressed);
    connect(stepOutShortcut, &QShortcut::activated, this, &MainWindow::stepOutPressed);

    ui->portView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->breakpointView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->watchpointView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    autotester::stepCallback = []() { QApplication::processEvents(); };

    QString portableSettings = qApp->applicationDirPath() + SETTING_DEFAULT_FILE;
    QString localSettings = configPath + SETTING_DEFAULT_FILE;

    if (opts.settingsFile.isEmpty()) {
        if (checkForCEmuBootImage()) {
            pathSettings = localSettings;
        } else if (fileExists(portableSettings)) {
            pathSettings = portableSettings;
            portable = true;
        } else {
            pathSettings = localSettings;
        }
    } else {
        pathSettings = opts.settingsFile;
    }

    settings = new QSettings(pathSettings, QSettings::IniFormat);
    resetSettingsIfLoadedCEmuBootableImage();

    if (portable) {
        ui->checkPortable->setChecked(true);
        ui->buttonChangeSavedDebugPath->setEnabled(false);
        ui->buttonChangeSavedImagePath->setEnabled(false);
        ui->settingsPath->setText(QFile(pathSettings).fileName());
    } else {
        ui->settingsPath->setText(pathSettings);
    }

#ifdef Q_OS_WIN
    installToggleConsole();
#endif

#ifdef Q_OS_MACX
    ui->actionDisableMenuBar->setVisible(false);
#endif

    bool good_version = !checkVersion();

    stopIcon.addPixmap(QPixmap(":/icons/resources/icons/stop.png"));
    runIcon.addPixmap(QPixmap(":/icons/resources/icons/run.png"));
    saveIcon.addPixmap(QPixmap(":/icons/resources/icons/import.png"));
    loadIcon.addPixmap(QPixmap(":/icons/resources/icons/export.png"));
    editIcon.addPixmap(QPixmap(":/icons/resources/icons/wizard.png"));
    removeIcon.addPixmap(QPixmap(":/icons/resources/icons/exit.png"));
    searchIcon.addPixmap(QPixmap(":/icons/resources/icons/search.png"));
    gotoIcon.addPixmap(QPixmap(":/icons/resources/icons/goto.png"));

    setMemoryDocks();
    optLoadFiles(opts);
    setFrameskip(settings->value(SETTING_CAPTURE_FRAMESKIP, 1).toUInt());
    setOptimizeRecording(settings->value(SETTING_CAPTURE_OPTIMIZE, true).toBool());
    setGuiSkip(settings->value(SETTING_SCREEN_FRAMESKIP, 0).toUInt());
    setEmuSpeed(settings->value(SETTING_EMUSPEED, 100).toUInt());
    setFont(settings->value(SETTING_DEBUGGER_TEXT_SIZE, 9).toUInt());
    setAutoCheckForUpdates(settings->value(SETTING_AUTOUPDATE, false).toBool());
    setAutoSaveState(settings->value(SETTING_RESTORE_ON_OPEN, true).toBool());
    setSaveDebug(settings->value(SETTING_DEBUGGER_RESTORE_ON_OPEN, false).toBool());
    setSpaceDisasm(settings->value(SETTING_DEBUGGER_ADD_DISASM_SPACE, false).toBool());
    setDebugResetTrigger(settings->value(SETTING_DEBUGGER_RESET_OPENS, false).toBool());
    setDebugIgnoreBreakpoints(settings->value(SETTING_DEBUGGER_BREAK_IGNORE, false).toBool());
    setDebugSoftCommands(settings->value(SETTING_DEBUGGER_ENABLE_SOFT, true).toBool());
    setDataCol(settings->value(SETTING_DEBUGGER_DATA_COL, true).toBool());
    setLcdSpi(settings->value(SETTING_SCREEN_SPI, true).toBool());
    setLcdDma(settings->value(SETTING_DEBUGGER_IGNORE_DMA, true).toBool());
    setFocusSetting(settings->value(SETTING_PAUSE_FOCUS, false).toBool());
    setRecentSave(settings->value(SETTING_RECENT_SAVE, true).toBool());
    setPreRevisionI(settings->value(SETTING_DEBUGGER_PRE_I, false).toBool());
    ui->flashBytes->setValue(settings->value(SETTING_DEBUGGER_FLASH_BYTES, 8).toInt());
    ui->ramBytes->setValue(settings->value(SETTING_DEBUGGER_RAM_BYTES, 8).toInt());
    ui->memBytes->setValue(settings->value(SETTING_DEBUGGER_MEM_BYTES, 8).toInt());

    currDir.setPath((settings->value(SETTING_CURRENT_DIR, QDir::homePath()).toString()));
    if (settings->value(SETTING_IMAGE_PATH).toString().isEmpty() || portable) {
        QString path = QDir::cleanPath(QFileInfo(settings->fileName()).absoluteDir().absolutePath() + SETTING_DEFAULT_IMAGE_FILE);
        settings->setValue(SETTING_IMAGE_PATH, path);
    }
    ui->savedImagePath->setText(settings->value(SETTING_IMAGE_PATH).toString());
    emu.image = ui->savedImagePath->text();

    if (settings->value(SETTING_DEBUGGER_IMAGE_PATH).toString().isEmpty() || portable) {
        QString path = QDir::cleanPath(QFileInfo(settings->fileName()).absoluteDir().absolutePath() + SETTING_DEFAULT_DEBUG_FILE);
        settings->setValue(SETTING_DEBUGGER_IMAGE_PATH, path);
    }
    ui->savedDebugPath->setText(settings->value(SETTING_DEBUGGER_IMAGE_PATH).toString());

    QString currKeyMap = settings->value(SETTING_KEYPAD_KEYMAP, SETTING_KEYPAD_CEMU).toString();
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

    ui->rompathView->setText(emu.rom);

    debugger_init();

    if (!fileExists(emu.rom)) {
        if (!runSetup()) {
            initPassed = false;
        }
    } else {
        if (settings->value(SETTING_RESTORE_ON_OPEN).toBool()
                && fileExists(emu.image)
                && opts.restoreOnOpen) {
            restoreEmuState();
        } else {
            emu.start();
            if (opts.forceReloadRom) {
                reloadROM();
            }
        }
    }

    colorback.setColor(QPalette::Base, QColor(Qt::yellow).lighter(160));
    consoleFormat = ui->console->currentCharFormat();

    optCheckSend(opts);

    if (opts.speed != -1) {
        setEmuSpeed(opts.speed);
    }

    debuggerInstall();

    setUIDocks();
    setSlotInfo();
    setRecentInfo();

    setUIEditMode(settings->value(SETTING_UI_EDIT_MODE, true).toBool());

    if (settings->value(SETTING_DEBUGGER_RESTORE_ON_OPEN, false).toBool()) {
        if (!opts.debugFile.isEmpty()) {
            debuggerImportFile(opts.debugFile);
        } else {
            debuggerImportFile(settings->value(SETTING_DEBUGGER_IMAGE_PATH).toString());
        }
    }

    if (!good_version) {
        settings->setValue(SETTING_FIRST_RUN, true);
        QMessageBox *box = new QMessageBox();
        box->setWindowTitle(tr("Different CEmu version detected"));
        box->setText(tr("This version of CEmu is not compatible with your settings, probably made by an older version. "
                        "Would you like to erase them to prevent any unexpected behavior?"));
        box->show();
        if (box->exec()) {
            reloadAll();
        }
        setVersion();
    }

    if (isFirstRun() && initPassed && !needFullReset) {
        infoBox = new QMessageBox();
        settings->setValue(SETTING_FIRST_RUN, true);
        infoBox->setWindowTitle(MSG_INFORMATION);
        infoBox->setText(tr("Welcome!\nCEmu uses a customizable dock-style interface. "
                            "Drag and drop to move tabs and windows around on the screen, "
                            "and choose which docks are available in the 'Docks' menu in the topmost bar. "
                            "Be sure that 'Enable UI edit mode' is selected when laying out your interface. "
                            "Enjoy!"));
        infoBox->setWindowModality(Qt::NonModal);
        infoBox->setWindowFlags(infoBox->windowFlags() | Qt::WindowStaysOnTopHint);
        infoBox->setAttribute(Qt::WA_DeleteOnClose);
        infoBox->show();
    }

    QString prefLang = settings->value(SETTING_PREFERRED_LANG, "none").toString();
    if (prefLang != QStringLiteral("none")) {
        switchTranslator(prefLang);
    }

    translateExtras(TRANSLATE_UPDATE);
}

void MainWindow::switchTranslator(const QString& lang) {
    qApp->removeTranslator(&appTranslator);
    // For English, nothing to load after removing the translator.
    if (lang == QStringLiteral("en_EN") || (appTranslator.load(lang, QStringLiteral(":/i18n/i18n/"))
                                            && qApp->installTranslator(&appTranslator))) {
        settings->setValue(SETTING_PREFERRED_LANG, lang);
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
                dock->setWindowTitle(__TXT_MEM_DOCK);
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
        actionToggleUI->setText(MSG_EDIT_UI);
        actionAddMemory->setText(MSG_ADD_MEMORY);
        debugMenu->setTitle(TITLE_DEBUG);
        docksMenu->setTitle(TITLE_DOCKS);

        action = docksMenu->actions().at(0);
        action->setText(TXT_VARIABLES);
        action = docksMenu->actions().at(1);
        action->setText(TXT_CAPTURE);
        action = docksMenu->actions().at(2);
        action->setText(TXT_SETTINGS);
        action = docksMenu->actions().at(3);
        action->setText(TXT_CONSOLE);
        action = docksMenu->actions().at(4);
        action->setText(TXT_STATE);
        action = docksMenu->actions().at(5);
        action->setText(TXT_KEYPAD);

        action = debugMenu->actions().at(0);
        action->setText(TXT_DEBUG_CONTROL);
        action = debugMenu->actions().at(1);
        action->setText(TXT_CPU_STATUS);
        action = debugMenu->actions().at(2);
        action->setText(TXT_DISASSEMBLY);
        action = debugMenu->actions().at(3);
        action->setText(TXT_MEMORY);
        action = debugMenu->actions().at(4);
        action->setText(TXT_TIMERS);
        action = debugMenu->actions().at(5);
        action->setText(TXT_BREAK_WATCH);
        action = debugMenu->actions().at(6);
        action->setText(TXT_OS_VIEW);
        action = debugMenu->actions().at(7);
        action->setText(TXT_MISC);
        action = debugMenu->actions().at(8);
        action->setText(TXT_AUTOTESTER);

#ifdef _WIN32
        actionToggleConsole->setText(TXT_TOGGLE_CONSOLE);
#endif
    }
}

void MainWindow::changeEvent(QEvent* event) {
    const auto eventType = event->type();
    if (eventType == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        translateExtras(TRANSLATE_UPDATE);
    } else if (eventType == QEvent::LocaleChange) {
        switchTranslator(QLocale::system().name());
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::showEvent(QShowEvent *e) {
    if (!firstShow) {
        if (!initPassed) {
            QFile(pathSettings).remove();
            close();
            e->accept();
            return;
        }
        if (needFullReset) {
            close();
            e->accept();
            return;
        }
        ui->lcd->setFocus();
        progressBar->setMaximumHeight(ui->statusBar->height()/2);
        setLcdScale(settings->value(SETTING_SCREEN_SCALE, 100).toUInt());
        setSkinToggle(settings->value(SETTING_SCREEN_SKIN, true).toBool());
        setAlwaysOnTop(settings->value(SETTING_ALWAYS_ON_TOP, false).toBool());
        setMenuBarState(settings->value(SETTING_WINDOW_MENUBAR, false).toBool());
        const QByteArray geometry = settings->value(SETTING_WINDOW_GEOMETRY, QByteArray()).toByteArray();
        if (geometry.isEmpty()) {
            const QRect availableGeometry = QApplication::desktop()->availableGeometry(this);
            resize(availableGeometry.width() / 2, availableGeometry.height() / 2);
            move((availableGeometry.width() - width()) / 2,
                 (availableGeometry.height() - height()) / 2);
        } else {
            restoreGeometry(geometry);
            restoreState(settings->value(SETTING_WINDOW_STATE).toByteArray(), WindowStateVersion);
            if (!isMaximized()) {
                QSize newSize = settings->value(SETTING_WINDOW_SIZE).toSize();

                setMinimumSize(QSize(0, 0));
                setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));

                resize(newSize);
            }
        }
        QList<QDockWidget*> docks = findChildren<QDockWidget*>();
        foreach (QDockWidget* dock, docks) {
            if (dock->windowTitle().contains(TXT_MEM_DOCK)) {
                if (dock->visibleRegion().isEmpty()) {
                    removeDockWidget(dock);
                    memoryDocks--;
                }
            }
        }
        firstShow = true;
    }
    QMainWindow::showEvent(e);
    e->accept();
}

void MainWindow::createMemoryDock(QString title) {
    DockWidget *dw;

    memoryDocks++;

    dw = new DockWidget(title, this);
    dw->setAllowedAreas(Qt::AllDockWidgetAreas);
    dw->setFloating(true);
    dw->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, dw->minimumSize(), qApp->desktop()->availableGeometry()));

    QWidget *widget = new QWidget();
    QVBoxLayout *vlayout = new QVBoxLayout();
    QHBoxLayout *hlayout = new QHBoxLayout();
    QPushButton *buttonGoto = new QPushButton(gotoIcon, tr("Goto"));
    QPushButton *buttonSearch = new QPushButton(searchIcon, tr("Search"));
    QPushButton *buttonSync = new QPushButton(tr("Sync Changes"));
    QSpacerItem *spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Fixed);
    QSpinBox *spin = new QSpinBox();
    QHexEdit *edit = new QHexEdit();

    memory.append(edit);
    int index = memory.size() - 1;

    edit->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(spin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), edit, &QHexEdit::setBytesPerLine);
    connect(edit, &QHexEdit::customContextMenuRequested, this, &MainWindow::memContextMenu);
    connect(buttonSearch, &QPushButton::clicked, this, [this, index]{ memSearchPressed(index); });
    connect(buttonGoto, &QPushButton::clicked, this, [this, index]{ memGotoPressed(index); });
    connect(buttonSync, &QPushButton::clicked, this, [this, index]{ memSyncPressed(index); });

    spin->setValue(8);
    spin->setMaximum(32);
    spin->setMinimum(1);

    hlayout->addWidget(buttonGoto);
    hlayout->addWidget(buttonSearch);
    hlayout->addSpacerItem(spacer);
    hlayout->addWidget(buttonSync);
    hlayout->addWidget(spin);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(edit);
    widget->setLayout(vlayout);
    dw->setWidget(widget);

    buttonGoto->setEnabled(guiDebug);
    buttonSearch->setEnabled(guiDebug);
    buttonSync->setEnabled(guiDebug);
    spin->setEnabled(guiDebug);
    edit->setEnabled(guiDebug);

    if (guiDebug) {
        memUpdate(index, 0);
    }

    addDockWidget(Qt::RightDockWidgetArea, dw);
    dw->setVisible(true);
}

void MainWindow::toggleKeyHistory() {
    if (!keyHistoryWindow) {
        keyHistoryWindow = new KeyHistory(this);
        connect(ui->keypadWidget, &KeypadWidget::keyPressed, keyHistoryWindow, &KeyHistory::addEntry);
        keyHistoryWindow->show();
        ui->actionKeyHistory->setChecked(true);
    } else {
        disconnect(ui->keypadWidget, &KeypadWidget::keyPressed, keyHistoryWindow, &KeyHistory::addEntry);
        keyHistoryWindow->close();
        delete keyHistoryWindow;
        keyHistoryWindow = Q_NULLPTR;
        ui->actionKeyHistory->setChecked(false);
    }
}

void MainWindow::optCheckSend(CEmuOpts &o) {
    if (!o.autotesterFile.isEmpty()) {
        if (!openJSONConfig(o.autotesterFile)) {
           if (!o.deforceReset) { resetCalculator(); }
           setEmuSpeed(100);

           // Race condition requires this
           guiDelay(o.deforceReset ? 100 : 2000);
           launchTest();
        }
    }

    if (!o.sendFiles.isEmpty() || !o.sendArchFiles.isEmpty() || !o.sendRAMFiles.isEmpty()) {
        if (!o.deforceReset) { resetCalculator(); }
        setEmuSpeed(100);

        // Race condition requires this
        guiDelay(o.deforceReset ? 100 : 2000);
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
}

void MainWindow::optLoadFiles(CEmuOpts &o) {
    if (o.romFile.isEmpty()) {
        if (loadedCEmuBootImage) {
            emu.rom = configPath + SETTING_DEFAULT_ROM_FILE;
            settings->setValue(SETTING_ROM_PATH, emu.rom);
        } else {
            emu.rom = settings->value(SETTING_ROM_PATH).toString();
        }
    } else {
        emu.rom = o.romFile;
    }

    if (!o.imageFile.isEmpty()) {
        if (fileExists(o.imageFile)) {
            emu.image = o.imageFile;
        }
    }
}

void MainWindow::optAttemptLoad(CEmuOpts &o) {
    if (!fileExists(emu.rom)) {
        if (!runSetup()) {
            initPassed = false;
            close();
        }
    } else {
        if (o.restoreOnOpen && !o.imageFile.isEmpty() && fileExists(emu.image)) {
            restoreEmuState();
        } else {
            if (o.forceReloadRom) {
                reloadROM();
                guiDelay(500);
            }
        }
    }
}

MainWindow::~MainWindow() {
    delete com;
    delete settings;
    delete ui;
}

bool MainWindow::IsInitialized() {
    return initPassed;
}

void MainWindow::reloadAll() {
    ipcCloseOthers();
    needReload = true;
    needFullReset = true;
    close();
}

void MainWindow::reloadGui() {
    ipcCloseOthers();
    settings->remove(SETTING_WINDOW_GEOMETRY);
    settings->remove(SETTING_WINDOW_MEMORY_DOCKS);
    settings->remove(SETTING_WINDOW_SIZE);
    settings->remove(SETTING_WINDOW_STATE);
    settings->remove(SETTING_WINDOW_MENUBAR);
    settings->remove(SETTING_WINDOW_SEPARATOR);
    needReload = true;
    close();
}

bool MainWindow::IsReload() {
    emu_thread = Q_NULLPTR;
    return needReload;
}

void MainWindow::CheckResetAll() {
    if (needFullReset) {
        QDir dir(configPath);
        dir.removeRecursively();
    }
}

void MainWindow::sendASMKey() {
    autotester::sendKey(0x9CFC); // "Asm("
}

bool MainWindow::restoreEmuState() {
    QString defaultImage = settings->value(SETTING_IMAGE_PATH).toString();
    if (!defaultImage.isEmpty()) {
        return restoreFromPath(defaultImage);
    } else {
        QMessageBox::critical(this, MSG_ERROR, tr("No saved image path in settings."));
        return false;
    }
}

void MainWindow::saveToPath(const QString &path) {
    emu.saveImage(path);
}

bool MainWindow::restoreFromPath(const QString &path) {
    if (guiReceive) {
        receiveChangeState();
    }

    if (guiDebug) {
        debuggerChangeState();
    }

    guiEmuValid = false;

    if (!emu.restore(path)) {
        return false;
    }

    return true;
}

void MainWindow::saveEmuState() {
    QString default_savedImage = settings->value(SETTING_IMAGE_PATH).toString();
    if (!default_savedImage.isEmpty()) {
        saveToPath(default_savedImage);
    } else {
        QMessageBox::warning(this, MSG_WARNING, tr("No saved image path in settings given."));
    }
}

void MainWindow::restoreFromFile() {
    QString savedImage = QFileDialog::getOpenFileName(this, tr("Select saved image to restore from"),
                                                      currDir.absolutePath(),
                                                      tr("CEmu images (*.ce);;All files (*.*)"));
    if (!savedImage.isEmpty()) {
        currDir = QFileInfo(savedImage).absoluteDir();
        if (!restoreFromPath(savedImage)) {
            QMessageBox::critical(this, MSG_ERROR, tr("Could not resume; try restarting CEmu"));
        }
    }
}

void MainWindow::saveToFile() {
    QString savedImage = QFileDialog::getSaveFileName(this, tr("Set image to save to"),
                                                      currDir.absolutePath(),
                                                      tr("CEmu images (*.ce);;All files (*.*)"));
    if (!savedImage.isEmpty()) {
        currDir = QFileInfo(savedImage).absoluteDir();
        saveToPath(savedImage);
    }
}
void MainWindow::exportRom() {
    QString saveRom = QFileDialog::getSaveFileName(this, tr("Set Rom image to save to"),
                                                      currDir.absolutePath(),
                                                      tr("ROM images (*.rom);;All files (*.*)"));
    if (!saveRom.isEmpty()) {
        currDir = QFileInfo(saveRom).absoluteDir();
        emu.saveRom(saveRom);
    }
}

void MainWindow::started(bool success) {
    guiEmuValid = success;
    if (success) {
        ui->lcd->setup();
        setCalcSkinTopFromType();
        setKeypadColor(settings->value(SETTING_KEYPAD_COLOR, get_device_type() ? KEYPAD_WHITE : KEYPAD_BLACK).toUInt());
    } else {
        QMessageBox::critical(this, MSG_ERROR, tr("Could not load ROM image. Please see console for more information."));
    }
}

void MainWindow::restored(bool success) {
    guiEmuValid = success;
    if (success) {
        ui->lcd->setup();
        setCalcSkinTopFromType();
        setKeypadColor(settings->value(SETTING_KEYPAD_COLOR, get_device_type() ? KEYPAD_WHITE : KEYPAD_BLACK).toUInt());
    } else {
        QMessageBox::critical(this, MSG_ERROR, tr("Resuming failed.\nPlease reload the ROM from the 'Calculator' menu."));
    }
}

void MainWindow::saved(bool success) {
    if (!success) {
        QMessageBox::warning(this, MSG_WARNING, tr("Saving failed.\nSaving failed! Please let someone who maintains this know."));
    }

    if (closeAfterSave) {
        if (!success) {
            closeAfterSave = false;
        } else {
            close();
        }
    }
}

void MainWindow::dropEvent(QDropEvent *e) {
    if (isSendingROM) {
        setRom(dragROM);
    } else {
        sendingHandler->dropOccured(e, LINK_FILE);
    }
    equatesRefresh();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e) {

    // check if we are dragging a rom file
    dragROM = sendingROM(e, &isSendingROM);

    if (!isSendingROM) {
        sendingHandler->dragOccured(e);
    }
}

void MainWindow::closeEvent(QCloseEvent *e) {
    // shut down ipc server
    com->idClose();

    // close key history window
    if (keyHistoryWindow) {
        keyHistoryWindow->close();
    }

    if (!initPassed) {
        QMainWindow::closeEvent(e);
        return;
    }

    // ignore debug requests
    debugger.ignore = true;

    if (guiDebug) {
        debuggerChangeState();
    }

    if (guiReceive) {
        receiveChangeState();
    }

    if (settings->value(SETTING_DEBUGGER_SAVE_ON_CLOSE, false).toBool()) {
        debuggerExportFile(settings->value(SETTING_DEBUGGER_IMAGE_PATH).toString());
    }

    if (!closeAfterSave && settings->value(SETTING_SAVE_ON_CLOSE).toBool()) {
            closeAfterSave = true;
            saveEmuState();
            e->ignore();
            return;
    }

    emu.stop();

    saveMiscSettings();
    debugger_free();

    QMainWindow::closeEvent(e);
}

void MainWindow::consoleAppend(const QString &str, const QColor &color) {
    QTextCursor cur(ui->console->document());
    cur.movePosition(QTextCursor::End);
    consoleFormat.setForeground(color);
    cur.insertText(str, consoleFormat);
    if (ui->checkAutoScroll->isChecked()) {
        ui->console->setTextCursor(cur);
    }
}

void MainWindow::consoleStr(const QString &str) {
    if (str.at(0) == '\f') {
        if (nativeConsole) {
#ifdef _WIN32
            int ret = system("cls");
#else
            int ret = system("clear");
#endif
            if (ret == -1) {
                ui->console->clear();
            }
        } else {
            ui->console->clear();
        }
    } else {
        if (nativeConsole) {
            fputs(str.toStdString().c_str(), stdout);
        } else {
            consoleAppend(str);
        }
    }
}

void MainWindow::consoleErrStr(const QString &str) {
    if (str.at(0) == '\f') {
        if (nativeConsole) {
#ifdef _WIN32
            int ret = system("cls");
#else
            int ret = system("clear");
#endif
            if (ret == -1) {
                ui->console->clear();
            }
        } else {
            ui->console->clear();
        }
    } else {
        if (nativeConsole) {
            fputs(str.toStdString().c_str(), stderr);
        } else {
            consoleAppend(str, Qt::red);
        }
    }
}

void MainWindow::showEmuUpdates(int speed, double fps, double realFps) {
    QString label = " " + tr("Emulated Speed: ") + QString::number(speed, 10) + "% | FPS: " + QString::number(realFps, 'f', 2);
    speedLabel.setText(label);
    ui->maxFps->setText(tr("Actual FPS: ") + QString::number(fps, 'f', 2));
}

void MainWindow::showStatusMsg(const QString &str) {
    msgLabel.setText(str);
}

void MainWindow::setRom(const QString &name) {
    emu.rom = name;
    if (portable) {
        QDir dir(qApp->applicationDirPath());
        emu.rom = dir.relativeFilePath(emu.rom);
    }

    reloadROM();
    ui->rompathView->setText(emu.rom);
    settings->setValue(SETTING_ROM_PATH, emu.rom);
}

bool MainWindow::runSetup() {
    RomSelection *romWizard = new RomSelection();
    romWizard->setWindowModality(Qt::NonModal);
    romWizard->exec();

    const QString romPath = romWizard->getRomPath();

    delete romWizard;

    if (romPath.isEmpty()) {
        return false;
    } else {
        setRom(romPath);
    }

    return true;
}

void MainWindow::screenshotSave(const QString& nameFilter, const QString& defaultSuffix, const QString& temppath) {
    QFileDialog dialog(this);

    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setDirectory(currDir);
    dialog.setNameFilter(nameFilter);
    dialog.setWindowTitle("Save Screen");
    dialog.setDefaultSuffix(defaultSuffix);
    dialog.exec();

    if (!(dialog.selectedFiles().isEmpty())) {
        QString filename = dialog.selectedFiles().first();
        if (filename.isEmpty()) {
            QFile(temppath).remove();
        } else {
            QFile(filename).remove();
            QFile(temppath).rename(filename);
        }
    }
    currDir = dialog.directory().absolutePath();
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
        if (recordingAnimated) {
            return;
        }
        path = QDir::tempPath() + QDir::separator() + QStringLiteral("apng_tmp.png");
        apng_start(path.toStdString().c_str(), ui->guiSkip->value(), ui->apngSkip->value());
        showStatusMsg(tr("Recording..."));
    } else {
        showStatusMsg(QStringLiteral("Saving Recording..."));
        if (apng_stop()) {
            int res;

            QFileDialog dialog(this);

            dialog.setAcceptMode(QFileDialog::AcceptSave);
            dialog.setFileMode(QFileDialog::AnyFile);
            dialog.setDirectory(currDir);
            dialog.setNameFilter(tr("PNG images (*.png)"));
            dialog.setWindowTitle(tr("Save Recorded PNG"));
            dialog.setDefaultSuffix(QStringLiteral("png"));
            res = dialog.exec();

            QFile(path).remove();
            currDir = dialog.directory();
            path.clear();

            if (res == QDialog::Accepted) {
                QString filename = dialog.selectedFiles().first();
                saveAnimated(filename);
            } else {
                updateAnimatedControls();
            }
        } else {
            QMessageBox::critical(this, MSG_ERROR, tr("A failure occured during PNG recording."));
            showStatusMsg(QStringLiteral(""));
            path.clear();
        }
        return;
    }

    recordingAnimated = true;
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
    thread->filename = filename;
    thread->optimize = optimizeRecording;
    thread->start();
}

void MainWindow::updateAnimatedControls() {
    recordingAnimated = false;
    ui->apngSkip->setEnabled(true);
    ui->actionRecordAnimated->setEnabled(true);
    ui->buttonRecordAnimated->setEnabled(true);
    ui->actionRecordAnimated->setChecked(false);
    ui->buttonRecordAnimated->setText(tr("Record"));
    ui->actionRecordAnimated->setText(tr("Record animated PNG..."));
    showStatusMsg(QStringLiteral(""));
}

void RecordingThread::run() {
    apng_save(filename.toStdString().c_str(), optimize);
    emit done();
}
#endif

void MainWindow::showAbout() {
    QMessageBox *aboutBox = new QMessageBox(this);
    aboutBox->setIconPixmap(QPixmap(":/icons/resources/icons/icon.png"));
    aboutBox->setWindowTitle(tr("About CEmu"));

    QAbstractButton *buttonUpdateCheck = aboutBox->addButton(tr("Check for updates"), QMessageBox::ActionRole);
    connect(buttonUpdateCheck, &QAbstractButton::clicked, this, [=](){ this->checkForUpdates(true); });

    QAbstractButton *okButton = aboutBox->addButton(QMessageBox::Ok);
    okButton->setFocus();

    aboutBox->setText(tr("<h3>CEmu %1</h3>"
                         "<a href='https://github.com/CE-Programming/CEmu'>On GitHub</a><br>"
                         "<br>"
                         "Main authors:<br>"
                         "Matt Waltz (<a href='https://github.com/mateoconlechuga'>MateoConLechuga</a>)<br>"
                         "Jacob Young (<a href='https://github.com/jacobly0'>jacobly0</a>)<br>"
                         "Adrien Bertrand (<a href='https://github.com/adriweb'>adriweb</a>)<br>"
                         "<br>"
                         "Other contributors:<br>"
                         "Lionel Debroux (<a href='https://github.com/debrouxl'>debrouxl</a>)<br>"
                         "Fabian Vogt (<a href='https://github.com/Vogtinator'>Vogtinator</a>)<br>"
                         "<br>"
                         "Many thanks to the <a href='https://github.com/KnightOS/z80e'>z80e</a> (MIT license <a href='https://github.com/KnightOS/z80e/blob/master/LICENSE'>here</a>) and <a href='https://github.com/nspire-emus/firebird'>Firebird</a> (GPLv3 license <a href='https://github.com/nspire-emus/firebird/blob/master/LICENSE'>here</a>) projects.<br>In-program icons are courtesy of the <a href='http://www.famfamfam.com/lab/icons/silk/'>Silk iconset</a>.<br>"
                         "<br>"
                         "This work is licensed under the GPLv3.<br>"
                         "To view a copy of this license, visit <a href='https://www.gnu.org/licenses/gpl-3.0.html'>https://www.gnu.org/licenses/gpl-3.0.html</a>")
                         .arg(QStringLiteral(CEMU_VERSION)));
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
    menu.addMenu(docksMenu);
    menu.addMenu(ui->menuAbout);
    menu.exec(globalPos);
}

void MainWindow::consoleOutputChanged() {
    nativeConsole = ui->radioStderr->isChecked();
}

void MainWindow::pauseEmu(Qt::ApplicationState state) {
    if (pauseOnFocus) {
        if (state == Qt::ApplicationInactive) {
            emit changedEmuSpeed(0);
        }
        if (state == Qt::ApplicationActive) {
            setEmuSpeed(settings->value(SETTING_EMUSPEED).toInt());
        }
    }
}

// ------------------------------------------------
//  Linking things
// ------------------------------------------------

void MainWindow::receiveChangeState() {
    if (guiReceive) {
        changeVariableList();
        emit receiveDone();
    } else {
        emit receive();
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
    dialog.setDirectory(currDir);
    dialog.setNameFilter(name_filter);
    good = dialog.exec();

    currDir = dialog.directory().absolutePath();

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
        BasicCodeViewerWindow *codePopup = new BasicCodeViewerWindow(this);
        codePopup->setVariableName(ui->emuVarView->item(item->row(), VAR_NAME)->text());
        codePopup->setWindowModality(Qt::NonModal);
        codePopup->setAttribute(Qt::WA_DeleteOnClose);
        codePopup->setOriginalCode(QString::fromStdString(calc_var_content_string(var)));
        codePopup->show();
    }
}

void MainWindow::changeVariableList() {
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
                    var_type_str += " (ASM)";
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

    dialog.setDirectory(currDir);
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
    currDir = dialog.directory().absolutePath();

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
            filename = dialog.directory().absolutePath() + "/" + name + "." + var_extension[var.type1];

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

    dialog.setDirectory(currDir);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(QStringLiteral("JSON config (*.json)"));

    good = dialog.exec();
    currDir = dialog.directory().absolutePath();

    if (!good) {
        return;
    }

    openJSONConfig(dialog.selectedFiles().first());
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
        guiDelay(3000);
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

void MainWindow::emuStopped() {
    guiEmuValid = false;
}

void MainWindow::resetCalculator() {
    if (guiReceive) {
        receiveChangeState();
    }
    if (guiDebug) {
        debuggerChangeState();
    }

    emit reset();
}

void MainWindow::reloadROM() {
    if (guiReceive) {
        receiveChangeState();
    }

    if (guiDebug) {
        debuggerChangeState();
    }

    guiEmuValid = false;

    emit load();
}

void MainWindow::drawNextDisassembleLine() {
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
                formattedLine = QString("<pre><b><font color='#444'>%1</font></b>     %2</pre>")
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
            breakpointSymbols = QString("<font color='#A3FFA3'>%1</font><font color='#A3A3FF'>%2</font><font color='#FFA3A3'>%3</font>")
                                        .arg((disasmHighlight.rWatch  ? "&#9679;" : " "),
                                             (disasmHighlight.wWatch ? "&#9679;" : " "),
                                             (disasmHighlight.xBreak  ? "&#9679;" : " "));

            // Simple syntax highlighting
            instructionArgsHighlighted = QString::fromStdString(disasm.instruction.arguments)
                                                  .replace(QRegularExpression("(\\$[0-9a-fA-F]+)"), "<font color='green'>\\1</font>") // hex numbers
                                                  .replace(QRegularExpression("(^\\d)"), "<font color='blue'>\\1</font>")             // dec number
                                                  .replace(QRegularExpression("([()])"), "<font color='#600'>\\1</font>");            // parentheses

            formattedLine = QString("<pre><b><font color='#444'>%1</font></b> %2 %3  <font color='darkblue'>%4%5</font>%6</pre>")
                                    .arg(int2hex(disasm.baseAddress, 6),
                                        breakpointSymbols,
                                        QString::fromStdString(disasm.instruction.data).leftJustified(12, ' '),
                                        QString::fromStdString(disasm.instruction.opcode),
                                        QString::fromStdString(disasm.instruction.modeSuffix),
                                        instructionArgsHighlighted);

            ui->disassemblyView->appendHtml(formattedLine);
        }
    }

    if (!disasmOffsetSet && disasm.newAddress > addressPane) {
        disasmOffsetSet = true;
        disasmOffset = ui->disassemblyView->textCursor();
        disasmOffset.movePosition(QTextCursor::StartOfLine);
    }

    if (disasmHighlight.pc == true) {
        ui->disassemblyView->addHighlight(QColor(Qt::blue).lighter(160));
    }
}

void MainWindow::disasmContextMenu(const QPoint& posa) {
    QString set_pc = tr("Set PC");
    QString toggle_break = tr("Toggle Breakpoint");
    QString toggle_write_watch = tr("Toggle Write Watchpoint");
    QString toggle_read_watch = tr("Toggle Read Watchpoint");
    QString toggle_rw_watch = tr("Toggle Read/Write Watchpoint");
    QString run_until = tr("Run Until");
    QString goto_mem = tr("Goto Memory View");
    ui->disassemblyView->setTextCursor(ui->disassemblyView->cursorForPosition(posa));
    QPoint globalPos = ui->disassemblyView->mapToGlobal(posa);

    QMenu menu;
    menu.addAction(run_until);
    menu.addSeparator();
    menu.addAction(toggle_break);
    menu.addAction(toggle_read_watch);
    menu.addAction(toggle_write_watch);
    menu.addAction(toggle_rw_watch);
    menu.addSeparator();
    menu.addAction(goto_mem);
    menu.addAction(set_pc);

    QAction *item = menu.exec(globalPos);
    if (item) {
        if (item->text() == set_pc) {
            ui->pcregView->setText(ui->disassemblyView->getSelectedAddress());
            uint32_t address = static_cast<uint32_t>(hex2int(ui->pcregView->text()));
            debug_set_pc_address(address);
            updateDisasmView(cpu.registers.PC, true);
        } else if (item->text() == toggle_break) {
            breakpointGUIAdd();
        } else if (item->text() == toggle_read_watch) {
            watchpointReadGUIAdd();
        } else if (item->text() == toggle_write_watch) {
            watchpointWriteGUIAdd();
        } else if (item->text() == toggle_rw_watch) {
            watchpointReadWriteGUIAdd();
        } else if (item->text() == run_until) {
            uint32_t address = static_cast<uint32_t>(hex2int(ui->disassemblyView->getSelectedAddress()));
            debug_init_run_until(address);
            debuggerChangeState();
            emit setRunUntilMode();
        } else if (item->text() == goto_mem) {
            memGoto(MEM_MEM, ui->disassemblyView->getSelectedAddress());
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

void MainWindow::stepInPressed() {
    if (!guiDebug) {
        return;
    }

    debuggerUpdateChanges();
    disconnect(stepInShortcut, &QShortcut::activated, this, &MainWindow::stepInPressed);
    emit setDebugStepInMode();
}

void MainWindow::stepOverPressed() {
    if (!guiDebug) {
        return;
    }

    debuggerUpdateChanges();
    disconnect(stepOverShortcut, &QShortcut::activated, this, &MainWindow::stepOverPressed);
    emit setDebugStepOverMode();
}

void MainWindow::stepNextPressed() {
    if (!guiDebug) {
        return;
    }

    debuggerUpdateChanges();
    disconnect(stepNextShortcut, &QShortcut::activated, this, &MainWindow::stepNextPressed);
    emit setDebugStepNextMode();
}

void MainWindow::stepOutPressed() {
    if (!guiDebug) {
        return;
    }

    debuggerUpdateChanges();
    disconnect(stepOutShortcut, &QShortcut::activated, this, &MainWindow::stepOutPressed);
    emit setDebugStepOutMode();
}

void MainWindow::forceEnterDebug() {
    int count = 0;
    if (!guiDebug) {
        debuggerChangeState();
    }
    while (!guiDebug && count < 20) {
        guiDelay(50);
        count++;
    }
}

void MainWindow::consoleContextMenu(const QPoint &posa) {
    bool ok = true;

    QString goto_mem = tr("Goto Memory View");
    QString goto_disasm = tr("Goto Disassembly View");
    QString toggle_break = tr("Toggle Breakpoint");
    QString toggle_write_watch = tr("Toggle Write Watchpoint");
    QString toggle_read_watch = tr("Toggle Read Watchpoint");
    QString toggle_rw_watch = tr("Toggle Read/Write Watchpoint");
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
        menu.addAction(goto_mem);
        menu.addAction(goto_disasm);
        menu.addSeparator();
        menu.addAction(toggle_break);
        menu.addAction(toggle_read_watch);
        menu.addAction(toggle_write_watch);
        menu.addAction(toggle_rw_watch);

        QAction *item = menu.exec(globalp);
        if (item) {
            if (item->text() == goto_mem) {
                forceEnterDebug();
                memGoto(MEM_MEM, cursor.selectedText());
            } else if (item->text() == goto_disasm) {
                forceEnterDebug();
                forceGotoDisasm(address);
            } else if (item->text() == toggle_break) {
                breakpointAdd(breakpointNextLabel(), address, true, true);
            } else if (item->text() == toggle_read_watch) {
                watchpointAdd(watchpointNextLabel(), address, 1, DBG_READ_WATCHPOINT, true);
            } else if (item->text() == toggle_write_watch) {
                watchpointAdd(watchpointNextLabel(), address, 1, DBG_WRITE_WATCHPOINT, true);
            } else if (item->text() == toggle_rw_watch) {
                watchpointAdd(watchpointNextLabel(), address, 1, DBG_WRITE_WATCHPOINT | DBG_READ_WATCHPOINT, true);
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
    if (com->ipcSetup(opts.idString, opts.pidString)) {
        consoleStr(QStringLiteral("[CEmu] Initialized Server [") + opts.idString + QStringLiteral(" | ") + com->getServerName() + QStringLiteral("]\n"));
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
    com->send(byteArray);
    return false;
}

void MainWindow::ipcHandleCommandlineReceive(QDataStream &stream) {
    consoleStr("[CEmu] Received IPC: command line options\n");
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
    optCheckSend(o);
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

                    com->clientSetup(pid);
                    com->send(byteArray);
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
    QByteArray byteArray(com->getData());

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
           consoleStr("[CEmu] IPC Unknown\n");
           break;
    }
}

void MainWindow::ipcChangeID() {
    bool ok = true;
    QString text = QInputDialog::getText(this, tr("CEmu Change ID"), tr("New ID:"), QLineEdit::Normal, opts.idString, &ok);
    if (ok && !text.isEmpty() && text != opts.idString) {
        if (!ipc::idOpen(text)) {
            com->idClose();
            com->ipcSetup(opts.idString = text, opts.pidString);
            consoleStr(QStringLiteral("[CEmu] Initialized Server [") + opts.idString + QStringLiteral(" | ") + com->getServerName() + QStringLiteral("]\n"));
            setWindowTitle(QStringLiteral("CEmu | ") + opts.idString);
        }
    }
}

void MainWindow::ipcSpawnRandom() {
    QStringList arguments;
    arguments << "--id" << randomString(15);

    QProcess *myProcess = new QProcess(this);
    myProcess->startDetached(execPath, arguments);
}

void MainWindow::slotAddNew() {
    QString name = randomString(6);
    QString path = QDir::cleanPath(QFileInfo(settings->fileName()).absoluteDir().absolutePath() + QStringLiteral("/") + name + QStringLiteral(".ce"));
    slotAdd(name, path);
}

void MainWindow::slotAdd(QString &name, QString &path) {
    const int row = ui->slotView->rowCount();

    QToolButton *btnLoad   = new QToolButton();
    QToolButton *btnSave   = new QToolButton();
    QToolButton *btnEdit   = new QToolButton();
    QToolButton *btnRemove = new QToolButton();

    btnLoad->setIcon(loadIcon);
    btnSave->setIcon(saveIcon);
    btnEdit->setIcon(editIcon);
    btnRemove->setIcon(removeIcon);

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
    if (!restoreFromPath(path)) {
        QMessageBox::critical(this, MSG_ERROR, tr("Could not restore image!"));
    }
}

const char *MainWindow::var_extension[] = {
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
