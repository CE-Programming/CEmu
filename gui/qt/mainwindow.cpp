#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkAccessManager>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QProgressDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QScrollBar>
#include <QtNetwork/QNetworkReply>
#include <fstream>

#ifdef _MSC_VER
    #include <direct.h>
    #define chdir _chdir
#else
    #include <unistd.h>
#endif

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "sendinghandler.h"
#include "lcdpopout.h"
#include "dockwidget.h"
#include "searchwidget.h"
#include "basiccodeviewerwindow.h"
#include "utils.h"
#include "capture/gif.h"

#include "../../core/schedule.h"
#include "../../core/link.h"

#include "../../tests/autotester/crc32.hpp"
#include "../../tests/autotester/autotester.h"
#include "../../tests/autotester/autotester.h"

static const constexpr int WindowStateVersion = 0;

MainWindow::MainWindow(CEmuOpts cliOpts,QWidget *p) : QMainWindow(p), ui(new Ui::MainWindow), opts(cliOpts) {

    // Setup the UI
    ui->setupUi(this);
    ui->centralWidget->hide();
    ui->statusBar->addWidget(&statusLabel);
    ui->lcdWidget->setLCD(&lcd);

    // Allow for 2000 lines of logging
    ui->console->setMaximumBlockCount(2000);

    // Register QtKeypadBridge for the virtual keyboard functionality
    connect(&keypadBridge, &QtKeypadBridge::keyStateChanged, ui->keypadWidget, &KeypadWidget::changeKeyState);
    installEventFilter(&keypadBridge);
    ui->lcdWidget->installEventFilter(&keypadBridge);

    // Same for all the tabs/docks (iterate over them instead of hardcoding their names)
    for (const auto& tab : ui->tabWidget->children()[0]->children()) {
        tab->installEventFilter(&keypadBridge);
    }

    // Emulator -> GUI
    connect(&emu, &EmuThread::consoleStr, this, &MainWindow::consoleStr);
    connect(&emu, &EmuThread::consoleErrStr, this, &MainWindow::consoleErrStr);
    connect(&emu, &EmuThread::started, this, &MainWindow::started, Qt::QueuedConnection);
    connect(&emu, &EmuThread::restored, this, &MainWindow::restored, Qt::QueuedConnection);
    connect(&emu, &EmuThread::saved, this, &MainWindow::saved, Qt::QueuedConnection);
    connect(&emu, &EmuThread::isBusy, this, &MainWindow::isBusy, Qt::QueuedConnection);

    // Console actions
    connect(ui->buttonConsoleclear, &QPushButton::clicked, ui->console, &QPlainTextEdit::clear);
    connect(ui->radioConsole, &QRadioButton::clicked, this, &MainWindow::consoleOutputChanged);
    connect(ui->radioStderr, &QRadioButton::clicked, this, &MainWindow::consoleOutputChanged);

    // Debugger
    connect(&emu, &EmuThread::raiseDebugger, this, &MainWindow::debuggerRaise, Qt::QueuedConnection);
    connect(&emu, &EmuThread::disableDebugger, this, &MainWindow::debuggerGUIDisable, Qt::QueuedConnection);
    connect(&emu, &EmuThread::sendDebugCommand, this, &MainWindow::debuggerProcessCommand, Qt::QueuedConnection);
    connect(this, &MainWindow::debuggerSendNewState, &emu, &EmuThread::setDebugMode);
    connect(this, &MainWindow::setDebugStepInMode, &emu, &EmuThread::setDebugStepInMode);
    connect(this, &MainWindow::setRunUntilMode, &emu, &EmuThread::setRunUntilMode);
    connect(this, &MainWindow::setDebugStepOverMode, &emu, &EmuThread::setDebugStepOverMode);
    connect(this, &MainWindow::setDebugStepNextMode, &emu, &EmuThread::setDebugStepNextMode);
    connect(this, &MainWindow::setDebugStepOutMode, &emu, &EmuThread::setDebugStepOutMode);
    connect(ui->buttonRun, &QPushButton::clicked, this, &MainWindow::debuggerChangeState);

    connect(ui->tabDebugging, &QTabWidget::currentChanged, this, &MainWindow::debuggerTabSwitched);
    connect(ui->buttonAddPort, &QPushButton::clicked, this, &MainWindow::portSlotAdd);
    connect(ui->buttonRemovePort, &QPushButton::clicked, this, &MainWindow::portRemoveSelected);
    connect(ui->buttonAddBreakpoint, &QPushButton::clicked, this, &MainWindow::breakpointSlotAdd);
    connect(ui->buttonRemoveBreakpoint, &QPushButton::clicked, this, &MainWindow::breakpointRemoveSelectedRow);
    connect(ui->buttonAddWatchpoint, &QPushButton::clicked, this, &MainWindow::watchpointSlotAdd);
    connect(ui->buttonRemoveWatchpoint, &QPushButton::clicked, this, &MainWindow::watchpointRemoveSelectedRow);
    connect(ui->buttonStepIn, &QPushButton::clicked, this, &MainWindow::stepInPressed);
    connect(ui->buttonStepOver, &QPushButton::clicked, this, &MainWindow::stepOverPressed);
    connect(ui->buttonStepNext, &QPushButton::clicked, this, &MainWindow::stepNextPressed);
    connect(ui->buttonStepOut, &QPushButton::clicked, this, &MainWindow::stepOutPressed);
    connect(ui->buttonGoto, &QPushButton::clicked, this, &MainWindow::gotoPressed);
    connect(ui->disassemblyView, &QWidget::customContextMenuRequested, this, &MainWindow::disasmContextMenu);
    connect(ui->vatView, &QWidget::customContextMenuRequested, this, &MainWindow::vatContextMenu);
    connect(ui->opView, &QWidget::customContextMenuRequested, this, &MainWindow::opContextMenu);
    connect(ui->portView, &QTableWidget::itemChanged, this, &MainWindow::portDataChanged);
    connect(ui->portView, &QTableWidget::itemPressed, this, &MainWindow::portSetPreviousAddress);
    connect(ui->breakpointView, &QTableWidget::itemChanged, this, &MainWindow::breakpointDataChanged);
    connect(ui->breakpointView, &QTableWidget::itemPressed, this, &MainWindow::breakpointSetPreviousAddress);
    connect(ui->watchpointView, &QTableWidget::itemChanged, this, &MainWindow::watchpointDataChanged);
    connect(ui->watchpointView, &QTableWidget::itemPressed, this, &MainWindow::watchpointSetPreviousAddress);
    connect(ui->profilerView, &QTableWidget::itemChanged, this, &MainWindow::profilerDataChange);
    connect(ui->checkCharging, &QCheckBox::toggled, this, &MainWindow::batteryIsCharging);
    connect(ui->sliderBattery, &QSlider::valueChanged, this, &MainWindow::batteryChangeStatus);
    connect(ui->checkAddSpace, &QCheckBox::stateChanged, this, &MainWindow::setSpaceDisasm);
    connect(ui->buttonZero, &QPushButton::clicked, this, &MainWindow::debuggerZeroClockCounter);
    connect(ui->buttonAddProfiler, &QPushButton::clicked, this, &MainWindow::profilerSlotAdd);
    connect(ui->buttonRemoveProfiler, &QPushButton::clicked, this, &MainWindow::profilerRemoveSelected);
    connect(ui->buttonResetProfiler, &QPushButton::clicked, this, &MainWindow::profilerZero);
    connect(ui->buttonExportProfiler, &QPushButton::clicked, this, &MainWindow::profilerExport);

    // Debugger Options
    connect(ui->buttonAddEquateFile, &QPushButton::clicked, this, &MainWindow::equatesAddDialog);
    connect(ui->buttonClearEquates, &QPushButton::clicked, this, &MainWindow::equatesClear);
    connect(ui->buttonRefreshEquates, &QPushButton::clicked, this, &MainWindow::equatesRefresh);
    connect(ui->textSizeSlider, &QSlider::valueChanged, this, &MainWindow::setFont);
    connect(ui->comboGranularity, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::profilerChangeGranularity);

    // Debugging files
    connect(ui->actionImportDebugger, &QAction::triggered, this, &MainWindow::debuggerImport);
    connect(ui->actionExportDebugger, &QAction::triggered, this, &MainWindow::debuggerExport);

    // Linking
    connect(ui->buttonSend, &QPushButton::clicked, this, &MainWindow::selectFiles);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::selectFiles);
    connect(ui->buttonRefreshList, &QPushButton::clicked, this, &MainWindow::refreshVariableList);
    connect(this, &MainWindow::setReceiveState, &emu, &EmuThread::setReceiveState);
    connect(ui->buttonReceiveFiles, &QPushButton::clicked, this, &MainWindow::saveSelected);

    // Autotester
    connect(ui->buttonOpenJSONconfig, &QPushButton::clicked, this, &MainWindow::prepareAndOpenJSONConfig);
    connect(ui->buttonReloadJSONconfig, &QPushButton::clicked, this, &MainWindow::reloadJSONConfig);
    connect(ui->buttonLaunchTest, &QPushButton::clicked, this, &MainWindow::launchTest);
    connect(ui->comboBoxPresetCRC, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::updateCRCParamsFromPreset);
    connect(ui->buttonRefreshCRC, &QPushButton::clicked, this, &MainWindow::refreshCRC);

    // Toolbar Actions
    connect(ui->actionSetup, &QAction::triggered, this, &MainWindow::runSetup);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionScreenshot, &QAction::triggered, this, &MainWindow::screenshot);
    connect(ui->actionRecordGIF, &QAction::triggered, this, &MainWindow::recordGIF);
    connect(ui->actionTakeGIFScreenshot, &QAction::triggered, this, &MainWindow::screenshotGIF);
    connect(ui->actionRestoreState, &QAction::triggered, this, &MainWindow::restoreEmuState);
    connect(ui->actionSaveState, &QAction::triggered, this, &MainWindow::saveEmuState);
    connect(ui->actionExportCalculatorState, &QAction::triggered, this, &MainWindow::saveToFile);
    connect(ui->actionExportRomImage, &QAction::triggered, this, &MainWindow::exportRom);
    connect(ui->actionImportCalculatorState, &QAction::triggered, this, &MainWindow::restoreFromFile);
    connect(ui->actionReloadROM, &QAction::triggered, this, &MainWindow::reloadROM);
    connect(ui->actionResetCalculator, &QAction::triggered, this, &MainWindow::resetCalculator);
    connect(ui->actionPopoutLCD, &QAction::triggered, this, &MainWindow::createLCD);
    connect(this, &MainWindow::resetTriggered, &emu, &EmuThread::resetTriggered);

    // Capture
    connect(ui->buttonScreenshot, &QPushButton::clicked, this, &MainWindow::screenshot);
    connect(ui->buttonGIF, &QPushButton::clicked, this, &MainWindow::recordGIF);
    connect(ui->buttonGIFScreenshot, &QPushButton::clicked, this, &MainWindow::screenshotGIF);
    connect(ui->frameskipSlider, &QSlider::valueChanged, this, &MainWindow::changeFrameskip);

    // About
    connect(ui->actionCheckForUpdates, &QAction::triggered, this, [=](){ this->checkForUpdates(true); });
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
    
    // Other GUI actions
    connect(ui->buttonRunSetup, &QPushButton::clicked, this, &MainWindow::runSetup);
    connect(ui->scaleSlider, &QSlider::sliderMoved, this, &MainWindow::setReprintScale);
    connect(ui->scaleSlider, &QSlider::valueChanged, this, &MainWindow::setLCDScale);
    connect(ui->checkSkin, &QCheckBox::stateChanged, this, &MainWindow::setSkinToggle);
    connect(ui->refreshSlider, &QSlider::valueChanged, this, &MainWindow::setLCDRefresh);
    connect(ui->checkAlwaysOnTop, &QCheckBox::stateChanged, this, &MainWindow::setAlwaysOnTop);
    connect(ui->emulationSpeed, &QSlider::valueChanged, this, &MainWindow::setEmulatedSpeed);
    connect(ui->checkThrottle, &QCheckBox::stateChanged, this, &MainWindow::setThrottleMode);
    connect(ui->lcdWidget, &QWidget::customContextMenuRequested, this, &MainWindow::screenContextMenu);
    connect(ui->checkSaveRestore, &QCheckBox::stateChanged, this, &MainWindow::setAutoSaveState);
    connect(ui->checkPortable, &QCheckBox::stateChanged, this, &MainWindow::setPortableConfig);
    connect(ui->checkSaveLoadDebug, &QCheckBox::stateChanged, this, &MainWindow::setSaveDebug);
    connect(ui->buttonChangeSavedImagePath, &QPushButton::clicked, this, &MainWindow::setImagePath);
    connect(ui->buttonChangeSavedDebugPath, &QPushButton::clicked, this, &MainWindow::setDebugPath);
    connect(this, &MainWindow::setEmuSpeed, &emu, &EmuThread::setEmuSpeed);
    connect(this, &MainWindow::changedThrottleMode, &emu, &EmuThread::changeThrottleMode);
    connect(&emu, &EmuThread::actualSpeedChanged, this, &MainWindow::showActualSpeed, Qt::QueuedConnection);
    connect(ui->flashBytes, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui->flashEdit, &QHexEdit::setBytesPerLine);
    connect(ui->ramBytes, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui->ramEdit, &QHexEdit::setBytesPerLine);
    connect(ui->memBytes, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui->memEdit, &QHexEdit::setBytesPerLine);
    connect(ui->emuVarView, &QTableWidget::itemDoubleClicked, this, &MainWindow::variableClicked);
    ui->emuVarView->setContextMenuPolicy(Qt::CustomContextMenu); // To handle right-clicks
    connect(ui->emuVarView, &QWidget::customContextMenuRequested, this, &MainWindow::variablesContextMenu);

    // Hex Editor
    connect(ui->buttonFlashGoto, &QPushButton::clicked, this, &MainWindow::flashGotoPressed);
    connect(ui->buttonFlashSearch, &QPushButton::clicked, this, &MainWindow::flashSearchPressed);
    connect(ui->buttonFlashSync, &QPushButton::clicked, this, &MainWindow::flashSyncPressed);
    connect(ui->buttonRamGoto, &QPushButton::clicked, this, &MainWindow::ramGotoPressed);
    connect(ui->buttonRamSearch, &QPushButton::clicked, this, &MainWindow::ramSearchPressed);
    connect(ui->buttonRamSync, &QPushButton::clicked, this, &MainWindow::ramSyncPressed);
    connect(ui->buttonMemGoto, &QPushButton::clicked, this, &MainWindow::memGotoPressed);
    connect(ui->buttonMemSearch, &QPushButton::clicked, this, &MainWindow::memSearchPressed);
    connect(ui->buttonMemSync, &QPushButton::clicked, this, &MainWindow::memSyncPressed);

    // Keybindings
    connect(ui->radioCEmuKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radioTilEmKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radioWabbitemuKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radiojsTIfiedKeys, &QRadioButton::clicked, this, &MainWindow::keymapChanged);

    // Keypad Coloring
    connect(ui->buttonTrueBlue,  &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonDenim,  &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonPink,  &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonPlum,  &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonRed,  &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonLightning,  &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonGolden,  &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonWhite,  &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonBlack,  &QPushButton::clicked, this, &MainWindow::selectKeypadColor);
    connect(ui->buttonSilver,  &QPushButton::clicked, this, &MainWindow::selectKeypadColor);

    // Auto Updates
    connect(ui->checkUpdates, &QCheckBox::stateChanged, this, &MainWindow::setAutoCheckForUpdates);

    // Shortcut Connections
    stepInShortcut = new QShortcut(QKeySequence(Qt::Key_F6), this);
    stepOverShortcut = new QShortcut(QKeySequence(Qt::Key_F7), this);
    stepNextShortcut = new QShortcut(QKeySequence(Qt::Key_F8), this);
    stepOutShortcut = new QShortcut(QKeySequence(Qt::Key_F9), this);
    debuggerShortcut = new QShortcut(QKeySequence(Qt::Key_F10), this);
    asmShortcut = new QShortcut(QKeySequence(Qt::Key_F11), this);

    debuggerShortcut->setAutoRepeat(false);
    stepInShortcut->setAutoRepeat(false);
    stepOverShortcut->setAutoRepeat(false);
    stepNextShortcut->setAutoRepeat(false);
    stepOutShortcut->setAutoRepeat(false);
    asmShortcut->setAutoRepeat(false);

    connect(debuggerShortcut, &QShortcut::activated, this, &MainWindow::debuggerChangeState);
    connect(stepInShortcut, &QShortcut::activated, this, &MainWindow::stepInPressed);
    connect(stepOverShortcut, &QShortcut::activated, this, &MainWindow::stepOverPressed);
    connect(stepNextShortcut, &QShortcut::activated, this, &MainWindow::stepNextPressed);
    connect(stepOutShortcut, &QShortcut::activated, this, &MainWindow::stepOutPressed);
    connect(asmShortcut, &QShortcut::activated, this, &MainWindow::sendASMKey);

    // Meta Types
    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<std::string>("std::string");

    ui->portView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->breakpointView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->profilerView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->watchpointView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    setUIStyle(true);

    autotester::stepCallback = []() { QApplication::processEvents(); };

    if (fileExists(QDir::toNativeSeparators(qApp->applicationDirPath() + "/cemu_config.ini").toStdString())) {
        pathSettings = qApp->applicationDirPath() + "/cemu_config.ini";
        portable = true;
    } else if (opts.settingsFile.isEmpty()) {
        pathSettings = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/CEmu/cemu_config.ini");
    } else {
        pathSettings = opts.settingsFile;
    }

    settings = new QSettings(pathSettings, QSettings::IniFormat);

    if (portable) {
        ui->checkPortable->setChecked(true);
        ui->buttonChangeSavedDebugPath->setEnabled(false);
        ui->buttonChangeSavedImagePath->setEnabled(false);
        ui->settingsPath->setText(QFile(pathSettings).fileName());
    } else {
        ui->settingsPath->setText(pathSettings);
    }

#ifdef _WIN32
    installToggleConsole();
#endif

    if (opts.romFile.isEmpty()) {
        emu.rom = settings->value(QStringLiteral("romImage")).toString();
    } else {
        emu.rom = opts.romFile;
    }
    changeFrameskip(settings->value(QStringLiteral("frameskip"), 3).toUInt());
    setLCDScale(settings->value(QStringLiteral("scale"), 100).toUInt());
    setSkinToggle(settings->value(QStringLiteral("skin"), 1).toBool());
    setLCDRefresh(settings->value(QStringLiteral("refreshRate"), 60).toUInt());
    setEmulatedSpeed(settings->value(QStringLiteral("emuRate"), 10).toUInt());
    setFont(settings->value(QStringLiteral("textSize"), 9).toUInt());
    setAutoCheckForUpdates(settings->value(QStringLiteral("autoUpdate"), false).toBool());
    setAutoSaveState(settings->value(QStringLiteral("restoreOnOpen"), true).toBool());
    setSaveDebug(settings->value(QStringLiteral("loadDebugOnOpen"), false).toBool());
    setSpaceDisasm(settings->value(QStringLiteral("addDisasmSpace"), false).toBool());
    setUIEditMode(settings->value(QStringLiteral("uiMode"), true).toBool());
    ui->flashBytes->setValue(settings->value(QStringLiteral("flashBytesPerLine"), 8).toInt());
    ui->ramBytes->setValue(settings->value(QStringLiteral("ramBytesPerLine"), 8).toInt());
    ui->memBytes->setValue(settings->value(QStringLiteral("memBytesPerLine"), 8).toInt());

    currentDir.setPath((settings->value(QStringLiteral("currDir"), QDir::homePath()).toString()));
    if (settings->value(QStringLiteral("savedImagePath")).toString().isEmpty() || portable) {
        QString path = QDir::cleanPath(QFileInfo(settings->fileName()).absoluteDir().absolutePath() + QStringLiteral("/cemu_image.ce"));
        settings->setValue(QStringLiteral("savedImagePath"), path);
    }
    ui->savedImagePath->setText(settings->value(QStringLiteral("savedImagePath")).toString());
    emu.image = ui->savedImagePath->text();

    if (settings->value(QStringLiteral("savedDebugPath")).toString().isEmpty() || portable) {
        QString path = QDir::cleanPath(QFileInfo(settings->fileName()).absoluteDir().absolutePath() + QStringLiteral("/cemu_debug.ini"));
        settings->setValue(QStringLiteral("savedDebugPath"), path);
    }
    ui->savedDebugPath->setText(settings->value(QStringLiteral("savedDebugPath")).toString());

    QString currKeyMap = settings->value(QStringLiteral("keyMap"), "cemu").toString();
    if (QStringLiteral("cemu").compare(currKeyMap, Qt::CaseInsensitive) == 0) {
        ui->radioCEmuKeys->setChecked(true);
    }
    else if (QStringLiteral("tilem").compare(currKeyMap, Qt::CaseInsensitive) == 0) {
        ui->radioTilEmKeys->setChecked(true);
    }
    else if (QStringLiteral("wabbitemu").compare(currKeyMap, Qt::CaseInsensitive) == 0) {
        ui->radioWabbitemuKeys->setChecked(true);
    }
    else if (QStringLiteral("jsTIfied").compare(currKeyMap, Qt::CaseInsensitive) == 0) {
        ui->radiojsTIfiedKeys->setChecked(true);
    }
    setKeymap(currKeyMap);

    ui->rompathView->setText(emu.rom);
    ui->emuVarView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->vatView->cursorState(true);
    ui->opView->cursorState(true);
    ui->opView->updateAllHighlights();
    ui->vatView->updateAllHighlights();

    debugger_init();
    profiler_init();

    if (!opts.imageFile.isEmpty()) {
        if (fileExists(opts.imageFile.toStdString())) {
            emu.image = opts.imageFile;
        }
    }

    if (!fileExists(QDir::toNativeSeparators(emu.rom).toStdString())) {
        if (!runSetup()) {
            exit(0);
        }
    } else {
        if (settings->value(QStringLiteral("restoreOnOpen")).toBool()
                && fileExists(emu.image.toStdString())
                && opts.restoreOnOpen) {
            restoreEmuState();
        } else {
            emu.start();
        }
    }

    speedUpdateTimer.start();
    speedUpdateTimer.setInterval(1000 / 2);

    colorback.setColor(QPalette::Base, QColor(Qt::yellow).lighter(160));
    setAlwaysOnTop(settings->value(QStringLiteral("onTop"), 0).toUInt());
    restoreGeometry(settings->value(QStringLiteral("windowGeometry")).toByteArray());
    restoreState(settings->value(QStringLiteral("windowState")).toByteArray(), WindowStateVersion);
    consoleFormat = ui->console->currentCharFormat();

    QPixmap pix;

    pix.load(":/icons/resources/icons/stop.png");
    stopIcon.addPixmap(pix);
    pix.load(":/icons/resources/icons/run.png");
    runIcon.addPixmap(pix);

    if (settings->value(QStringLiteral("loadDebugOnOpen"), false).toBool()) {
        if (!opts.debugFile.isEmpty()) {
            debuggerImportFile(opts.debugFile);
        } else {
            debuggerImportFile(settings->value(QStringLiteral("savedDebugPath")).toString());
        }
    }

    if (!opts.autotesterFile.isEmpty()){
        if (!openJSONConfig(opts.autotesterFile)) {
           resetCalculator();
           setEmuSpeed(100);

           // Race condition requires this
           guiDelay(1000);
           launchTest();
        }
    }

    if (!opts.sendFiles.isEmpty() || !opts.sendArchFiles.isEmpty() || !opts.sendRAMFiles.isEmpty()) {
        resetCalculator();
        setEmuSpeed(100);

        // Race condition requires this
        guiDelay(1000);
        if (!opts.sendFiles.isEmpty()) {
            sendingHandler.sendFiles(opts.sendFiles, LINK_FILE);
        }
        if (!opts.sendArchFiles.isEmpty()) {
            sendingHandler.sendFiles(opts.sendArchFiles, LINK_ARCH);
        }
        if (!opts.sendRAMFiles.isEmpty()) {
            sendingHandler.sendFiles(opts.sendRAMFiles, LINK_RAM);
        }
    }

    setThrottleMode(opts.useUnthrottled ? Qt::Unchecked : Qt::Checked);
    ui->lcdWidget->setFocus();
}

MainWindow::~MainWindow() {
    debugger_free();
    profiler_free();

    delete toggleAction;
    delete debuggerShortcut;
    delete stepInShortcut;
    delete stepOverShortcut;
    delete stepNextShortcut;
    delete stepOutShortcut;
    delete asmShortcut;
    delete settings;
    delete ui->flashEdit;
    delete ui->ramEdit;
    delete ui->memEdit;
    delete ui;
}

void MainWindow::sendASMKey() {
    autotester::sendKey(0x9CFC); // "Asm("
}

bool MainWindow::restoreEmuState() {
    QString default_saved_image = settings->value(QStringLiteral("savedImagePath")).toString();
    if (!default_saved_image.isEmpty()) {
        return restoreFromPath(default_saved_image);
    } else {
        QMessageBox::warning(this, tr("Can't restore state"), tr("No saved image path in settings"));
        return false;
    }
}

void MainWindow::saveToPath(QString path) {
    emu_thread->save(path);
}

bool MainWindow::restoreFromPath(QString path) {
    if (isReceiving || isSending) {
        refreshVariableList();
    }
    if (!emu_thread->restore(path)) {
        QMessageBox::warning(this, tr("Could not restore"), tr("Try restarting"));
        return false;
    }

    return true;
}

void MainWindow::saveEmuState() {
    QString default_savedImage = settings->value(QStringLiteral("savedImagePath")).toString();
    if (!default_savedImage.isEmpty()) {
        saveToPath(default_savedImage);
    } else {
        QMessageBox::warning(this, tr("Can't save image"), tr("No saved image path in settings given"));
    }
}

void MainWindow::restoreFromFile() {
    QString savedImage = QFileDialog::getOpenFileName(this, tr("Select saved image to restore from"),
                                                      currentDir.absolutePath(),
                                                      tr("CEmu images (*.ce);;All files (*.*)"));
    if (!savedImage.isEmpty()) {
        currentDir = QFileInfo(savedImage).absoluteDir();
        if (restoreFromPath(savedImage)) {
            usingLoadedImage = true;
        }
    }
}

void MainWindow::saveToFile() {
    QString savedImage = QFileDialog::getSaveFileName(this, tr("Set image to save to"),
                                                      currentDir.absolutePath(),
                                                      tr("CEmu images (*.ce);;All files (*.*)"));
    if (!savedImage.isEmpty()) {
        currentDir = QFileInfo(savedImage).absoluteDir();
        saveToPath(savedImage);
    }
}
void MainWindow::exportRom() {
    QString saveRom = QFileDialog::getSaveFileName(this, tr("Set Rom image to save to"),
                                                      currentDir.absolutePath(),
                                                      tr("ROM images (*.rom);;All files (*.*)"));
    if (!saveRom.isEmpty()) {
        currentDir = QFileInfo(saveRom).absoluteDir();
        emu_thread->saveRomImage(saveRom);
    }
}

void MainWindow::started(bool success) {
    if (success) {
        setKeypadColor(settings->value(QStringLiteral("keypadColor"), true).toUInt());
    }
}

void MainWindow::restored(bool success) {
    started(success);
    if (success) {
        showStatusMsg(tr("Emulation restored from image."));
    } else {
        QMessageBox::warning(this, tr("Could not restore"), tr("Resuming failed.\nPlease Reload your ROM."));
    }
}

void MainWindow::saved(bool success) {
    if (success) {
        showStatusMsg(tr("Image saved."));
    } else {
        QMessageBox::warning(this, tr("Could not save"), tr("Saving failed.\nSaving failed, go tell someone."));
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
    sendingHandler.dropOccured(e, LINK_FILE);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e) {
    sendingHandler.dragOccured(e);
}

void MainWindow::closeEvent(QCloseEvent *e) {
    if (inDebugger) {
        debuggerChangeState();
    }
    if (isReceiving || isSending) {
        refreshVariableList();
    }

    settings->setValue(QStringLiteral("windowState"),       saveState(WindowStateVersion));
    settings->setValue(QStringLiteral("windowGeometry"),    saveGeometry());
    settings->setValue(QStringLiteral("currDir"),           currentDir.absolutePath());
    settings->setValue(QStringLiteral("flashBytesPerLine"), ui->flashBytes->value());
    settings->setValue(QStringLiteral("ramBytesPerLine"),   ui->ramBytes->value());
    settings->setValue(QStringLiteral("memBytesPerLine"),   ui->memBytes->value());
    settings->setValue(QStringLiteral("keypadColor"),       ui->keypadWidget->getCurrColor());

    if (settings->value(QStringLiteral("saveDebugOnClose"), false).toBool()) {
        debuggerExportFile(settings->value(QStringLiteral("savedDebugPath")).toString());
    }

    if (!closeAfterSave && settings->value(QStringLiteral("saveOnClose")).toBool()) {
            closeAfterSave = true;
            saveEmuState();
            e->ignore();
            return;
    }

    if (!emu.stop()) {
        qDebug("Thread Termination Failed.");
    }

    speedUpdateTimer.stop();

    QMainWindow::closeEvent(e);
}

void MainWindow::consoleAppend(QString str, QColor color) {
    QTextCursor cur(ui->console->document());
    cur.movePosition(QTextCursor::End);
    consoleFormat.setForeground(color);
    cur.insertText(str, consoleFormat);
    if (ui->checkAutoScroll->isChecked()) {
        ui->console->setTextCursor(cur);
    }
}

void MainWindow::consoleStr(QString str) {
    if (nativeConsole) {
        fputs(str.toStdString().c_str(), stdout);
    } else {
        consoleAppend(str);
    }
}

void MainWindow::consoleErrStr(QString str) {
    if (nativeConsole) {
        fputs(str.toStdString().c_str(), stderr);
    } else {
        consoleAppend(str, Qt::red);
    }
}

void MainWindow::showActualSpeed(int speed) {
    showStatusMsg(QStringLiteral(" ") + tr("Actual Speed: ") + QString::number(speed, 10) + QStringLiteral("%"));
}

void MainWindow::showStatusMsg(QString str) {
    statusLabel.setText(str);
}

bool MainWindow::runSetup() {
    RomSelection romWizard;
    romWizard.show();
    romWizard.exec();

    if (romWizard.romPath().isEmpty()) {
        return false;
    } else {
        if (isReceiving || isSending) {
            refreshVariableList();
        }
        if (inDebugger) {
            debuggerChangeState();
        }
        guiDelay(300);
        emu.rom = romWizard.romPath();
        if (portable) {
            QDir dir(qApp->applicationDirPath());
            emu.rom = dir.relativeFilePath(emu.rom);
        }
        if (emu.stop()) {
            speedUpdateTimer.stop();
            ui->rompathView->setText(emu.rom);
            emu.start();
            speedUpdateTimer.start();
            speedUpdateTimer.setInterval(1000 / 2);
        }
        settings->setValue(QStringLiteral("romImage"), emu.rom);
    }

    return true;
}

void MainWindow::screenshotSave(QString nameFilter, QString defaultSuffix, QString temppath) {
    QFileDialog dialog(this);

    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setDirectory(currentDir);
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
    currentDir = dialog.directory();
}

void MainWindow::screenshot() {
    QImage image = renderFramebuffer(&lcd);

    QString path = QDir::tempPath() + QDir::separator() + QStringLiteral("cemu_tmp.img");
    if (!image.save(path, "PNG", 0)) {
        QMessageBox::critical(this, tr("Screenshot failed"), tr("Failed to save screenshot!"));
    }

    screenshotSave(tr("PNG images (*.png)"), QStringLiteral("png"), path);
}

void MainWindow::screenshotGIF() {
    if (ui->actionRecordGIF->isChecked()) {
        QMessageBox::warning(this, tr("Recording GIF"), tr("Currently recording GIF."));
        return;
    }

    QString path = QDir::tempPath() + QDir::separator() + QStringLiteral("cemu_tmp.img");
    lcd_event_gui_callback = gif_new_frame;
    if (!gif_single_frame(path.toStdString().c_str())) {
        QMessageBox::critical(this, tr("Screenshot failed"), tr("Failed to save screenshot!"));
    }
    lcd_event_gui_callback = NULL;

    screenshotSave(tr("GIF images (*.gif)"), QStringLiteral("gif"), path);
}

void MainWindow::recordGIF() {
  static QString path;
  static QString opt_path;

  if (path.isEmpty()) {
        path = QDir::tempPath() + QDir::separator() + QStringLiteral("cemu_tmp.gif");
        opt_path = QDir::tempPath() + QDir::separator() + QStringLiteral("cemu_opt_tmp.gif");
        lcd_event_gui_callback = gif_new_frame;
        gif_start_recording(path.toStdString().c_str(), ui->frameskipSlider->value());
    } else {
        if (gif_stop_recording()) {
            if (!(gif_optimize(path.toStdString().c_str(), opt_path.toStdString().c_str()))) {
                QFile(path).remove();
                QMessageBox::warning(this, tr("GIF Optimization Failed"), tr("A failure occured during recording"));
            } else {
                QFile(path).remove();
                screenshotSave(tr("GIF images (*.gif)"), QStringLiteral("gif"), opt_path);
            }
        } else {
            QMessageBox::warning(this, tr("Failed recording GIF"), tr("A failure occured during recording"));
        }
        lcd_event_gui_callback = NULL;
        path = QString();
        opt_path = QString();
    }

    ui->frameskipSlider->setEnabled(path.isEmpty());
    ui->actionRecordGIF->setChecked(!path.isEmpty());
    ui->buttonGIF->setText((!path.isEmpty()) ? tr("Stop Recording") : tr("Record GIF"));
    ui->actionRecordGIF->setText((!path.isEmpty()) ? tr("Stop GIF Recording...") : tr("Record animated GIF..."));
}

void MainWindow::changeFrameskip(int value) {
    settings->setValue(QStringLiteral("frameskip"), value);
    ui->frameskipLabel->setText(QString::number(value));
    ui->frameskipSlider->setValue(value);
    changeFramerate();
}

void MainWindow::changeFramerate() {
    float framerate = ((float) ui->refreshSlider->value()) / (ui->frameskipSlider->value() + 1);
    ui->framerateLabel->setText(QString::number(framerate).left(4));
}

void MainWindow::showAbout() {
    QMessageBox aboutBox(this);
    aboutBox.setIconPixmap(QPixmap(":/icons/resources/icons/icon.png"));
    aboutBox.setWindowTitle(tr("About CEmu"));

    QAbstractButton* buttonUpdateCheck = aboutBox.addButton(tr("Check for updates"), QMessageBox::ActionRole);
    connect(buttonUpdateCheck, &QAbstractButton::clicked, this, [=](){ this->checkForUpdates(true); });

    QAbstractButton* okButton = aboutBox.addButton(QMessageBox::Ok);
    okButton->setFocus();

    aboutBox.setText(tr("<h3>CEmu %1</h3>"
                         "<a href='https://github.com/CE-Programming/CEmu'>On GitHub</a><br>"
                         "<br>"
                         "Main authors:<br>"
                         "Matt Waltz (<a href='https://github.com/MateoConLechuga'>MateoConLechuga</a>)<br>"
                         "Jacob Young (<a href='https://github.com/jacobly0'>jacobly0</a>)<br>"
                         "<br>"
                         "Other contributors:<br>"
                         "Adrien Bertrand (<a href='https://github.com/adriweb'>adriweb</a>)<br>"
                         "Lionel Debroux (<a href='https://github.com/debrouxl'>debrouxl</a>)<br>"
                         "Fabian Vogt (<a href='https://github.com/Vogtinator'>Vogtinator</a>)<br>"
                         "<br>"
                         "Many thanks to the <a href='https://github.com/KnightOS/z80e'>z80e</a> (MIT license <a href='https://github.com/KnightOS/z80e/blob/master/LICENSE'>here</a>) and <a href='https://github.com/nspire-emus/firebird'>Firebird</a> (GPLv3 license <a href='https://github.com/nspire-emus/firebird/blob/master/LICENSE'>here</a>) projects.<br>In-program icons are courtesy of the <a href='http://www.famfamfam.com/lab/icons/silk/'>Silk iconset</a>.<br>"
                         "<br>"
                         "This work is licensed under the GPLv3.<br>"
                         "To view a copy of this license, visit <a href='https://www.gnu.org/licenses/gpl-3.0.html'>https://www.gnu.org/licenses/gpl-3.0.html</a>")
                         .arg(QStringLiteral(CEMU_VERSION)));
    aboutBox.setTextFormat(Qt::RichText);
    aboutBox.show();
    aboutBox.exec();
}

void MainWindow::screenContextMenu(const QPoint &posa) {
    QMenu contextMenu;
    QPoint globalPos = ui->lcdWidget->mapToGlobal(posa);
    QList<QMenu*> list = ui->menubar->findChildren<QMenu*>();
    for (int i=0; i<list.size(); i++) {
        contextMenu.addMenu(list.at(i));
    }
    contextMenu.exec(globalPos);
}

void MainWindow::consoleOutputChanged() {
    nativeConsole = ui->radioStderr->isChecked();
}

void MainWindow::isBusy(bool busy) {
    if (busy) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    } else {
        QApplication::restoreOverrideCursor();
    }
}

// ------------------------------------------------
//  Linking things
// ------------------------------------------------

QStringList MainWindow::showVariableFileDialog(QFileDialog::AcceptMode mode, QString name_filter) {
    QFileDialog dialog(this);
    int good;

    dialog.setAcceptMode(mode);
    dialog.setFileMode(mode == QFileDialog::AcceptOpen ? QFileDialog::ExistingFiles : QFileDialog::AnyFile);
    dialog.setDirectory(currentDir);
    dialog.setNameFilter(name_filter);
    good = dialog.exec();

    currentDir = dialog.directory();

    if (good) {
        return dialog.selectedFiles();
    }

    return QStringList();
}

void MainWindow::selectFiles() {
    if (inDebugger) {
       return;
    }

    QStringList fileNames = showVariableFileDialog(QFileDialog::AcceptOpen, tr("TI Variable (*.8xp *.8xv *.8xl *.8xn *.8xm *.8xy *.8xg *.8xs *.8xd *.8xw *.8xc *.8xl *.8xz *.8xt *.8ca *.8cg *.8ci *.8ek);;All Files (*.*)"));

    sendingHandler.sendFiles(fileNames, LINK_FILE);
}

void MainWindow::variableClicked(QTableWidgetItem *item) {
    const calc_var_t& var_tmp = vars[ui->emuVarView->item(item->row(), 0)->data(Qt::UserRole).toInt()];
    if (!calc_var_is_asmprog(&var_tmp) && (!calc_var_is_internal(&var_tmp) || var_tmp.name[0] == '#')) {
        BasicCodeViewerWindow codePopup;
        codePopup.setOriginalCode((var_tmp.size <= 500) ? ui->emuVarView->item(item->row(), 3)->text() : QString::fromStdString(calc_var_content_string(var_tmp)));
        codePopup.setVariableName(ui->emuVarView->item(item->row(), 0)->text());
        codePopup.show();
        codePopup.exec();
    }
}

void MainWindow::refreshVariableList() {
    calc_var_t var;

    if (inDebugger) {
        return;
    }

    ui->emuVarView->setRowCount(0);

    if (isReceiving || isSending) {
        ui->buttonRefreshList->setText(tr("Refresh variable list..."));
        ui->buttonReceiveFiles->setEnabled(false);
        ui->buttonRun->setEnabled(true);
        ui->buttonSend->setEnabled(true);
        setReceiveState(false);
    } else {
        ui->buttonRefreshList->setText(tr("Resume emulation"));
        ui->buttonSend->setEnabled(false);
        ui->buttonReceiveFiles->setEnabled(true);
        ui->buttonRun->setEnabled(false);
        setReceiveState(true);
        ui->emuVarView->blockSignals(true);
        guiDelay(200);

        vat_search_init(&var);
        vars.clear();
        while (vat_search_next(&var)) {
            if (var.size > 2) {
                int currRow;

                vars.append(var);
                currRow = ui->emuVarView->rowCount();
                ui->emuVarView->setRowCount(currRow + 1);

                bool var_preview_needs_gray = false;
                QString var_value;
                if (calc_var_is_asmprog(&var)) {
                    var_value = tr("Can't preview ASM");
                    var_preview_needs_gray = true;
                } else if (calc_var_is_internal(&var) && var.name[0] != '#') { // # is previewable
                    var_value = tr("Can't preview this OS variable");
                    var_preview_needs_gray = true;
                } else if (var.type == CALC_VAR_TYPE_APP_VAR || var.size > 500) {
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
                QTableWidgetItem *var_type = new QTableWidgetItem(var_type_str);
                QTableWidgetItem *var_size = new QTableWidgetItem(QString::number(var.size));
                QTableWidgetItem *var_preview = new QTableWidgetItem(var_value);

                // Attach var index (hidden) to the name. Needed elsewhere
                var_name->setData(Qt::UserRole, currRow);

                var_name->setCheckState(Qt::Unchecked);

                if (var_preview_needs_gray) {
                    var_preview->setForeground(Qt::gray);
                }

                ui->emuVarView->setItem(currRow, 0, var_name);
                ui->emuVarView->setItem(currRow, 1, var_type);
                ui->emuVarView->setItem(currRow, 2, var_size);
                ui->emuVarView->setItem(currRow, 3, var_preview);
            }
        }
        ui->emuVarView->resizeColumnsToContents();
        ui->emuVarView->horizontalHeader()->setStretchLastSection(true);
        ui->emuVarView->setVisible(false);  // This is needed
        ui->emuVarView->setVisible(true);   // to refresh
    }

    ui->emuVarView->blockSignals(false);
}

void MainWindow::saveSelected() {
    constexpr const char* var_extension[] = {
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

    setReceiveState(true);

    QVector<calc_var_t> selectedVars;
    QStringList fileNames;
    for (int currRow = 0; currRow < ui->emuVarView->rowCount(); currRow++) {
        if (ui->emuVarView->item(currRow, 0)->checkState()) {
            selectedVars.append(vars[currRow]);
        }
    }
    if (selectedVars.size() < 1) {
        QMessageBox::warning(this, tr("No transfer to do"), tr("Select at least one file to transfer"));
    } else {
        if (selectedVars.size() == 1) {
            uint8_t i = selectedVars.at(0).type1;
            fileNames = showVariableFileDialog(QFileDialog::AcceptSave, QStringLiteral("TI ") +
                                               QString(calc_var_type_names[i]) +
                                               " (*." + var_extension[i] + tr(");;All Files (*.*)"));
        } else {
            fileNames = showVariableFileDialog(QFileDialog::AcceptSave, tr("TI Group (*.8cg);;All Files (*.*)"));
        }
        if (fileNames.size() == 1) {
            if (!receiveVariableLink(selectedVars.size(), selectedVars.constData(), fileNames.at(0).toUtf8())) {
                QMessageBox::warning(this, tr("Failed Transfer"), tr("A failure occured during transfer of: ")+fileNames.at(0));
            }
        }
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
    QMessageBox::warning(this, tr("Autotester error"), errMsg);
}


int MainWindow::openJSONConfig(const QString& jsonPath) {
    std::string jsonContents;
    std::ifstream ifs(jsonPath.toStdString());

    if (ifs.good())
    {
        int ok = chdir(QDir::toNativeSeparators(QFileInfo(jsonPath).absoluteDir().path()).toStdString().c_str());
        ui->buttonReloadJSONconfig->setEnabled(true);
        if (ok != 0) {
            QMessageBox::warning(this, tr("Internal Autotester error"), tr("Couldn't go to where the JSON file is."));
            return 0;
        }
        std::getline(ifs, jsonContents, '\0');
        if (!ifs.eof()) {
            QMessageBox::warning(this, tr("File error"), tr("Couldn't read JSON file."));
            return 0;
        }
    } else {
        ui->buttonReloadJSONconfig->setEnabled(false);
        QMessageBox::warning(this, tr("Opening error"), tr("Unable to open the file."));
        return 1;
    }

    autotester::ignoreROMfield = true;
    autotester::debugLogs = false;
    if (autotester::loadJSONConfig(jsonContents))
    {
        ui->JSONconfigPath->setText(jsonPath);
        ui->buttonLaunchTest->setEnabled(true);
        std::cout << "[OK] Test config loaded and verified. " << autotester::config.hashes.size() << " unique tests found." << std::endl;
    } else {
        QMessageBox::warning(this, tr("JSON format error"), tr("Error. See the test config file format and make sure values are correct and referenced files are there."));
        return 1;
    }
    return 0;
}

void MainWindow::prepareAndOpenJSONConfig() {
    QFileDialog dialog(this);

    ui->buttonLaunchTest->setEnabled(false);

    dialog.setDirectory(QDir::homePath());
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(QStringLiteral("JSON config (*.json)"));
    if (!dialog.exec()) {
        return;
    }

    openJSONConfig(dialog.selectedFiles().at(0));
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
        guiDelay(1000);
    }

    if (ui->checkBoxTestClear->isChecked()) {
        // Clear home screen
        autotester::sendKey(0x09);
    }

    QStringList filesList;
    for (const auto& file : autotester::config.transfer_files) {
        filesList << QString::fromStdString(file);
    }

    sendingHandler.sendFiles(filesList, LINK_FILE);
    guiDelay(100);

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
    size_t crc_size = 0;
    uint8_t* start;
    char *endptr1, *endptr2; // catch strtoul issues

    ui->startCRC->setText(ui->startCRC->text().trimmed());
    ui->sizeCRC->setText(ui->sizeCRC->text().trimmed());

    if (ui->startCRC->text().isEmpty() || ui->sizeCRC->text().isEmpty()) {
        goto errCRCret;
    }

    // Get GUI values
    tmp_start = (uint32_t)strtoul(ui->startCRC->text().toStdString().c_str(), &endptr1, 0);
    crc_size = (size_t)strtoul(ui->sizeCRC->text().toStdString().c_str(), &endptr2, 0);
    if (*endptr1 || *endptr2) {
        goto errCRCret;
    }

    // Get real start pointer
    start = phys_mem_ptr(tmp_start, crc_size);

    // Compute and display CRC
    if (start != NULL) {
        char buf[10] = {0};
        sprintf(buf, "%X", crc32(start, crc_size));
        ui->valueCRC->setText(buf);
        return;
    } else {
        goto errCRCret;
    }

errCRCret:
    QMessageBox::warning(this, tr("CRC Error"), tr("Error. Make sure you have entered a valid start/size pair or preset."));
    return;
}

void MainWindow::updateTIOSView() {
    calc_var_t var;
    QString formattedLine;
    QString calcData, varName;
    QString opType;
    uint8_t gotData[11];

    ui->opView->clear();
    ui->vatView->clear();

    for (uint32_t i = 0xD005F8; i<0xD005F8+77; i+=11) {
        uint8_t index = 0;
        calcData.clear();
        opType.clear();
        for (uint32_t j = i; j < i+11; j++) {
            gotData[index] = mem_peek_byte(j);
            calcData += int2hex(gotData[index++], 2);
        }
        if (*gotData < 0x40) {
            opType = QString(calc_var_type_names[*gotData]);
        }

        formattedLine = QString("<pre><b><font color='#444'>%1</font></b><font color='darkblue'>  %2  </font>%3 <font color='green'>%4</font></pre>")
                                       .arg(int2hex(i, 6), "OP"+QString::number(((i-0xD005F8)/11)+1), calcData, opType);

        ui->opView->appendHtml(formattedLine);
    }

    vat_search_init(&var);
    while (vat_search_next(&var)) {
        formattedLine = QString("<pre><b><font color='#444'>%1</font></b>  <font color='darkblue'>%2</font> <font color='green'>%3</font> <font color='green'>%4</font>%5</pre>")
                                        .arg(int2hex(var.address,6), int2hex(var.vat,6), int2hex(var.size,4), QString(calc_var_type_names[var.type]).leftJustified(19, ' '), QString(calc_var_name_to_utf8(var.name)));
        ui->vatView->appendHtml(formattedLine);
    }
    ui->vatView->moveCursor(QTextCursor::Start);
}

void MainWindow::resetCalculator() {
    if (isReceiving || isSending) {
        refreshVariableList();
    }
    if (inDebugger) {
        debuggerChangeState();
    }
    emit resetTriggered();
}

void MainWindow::reloadROM() {
    if (isReceiving || isSending) {
        refreshVariableList();
    }
    if (inDebugger) {
        debuggerChangeState();
    }

    if (!usingLoadedImage) {
        QFile(emu.image).remove();
    }

    usingLoadedImage = false;
    if (emu.stop()) {
        emu.start();
        consoleStr("[CEmu] Reload Successful.\n");
    } else {
        consoleStr("[CEmu] Reload Failed.\n");
    }
}

void MainWindow::updateStackView() {
    QString formattedLine;

    ui->stackView->blockSignals(true);
    ui->stackView->clear();

    if (cpu.ADL) {
        for (int i=0; i<60; i+=3) {
            formattedLine = QString("<pre><b><font color='#444'>%1</font></b> %2</pre>")
                                    .arg(int2hex(cpu.registers.SPL+i, 6),
                                         int2hex(mem_peek_word(cpu.registers.SPL+i, 1), 6));
            ui->stackView->appendHtml(formattedLine);
        }
    } else {
        for (int i=0; i<40; i+=2) {
            formattedLine = QString("<pre><b><font color='#444'>%1</font></b> %2</pre>")
                                    .arg(int2hex(cpu.registers.SPS+i, 4),
                                         int2hex(mem_peek_word(cpu.registers.SPS+i, 0), 4));
            ui->stackView->appendHtml(formattedLine);
        }
    }

    ui->stackView->moveCursor(QTextCursor::Start);
    ui->stackView->blockSignals(false);
}

void MainWindow::drawNextDisassembleLine() {
    std::string *label = 0;

    if (disasm.base_address != disasm.new_address) {
        disasm.base_address = disasm.new_address;
        addressMap_t::iterator item = disasm.addressMap.find(disasm.new_address);
        if (item != disasm.addressMap.end()) {
            disasmHighlight.hit_read_watchpoint = false;
            disasmHighlight.hit_write_watchpoint = false;
            disasmHighlight.hit_exec_breakpoint = false;
            disasmHighlight.hit_pc = false;

            disasm.instruction.data.clear();
            disasm.instruction.opcode.clear();
            disasm.instruction.mode_suffix.clear();
            disasm.instruction.arguments.clear();
            disasm.instruction.size = 0;

            label = &item->second;
        } else {
            disassembleInstruction();
        }
    } else {
        disassembleInstruction();
    }

    // Some round symbol things
    QString breakpointSymbols = QString("<font color='#A3FFA3'>%1</font><font color='#A3A3FF'>%2</font><font color='#FFA3A3'>%3</font>")
                                   .arg(((disasmHighlight.hit_read_watchpoint == true)  ? "&#9679;" : " "),
                                        ((disasmHighlight.hit_write_watchpoint == true) ? "&#9679;" : " "),
                                        ((disasmHighlight.hit_exec_breakpoint == true)  ? "&#9679;" : " "));

    // Simple syntax highlighting
    QString instructionArgsHighlighted = QString::fromStdString(disasm.instruction.arguments)
                                        .replace(QRegularExpression("(\\$[0-9a-fA-F]+)"), "<font color='green'>\\1</font>") // hex numbers
                                        .replace(QRegularExpression("(^\\d)"), "<font color='blue'>\\1</font>")             // dec number
                                        .replace(QRegularExpression("([()])"), "<font color='#600'>\\1</font>");            // parentheses

    QString formattedLine = QString("<pre><b><font color='#444'>%1</font></b> %2 %3  <font color='darkblue'>%4%5</font>%6</pre>")
                               .arg(int2hex(disasm.base_address, 6),
                                    breakpointSymbols,
                                    label ? QString::fromStdString(*label) + ":" : ui->checkDataCol->isChecked() ? QString::fromStdString(disasm.instruction.data).leftJustified(12, ' ') : "",
                                    QString::fromStdString(disasm.instruction.opcode),
                                    QString::fromStdString(disasm.instruction.mode_suffix),
                                    instructionArgsHighlighted);

    ui->disassemblyView->appendHtml(formattedLine);

    if (!disasmOffsetSet && disasm.new_address > addressPane) {
        disasmOffsetSet = true;
        disasmOffset = ui->disassemblyView->textCursor();
        disasmOffset.movePosition(QTextCursor::StartOfLine);
    }

    if (disasmHighlight.hit_pc == true) {
        ui->disassemblyView->addHighlight(QColor(Qt::blue).lighter(160));
    }
}

void MainWindow::disasmContextMenu(const QPoint& posa) {
    QString set_pc = "Set PC";
    QString toggle_break = "Toggle Breakpoint";
    QString toggle_write_watch = "Toggle Write Watchpoint";
    QString toggle_read_watch = "Toggle Read Watchpoint";
    QString toggle_rw_watch = "Toggle Read/Write Watchpoint";
    QString run_until = "Run Until";
    QString goto_mem = "Goto Memory View";
    ui->disassemblyView->setTextCursor(ui->disassemblyView->cursorForPosition(posa));
    QPoint globalPos = ui->disassemblyView->mapToGlobal(posa);

    QMenu contextMenu;
    contextMenu.addAction(run_until);
    contextMenu.addSeparator();
    contextMenu.addAction(toggle_break);
    contextMenu.addAction(toggle_read_watch);
    contextMenu.addAction(toggle_write_watch);
    contextMenu.addAction(toggle_rw_watch);
    contextMenu.addSeparator();
    contextMenu.addAction(goto_mem);
    contextMenu.addAction(set_pc);

    QAction* selectedItem = contextMenu.exec(globalPos);
    if (selectedItem) {
        if (selectedItem->text() == set_pc) {
            ui->pcregView->setText(ui->disassemblyView->getSelectedAddress());
            uint32_t address = static_cast<uint32_t>(hex2int(ui->pcregView->text()));
            debug_set_pc_address(address);
            updateDisasmView(cpu.registers.PC, true);
        } else if (selectedItem->text() == toggle_break) {
            breakpointGUIAdd();
        } else if (selectedItem->text() == toggle_read_watch) {
            watchpointReadGUIAdd();
        } else if (selectedItem->text() == toggle_write_watch) {
            watchpointWriteGUIAdd();
        } else if (selectedItem->text() == toggle_rw_watch) {
            watchpointReadWriteGUIAdd();
        } else if (selectedItem->text() == run_until) {
            uint32_t address = static_cast<uint32_t>(hex2int(ui->disassemblyView->getSelectedAddress()));
            debug_init_run_until(address);
            debuggerChangeState();
            emit setRunUntilMode();
        } else if (selectedItem->text() == goto_mem) {
            memGoto(ui->disassemblyView->getSelectedAddress());
        }
    }
}

void MainWindow::variablesContextMenu(const QPoint& posa) {
    int itemRow = ui->emuVarView->rowAt(posa.y());
    if (itemRow == -1) {
        return;
    }

    const calc_var_t& selected_var = vars[ui->emuVarView->item(itemRow, 0)->data(Qt::UserRole).toInt()];

    QString launch_prgm = tr("Launch program"),
            goto_mem    = tr("Goto Memory View");

    QMenu contextMenu;
    if (calc_var_is_prog(&selected_var) && !calc_var_is_internal(&selected_var)) {
        contextMenu.addAction(launch_prgm);
    }
    contextMenu.addAction(goto_mem);

    QAction* selectedItem = contextMenu.exec(ui->emuVarView->mapToGlobal(posa));
    if (selectedItem) {
        if (selectedItem->text() == launch_prgm) {
            // Reset keypad state and resume emulation
            keypad_reset();
            refreshVariableList();

            // Launch the program, assuming we're at the home screen...
            autotester::sendKey(0x09); // Clear
            if (calc_var_is_asmprog(&selected_var)) {
                autotester::sendKey(0x9CFC); // Asm(
            }
            autotester::sendKey(0xDA); // prgm
            for (const char& c : selected_var.name) {
                if (!c) { break; }
                autotester::sendLetterKeyPress(c); // type program name
            }
            autotester::sendKey(0x05); // Enter

            // Restore focus to catch keypresses.
            ui->lcdWidget->setFocus();
        } else if (selectedItem->text() == goto_mem) {
            refreshVariableList();
            ui->checkLockPosition->setChecked(true); // TODO: find better than that to prevent the debugger's PC to take over
            if (!inDebugger) {
                debuggerChangeState();
            }
            memGoto(int2hex(selected_var.address, 6));
        }
    }
}

void MainWindow::vatContextMenu(const QPoint& posa) {
    QString goto_mem = tr("Goto Memory View");
    ui->vatView->setTextCursor(ui->vatView->cursorForPosition(posa));
    QPoint globalPos = ui->vatView->mapToGlobal(posa);

    QMenu contextMenu;
    contextMenu.addAction(goto_mem);

    QAction* selectedItem = contextMenu.exec(globalPos);
    if (selectedItem) {
        if (selectedItem->text() == goto_mem) {
            memGoto(ui->vatView->getSelectedAddress());
        }
    }
}

void MainWindow::opContextMenu(const QPoint& posa) {
    QString goto_mem = tr("Goto Memory View");
    ui->opView->setTextCursor(ui->opView->cursorForPosition(posa));
    QPoint globalPos = ui->opView->mapToGlobal(posa);

    QMenu contextMenu;
    contextMenu.addAction(goto_mem);

    QAction* selectedItem = contextMenu.exec(globalPos);
    if (selectedItem) {
        if (selectedItem->text() == goto_mem) {
            memGoto(ui->opView->getSelectedAddress());
        }
    }
}

void MainWindow::createLCD() {
    LCDPopout *p = new LCDPopout(&keypadBridge, this);
    p->show();
}

void MainWindow::stepInPressed() {
    if (!inDebugger) {
        return;
    }

    debuggerUpdateChanges();
    disconnect(stepInShortcut, &QShortcut::activated, this, &MainWindow::stepInPressed);
    emit setDebugStepInMode();
}

void MainWindow::stepOverPressed() {
    if (!inDebugger) {
        return;
    }

    debuggerUpdateChanges();
    disconnect(stepOverShortcut, &QShortcut::activated, this, &MainWindow::stepOverPressed);
    emit setDebugStepOverMode();
}

void MainWindow::stepNextPressed() {
    if (!inDebugger) {
        return;
    }

    debuggerUpdateChanges();
    disconnect(stepNextShortcut, &QShortcut::activated, this, &MainWindow::stepNextPressed);
    emit setDebugStepNextMode();
}

void MainWindow::stepOutPressed() {
    if (!inDebugger) {
        return;
    }

    debuggerUpdateChanges();
    disconnect(stepOutShortcut, &QShortcut::activated, this, &MainWindow::stepOutPressed);
    emit setDebugStepOutMode();
}
