/* Copyright (C) 2015-2016
 * Parts derived from Firebird by Fabian Vogt
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkAccessManager>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QProgressDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QComboBox>
#include <QtQuickWidgets/QQuickWidget>
#include <QtWidgets/QScrollBar>
#include <QtGui/QFont>
#include <QtGui/QPixmap>

#include <fstream>

#ifdef _MSC_VER
    #include <direct.h>
    #define chdir _chdir
#else
    #include <unistd.h>
#endif

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "lcdpopout.h"
#include "emuthread.h"
#include "qmlbridge.h"
#include "qtframebuffer.h"
#include "qtkeypadbridge.h"
#include "searchwidget.h"
#include "basiccodeviewerwindow.h"
#include "cemuopts.h"
#include "utils.h"
#include "capture/gif.h"

#include "../../core/schedule.h"
#include "../../core/debug/disasm.h"
#include "../../core/link.h"
#include "../../core/os/os.h"

#include "../../tests/autotester/crc32.hpp"
#include "../../tests/autotester/autotester.h"

static const constexpr int WindowStateVersion = 0;

MainWindow::MainWindow(CEmuOpts cliOpts,QWidget *p) :QMainWindow(p), ui(new Ui::MainWindow) {
    opts = cliOpts;

    // Setup the UI1
    ui->setupUi(this);
    ui->centralWidget->hide();
    ui->statusBar->addWidget(&statusLabel);
    ui->lcdWidget->setLCD(&lcd);

    // Allow for 2000 lines of logging
    ui->console->setMaximumBlockCount(2000);

    // Register QtKeypadBridge for the virtual keyboard functionality
    installEventFilter(&qt_keypad_bridge);
    ui->lcdWidget->installEventFilter(&qt_keypad_bridge);
    // Same for all the tabs/docks (iterate over them instead of harcoding their names)
    for (const auto& tab : ui->tabWidget->children()[0]->children()) {
        tab->installEventFilter(&qt_keypad_bridge);
    }

    ui->keypadWidget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);

    // Emulator -> GUI
    connect(&emu, &EmuThread::consoleStr, this, &MainWindow::consoleStr);
    connect(&emu, &EmuThread::errConsoleStr, this, &MainWindow::errConsoleStr);
    connect(&emu, &EmuThread::restored, this, &MainWindow::restored, Qt::QueuedConnection);
    connect(&emu, &EmuThread::saved, this, &MainWindow::saved, Qt::QueuedConnection);
    connect(&emu, &EmuThread::isBusy, this, &MainWindow::isBusy, Qt::QueuedConnection);

    // Console actions
    connect(ui->buttonConsoleclear, &QPushButton::clicked, ui->console, &QPlainTextEdit::clear);
    connect(ui->radioConsole, &QRadioButton::clicked, this, &MainWindow::consoleOutputChanged);
    connect(ui->radioStderr, &QRadioButton::clicked, this, &MainWindow::consoleOutputChanged);

    // Debugger
    connect(&emu, &EmuThread::raiseDebugger, this, &MainWindow::raiseDebugger, Qt::QueuedConnection);
    connect(&emu, &EmuThread::disableDebugger, this, &MainWindow::disableDebugger, Qt::QueuedConnection);
    connect(&emu, &EmuThread::sendDebugCommand, this, &MainWindow::processDebugCommand, Qt::QueuedConnection);
    connect(this, &MainWindow::debuggerChangedState, &emu, &EmuThread::setDebugMode);
    connect(this, &MainWindow::setDebugStepInMode, &emu, &EmuThread::setDebugStepInMode);
    connect(this, &MainWindow::setDebugStepOverMode, &emu, &EmuThread::setDebugStepOverMode);
    connect(this, &MainWindow::setDebugStepNextMode, &emu, &EmuThread::setDebugStepNextMode);
    connect(this, &MainWindow::setDebugStepOutMode, &emu, &EmuThread::setDebugStepOutMode);
    connect(ui->buttonRun, &QPushButton::clicked, this, &MainWindow::changeDebuggerState);
    connect(ui->tabDebugging, &QTabWidget::currentChanged, this, &MainWindow::updateDisassembly);
    connect(ui->buttonAddPort, &QPushButton::clicked, this, &MainWindow::addPort);
    connect(ui->buttonRemovePort, &QPushButton::clicked, this, &MainWindow::removePort);
    connect(ui->buttonAddBreakpoint, &QPushButton::clicked, this, &MainWindow::addBreakpoint);
    connect(ui->buttonRemoveBreakpoint, &QPushButton::clicked, this, &MainWindow::removeBreakpoint);
    connect(ui->buttonAddWatchpoint, &QPushButton::clicked, this, &MainWindow::addWatchpoint);
    connect(ui->buttonRemoveWatchpoint, &QPushButton::clicked, this, &MainWindow::removeWatchpoint);
    connect(ui->buttonStepIn, &QPushButton::clicked, this, &MainWindow::stepInPressed);
    connect(ui->buttonStepOver, &QPushButton::clicked, this, &MainWindow::stepOverPressed);
    connect(ui->buttonStepNext, &QPushButton::clicked, this, &MainWindow::stepNextPressed);
    connect(ui->buttonStepOut, &QPushButton::clicked, this, &MainWindow::stepOutPressed);
    connect(ui->buttonGoto, &QPushButton::clicked, this, &MainWindow::gotoPressed);
    connect(ui->disassemblyView, &QWidget::customContextMenuRequested, this, &MainWindow::disasmContextMenu);
    connect(ui->vatView, &QWidget::customContextMenuRequested, this, &MainWindow::vatContextMenu);
    connect(ui->opView, &QWidget::customContextMenuRequested, this, &MainWindow::opContextMenu);
    connect(ui->portView, &QTableWidget::itemChanged, this, &MainWindow::changePortValues);
    connect(ui->portView, &QTableWidget::itemPressed, this, &MainWindow::setPreviousPortValues);
    connect(ui->breakpointView, &QTableWidget::itemChanged, this, &MainWindow::changeBreakpointAddress);
    connect(ui->breakpointView, &QTableWidget::itemPressed, this, &MainWindow::setPreviousBreakpointAddress);
    connect(ui->watchpointView, &QTableWidget::itemChanged, this, &MainWindow::changeWatchpointAddress);
    connect(ui->watchpointView, &QTableWidget::itemPressed, this, &MainWindow::setPreviousWatchpointAddress);
    connect(ui->checkCharging, &QCheckBox::toggled, this, &MainWindow::changeBatteryCharging);
    connect(ui->sliderBattery, &QSlider::valueChanged, this, &MainWindow::changeBatteryStatus);
    connect(ui->disassemblyView->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::scrollDisasmView);

    // Debugger Options
    connect(ui->buttonAddEquateFile, &QPushButton::clicked, this, &MainWindow::addEquateFileDialog);
    connect(ui->buttonClearEquates, &QPushButton::clicked, this, &MainWindow::clearEquateFile);
    connect(ui->buttonRefreshEquates, &QPushButton::clicked, this, &MainWindow::refreshEquateFile);
    connect(ui->textSizeSlider, &QSlider::valueChanged, this, &MainWindow::setFont);

    // Linking
    connect(ui->buttonSend, &QPushButton::clicked, this, &MainWindow::selectFiles);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::selectFiles);
    connect(this, &MainWindow::setSendState, &emu, &EmuThread::setSendState);
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
    connect(ui->scaleSlider, &QSlider::sliderMoved, this, &MainWindow::reprintScale);
    connect(ui->scaleSlider, &QSlider::valueChanged, this, &MainWindow::changeScale);
    connect(ui->checkSkin, &QCheckBox::stateChanged, this, &MainWindow::toggleSkin);
    connect(ui->refreshSlider, &QSlider::valueChanged, this, &MainWindow::changeLCDRefresh);
    connect(ui->checkAlwaysOnTop, &QCheckBox::stateChanged, this, &MainWindow::alwaysOnTop);
    connect(ui->emulationSpeed, &QSlider::valueChanged, this, &MainWindow::changeEmulatedSpeed);
    connect(ui->checkThrottle, &QCheckBox::stateChanged, this, &MainWindow::changeThrottleMode);
    connect(ui->lcdWidget, &QWidget::customContextMenuRequested, this, &MainWindow::screenContextMenu);
    connect(ui->checkRestore, &QCheckBox::stateChanged, this, &MainWindow::setRestoreOnOpen);
    connect(ui->checkSave, &QCheckBox::stateChanged, this, &MainWindow::setSaveOnClose);
    connect(ui->buttonChangeSavedImagePath, &QPushButton::clicked, this, &MainWindow::changeImagePath);
    connect(this, &MainWindow::changedEmuSpeed, &emu, &EmuThread::changeEmuSpeed);
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

    // Auto Updates
    connect(ui->checkUpdates, &QCheckBox::stateChanged, this, &MainWindow::autoCheckForUpdates);

    // Shortcut Connections
    stepInShortcut = new QShortcut(QKeySequence(Qt::Key_F6), this);
    stepOverShortcut = new QShortcut(QKeySequence(Qt::Key_F7), this);
    stepNextShortcut = new QShortcut(QKeySequence(Qt::Key_F8), this);
    stepOutShortcut = new QShortcut(QKeySequence(Qt::Key_F9), this);
    debuggerShortcut = new QShortcut(QKeySequence(Qt::Key_F10), this);

    debuggerShortcut->setAutoRepeat(false);
    stepInShortcut->setAutoRepeat(false);
    stepOverShortcut->setAutoRepeat(false);
    stepNextShortcut->setAutoRepeat(false);
    stepOutShortcut->setAutoRepeat(false);

    connect(debuggerShortcut, &QShortcut::activated, this, &MainWindow::changeDebuggerState);
    connect(stepInShortcut, &QShortcut::activated, this, &MainWindow::stepInPressed);
    connect(stepOverShortcut, &QShortcut::activated, this, &MainWindow::stepOverPressed);
    connect(stepNextShortcut, &QShortcut::activated, this, &MainWindow::stepNextPressed);
    connect(stepOutShortcut, &QShortcut::activated, this, &MainWindow::stepOutPressed);

    // Meta Types
    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<std::string>("std::string");

    ui->portView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->breakpointView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    setUIMode(true);
    setAcceptDrops(true);
    debuggerOn = false;

    autotester::stepCallback = []() { QApplication::processEvents(); };

    settings = new QSettings(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/CEmu/cemu_config.ini"), QSettings::IniFormat);

#ifdef _WIN32
    installToggleConsole();
#endif

    changeThrottleMode(Qt::Checked);
    emu.rom = settings->value(QStringLiteral("romImage")).toString().toStdString();
    changeFrameskip(settings->value(QStringLiteral("frameskip"), 3).toUInt());
    changeScale(settings->value(QStringLiteral("scale"), 100).toUInt());
    toggleSkin(settings->value(QStringLiteral("skin"), 1).toBool());
    changeLCDRefresh(settings->value(QStringLiteral("refreshRate"), 60).toUInt());
    changeEmulatedSpeed(settings->value(QStringLiteral("emuRate"), 10).toUInt());
    setFont(settings->value(QStringLiteral("textSize"), 9).toUInt());
    autoCheckForUpdates(settings->value(QStringLiteral("autoUpdate"), false).toBool());
    setSaveOnClose(settings->value(QStringLiteral("saveOnClose"), true).toBool());
    setRestoreOnOpen(settings->value(QStringLiteral("restoreOnOpen"), true).toBool());
    ui->flashBytes->setValue(settings->value(QStringLiteral("flashBytesPerLine"), 8).toInt());
    ui->ramBytes->setValue(settings->value(QStringLiteral("ramBytesPerLine"), 8).toInt());
    ui->memBytes->setValue(settings->value(QStringLiteral("memBytesPerLine"), 8).toInt());

    currentDir.setPath((settings->value(QStringLiteral("currDir"), QDir::homePath()).toString()));
    if(settings->value(QStringLiteral("savedImagePath")).toString().isEmpty()) {
        QString path = QDir::cleanPath(QFileInfo(settings->fileName()).absoluteDir().absolutePath() + QStringLiteral("/cemu_image.ce"));
        settings->setValue(QStringLiteral("savedImagePath"),path);
    }
    ui->savedImagePath->setText(settings->value(QStringLiteral("savedImagePath")).toString());
    emu.imagePath = ui->savedImagePath->text().toStdString();

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
    changeKeymap(currKeyMap);

    ui->rompathView->setText(QString::fromStdString(emu.rom));
    ui->emuVarView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->vatView->cursorState(true);
    ui->opView->cursorState(true);
    ui->opView->updateAllHighlights();
    ui->vatView->updateAllHighlights();

    debugger_init();

    if (!fileExists(emu.rom)) {
        if (!runSetup()) {
            exit(0);
        }
    } else {
        if(settings->value(QStringLiteral("restoreOnOpen")).toBool() && fileExists(emu.imagePath) && opts.restoreOnOpen ) {
            restoreEmuState();
        } else {
            emu.start();
        }
    }

    speedUpdateTimer.start();
    speedUpdateTimer.setInterval(1000 / 4);

    colorback.setColor(QPalette::Base, QColor(Qt::yellow).lighter(160));
    nocolorback.setColor(QPalette::Base, QColor(Qt::white));
    alwaysOnTop(settings->value(QStringLiteral("onTop"), 0).toUInt());
    restoreGeometry(settings->value(QStringLiteral("windowGeometry")).toByteArray());
    restoreState(settings->value(QStringLiteral("windowState")).toByteArray(), WindowStateVersion);
    console_format = ui->console->currentCharFormat();

    QPixmap pix;

    pix.load(":/icons/resources/icons/stop.png");
    stopIcon.addPixmap(pix);
    pix.load(":/icons/resources/icons/run.png");
    runIcon.addPixmap(pix);

    if (opts.AutotesterFile != ""){
        if(openJSONConfig(opts.AutotesterFile)==0)
        {
           resetCalculator();
           //Race condition requires this. Else We hand onsending files.
           QThread::msleep(500);
           launchTest();
        }
   }

}

MainWindow::~MainWindow() {
    debugger_free();

    settings->setValue(QStringLiteral("windowState"), saveState(WindowStateVersion));
    settings->setValue(QStringLiteral("windowGeometry"), saveGeometry());
    settings->setValue(QStringLiteral("currDir"), currentDir.absolutePath());
    settings->setValue(QStringLiteral("flashBytesPerLine"), ui->flashBytes->value());
    settings->setValue(QStringLiteral("ramBytesPerLine"), ui->ramBytes->value());
    settings->setValue(QStringLiteral("memBytesPerLine"), ui->memBytes->value());

    delete settings;
    delete ui->flashEdit;
    delete ui->ramEdit;
    delete ui->memEdit;
    delete ui;
}

void MainWindow::changeImagePath() {
    QString saveImagePath = QFileDialog::getSaveFileName(this, tr("Select saved image to restore from"),
                                                         currentDir.absolutePath(),
                                                         tr("CEmu images (*.ce);;All files (*.*)"));
    if(!saveImagePath.isEmpty()) {
        currentDir = QFileInfo(saveImagePath).absoluteDir();
        settings->setValue(QStringLiteral("savedImagePath"), QVariant(saveImagePath.toStdString().c_str()));
        ui->savedImagePath->setText(saveImagePath);
    }
}

bool MainWindow::restoreEmuState() {
    QString default_savedImage = settings->value(QStringLiteral("savedImagePath")).toString();
    if(!default_savedImage.isEmpty()) {
        return restoreFromPath(default_savedImage);
    } else {
        QMessageBox::warning(this, tr("Can't restore state"), tr("No saved image path in settings"));
        return false;
    }
}

void MainWindow::saveToPath(QString path) {
    emu_thread->save(path);
}

bool MainWindow::restoreFromPath(QString path) {
    if (inReceivingMode) {
        refreshVariableList();
    }
    if(!emu_thread->restore(path)) {
        QMessageBox::warning(this, tr("Could not restore"), tr("Try restarting"));
        return false;
    }

    return true;
}

void MainWindow::setSaveOnClose(bool b) {
    ui->checkSave->setChecked(b);
    settings->setValue(QStringLiteral("saveOnClose"), b);
}

void MainWindow::setRestoreOnOpen(bool b) {
    ui->checkRestore->setChecked(b);
    settings->setValue(QStringLiteral("restoreOnOpen"), b);
}

void MainWindow::saveEmuState() {
    QString default_savedImage = settings->value(QStringLiteral("savedImagePath")).toString();
    if(!default_savedImage.isEmpty()) {
        saveToPath(default_savedImage);
    } else {
        QMessageBox::warning(this, tr("Can't save image"), tr("No saved image path in settings given"));
    }
}

void MainWindow::restoreFromFile() {
    QString savedImage = QFileDialog::getOpenFileName(this, tr("Select saved image to restore from"),
                                                      currentDir.absolutePath(),
                                                      tr("CEmu images (*.ce);;All files (*.*)"));
    if(!savedImage.isEmpty()) {
        currentDir = QFileInfo(savedImage).absoluteDir();
        restoreFromPath(savedImage);
    }
}

void MainWindow::saveToFile() {
    QString savedImage = QFileDialog::getSaveFileName(this, tr("Set image to save to"),
                                                      currentDir.absolutePath(),
                                                      tr("CEmu images (*.ce);;All files (*.*)"));
    if(!savedImage.isEmpty()) {
        currentDir = QFileInfo(savedImage).absoluteDir();
        saveToPath(savedImage);
    }
}
void MainWindow::exportRom() {
    QString saveRom = QFileDialog::getSaveFileName(this, tr("Set Rom image to save to"),
                                                      currentDir.absolutePath(),
                                                      tr("ROM images (*.rom);;All files (*.*)"));
    if(!saveRom.isEmpty()) {
        currentDir = QFileInfo(saveRom).absoluteDir();
        emu_thread->saveRomImage(saveRom);
    }
}

void MainWindow::restored(bool success) {
    if(success) {
        showStatusMsg(tr("Emulation restored from image."));
    } else {
        QMessageBox::warning(this, tr("Could not restore"), tr("Resuming failed.\nPlease Reload your ROM."));
    }
}

void MainWindow::saved(bool success) {
    if(success) {
        showStatusMsg(tr("Image saved."));
    } else {
        QMessageBox::warning(this, tr("Could not save"), tr("Saving failed.\nSaving failed, go tell someone."));
    }

    if(closeAfterSave) {
        if(!success) {
            closeAfterSave = false;
        } else {
            this->close();
        }
    }
}

void MainWindow::dropEvent(QDropEvent *e) {
    const QMimeData* mime_data = e->mimeData();
    if (!mime_data->hasUrls()) {
        return;
    }

    QStringList files;
    for(auto &&url : mime_data->urls()) {
        files.append(url.toLocalFile());
    }

    sendFiles(files);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasUrls() == false || inReceivingMode) {
        return e->ignore();
    }

    for(QUrl &url : e->mimeData()->urls()) {
        static const QStringList valid_suffixes = { QStringLiteral("8xp"),
                                                    QStringLiteral("8xv"),
                                                    QStringLiteral("8xl"),
                                                    QStringLiteral("8xn"),
                                                    QStringLiteral("8xm"),
                                                    QStringLiteral("8xy"),
                                                    QStringLiteral("8xg"),
                                                    QStringLiteral("8xs"),
                                                    QStringLiteral("8xd"),
                                                    QStringLiteral("8xw"),
                                                    QStringLiteral("8xc"),
                                                    QStringLiteral("8xz"),
                                                    QStringLiteral("8xt"),
                                                    QStringLiteral("8ca"),
                                                    QStringLiteral("8ci") };

        QFileInfo file(url.fileName());
        if(!valid_suffixes.contains(file.suffix().toLower()))
            return e->ignore();
    }

    e->accept();
}

void MainWindow::closeEvent(QCloseEvent *e) {
    if (inDebugger) {
        changeDebuggerState();
    }
    if (inReceivingMode) {
        refreshVariableList();
    }

    if (!closeAfterSave && settings->value(QStringLiteral("saveOnClose")).toBool() && opts.restoreOnOpen) {
            closeAfterSave = true;
            qDebug("Saving...");
            saveEmuState();
            e->ignore();
            return;
    }

    if (!emu.stop()) {
        qDebug("Thread Termmination Failed.");
    }

    speedUpdateTimer.stop();

    QMainWindow::closeEvent(e);
}

void MainWindow::appendToConsole(QString str, QColor color) {
    QTextCursor cur(ui->console->document());
    cur.movePosition(QTextCursor::End);
    console_format.setForeground(color);
    cur.insertText(str, console_format);
    if (ui->checkAutoScroll->isChecked()) {
        ui->console->setTextCursor(cur);
    }
}

void MainWindow::consoleStr(QString str) {
    if (native_console) {
        fputs(str.toStdString().c_str(), stdout);
    } else {
        appendToConsole(str);
    }
}

void MainWindow::errConsoleStr(QString str) {
    if (native_console) {
        fputs(str.toStdString().c_str(), stderr);
    } else {
        appendToConsole(str, Qt::red);
    }
}

void MainWindow::changeThrottleMode(int mode) {
    ui->checkThrottle->setChecked(mode == Qt::Checked);
    emit changedThrottleMode(mode == Qt::Checked);
}

void MainWindow::showActualSpeed(int speed) {
    showStatusMsg(QStringLiteral(" ") + tr("Actual Speed: ") + QString::number(speed, 10) + QStringLiteral("%"));
}

void MainWindow::showStatusMsg(QString str) {
    statusLabel.setText(str);
}

bool MainWindow::runSetup() {
    RomSelection romSelection;
    romSelection.show();
    romSelection.exec();

    emu.rom = romSelection.getROMImage();

    if (emu.rom.empty()) {
        return false;
    } else {
        if (inReceivingMode) {
            refreshVariableList();
        }
        if(debuggerOn) {
            changeDebuggerState();
        }
        settings->setValue(QStringLiteral("romImage"), QVariant(emu.rom.c_str()));
        if(emu.stop()) {
            speedUpdateTimer.stop();
            ui->rompathView->setText(emu.rom.c_str());
            emu.start();
            speedUpdateTimer.start();
            speedUpdateTimer.setInterval(1000 / 2);
        }
    }

    return true;
}

void MainWindow::setUIMode(bool docks_enabled) {
    // Already in this mode?
    if (docks_enabled == ui->tabWidget->isHidden()) {
        return;
    }

    // Create "Docks" menu to make closing and opening docks more intuitive
    QMenu *docksMenu = new QMenu(tr("Docks"), this);
    ui->menubar->insertMenu(ui->menuAbout->menuAction(), docksMenu);

    //Convert the tabs into QDockWidgets
    QDockWidget *last_dock = nullptr;
    while(ui->tabWidget->count()) {
        QDockWidget *dw = new QDockWidget(ui->tabWidget->tabText(0));
        dw->setWindowIcon(ui->tabWidget->tabIcon(0));
        dw->setObjectName(dw->windowTitle());

        // Fill "Docks" menu
        QAction *action = dw->toggleViewAction();
        action->setIcon(dw->windowIcon());
        docksMenu->addAction(action);

        QWidget *tab = ui->tabWidget->widget(0);
        if(tab == ui->tabDebugger)
            debuggerDock = dw;

        dw->setWidget(tab);

        addDockWidget(Qt::RightDockWidgetArea, dw);
        if(last_dock != nullptr)
            tabifyDockWidget(last_dock, dw);

        last_dock = dw;
    }

    ui->tabWidget->setHidden(true);
}

void MainWindow::saveScreenshot(QString namefilter, QString defaultsuffix, QString temppath) {
    QFileDialog dialog(this);

    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setDirectory(currentDir);
    dialog.setNameFilter(namefilter);
    dialog.setWindowTitle("Save Screen");
    dialog.setDefaultSuffix(defaultsuffix);
    dialog.exec();

    if(!(dialog.selectedFiles().isEmpty())) {
        QString filename = dialog.selectedFiles().at(0);
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

    saveScreenshot(tr("PNG images (*.png)"), QStringLiteral("png"), path);
}

void MainWindow::screenshotGIF() {
    if (ui->actionRecordGIF->isChecked()) {
        QMessageBox::warning(this, tr("Recording GIF"), tr("Currently recording GIF."));
        return;
    }

    QString path = QDir::tempPath() + QDir::separator() + QStringLiteral("cemu_tmp.img");
    if (!gif_single_frame(path.toStdString().c_str())) {
        QMessageBox::critical(this, tr("Screenshot failed"), tr("Failed to save screenshot!"));
    }

    saveScreenshot(tr("GIF images (*.gif)"), QStringLiteral("gif"), path);
}

void MainWindow::recordGIF() {
  static QString path;

  if (path.isEmpty()) {
      // TODO: Use QTemporaryFile?
      path = QDir::tempPath() + QDir::separator() + QStringLiteral("cemu_tmp.gif");

        gif_start_recording(path.toStdString().c_str(), ui->frameskipSlider->value());
    } else {
        if (gif_stop_recording()) {
            saveScreenshot(tr("GIF images (*.gif)"), QStringLiteral("gif"), path);
        } else {
            QMessageBox::warning(this, tr("Failed recording GIF"), tr("A failure occured during recording"));
        }
        path = QString();
    }

    ui->frameskipSlider->setEnabled(path.isEmpty());
    ui->actionRecordGIF->setChecked(!path.isEmpty());
    ui->buttonGIF->setText((!path.isEmpty()) ? QString("Stop Recording") : QString("Record GIF"));
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

void MainWindow::autoCheckForUpdates(int state) {
    settings->setValue(QStringLiteral("autoUpdate"), state);
    ui->checkUpdates->setChecked(state);

    if(state == Qt::Checked) {
        checkForUpdates(true);
    }
}

void MainWindow::checkForUpdates(bool forceInfoBox) {
    if (QStringLiteral(CEMU_VERSION).contains(QStringLiteral("dev")))
    {
        if (forceInfoBox)
        {
            QMessageBox::warning(this, tr("Update check disabled"), tr("Checking updates is disabled for development builds"));
        }
        return;
    }

    static const QString currentVersionReleaseURL = QStringLiteral("https://github.com/CE-Programming/CEmu/releases/tag/" CEMU_VERSION);
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply) {
        QString newVersionURL = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
        if (!newVersionURL.isEmpty())
        {
            if (newVersionURL.compare(currentVersionReleaseURL) == 0)
            {
                if (forceInfoBox)
                {
                    QMessageBox::information(this, tr("No update available"), tr("You already have the latest CEmu version (" CEMU_VERSION ")"));
                }
            } else {
                QMessageBox updateInfoBox(this);
                updateInfoBox.addButton(QMessageBox::Ok);
                updateInfoBox.setIconPixmap(QPixmap(":/icons/resources/icons/icon.png"));
                updateInfoBox.setWindowTitle(tr("CEmu update"));
                updateInfoBox.setText(tr("<b>A new version of CEmu is available!</b>"
                                         "<br/>"
                                         "You can <a href='%1'>download it here</a>.")
                                     .arg(newVersionURL));
                updateInfoBox.setTextFormat(Qt::RichText);
                updateInfoBox.show();
                updateInfoBox.exec();
            }
        } else { // No internet connection? GitHub doesn't provide this redirection anymore?
            if (forceInfoBox)
            {
                QMessageBox updateInfoBox(this);
                updateInfoBox.addButton(QMessageBox::Ok);
                updateInfoBox.setIcon(QMessageBox::Warning);
                updateInfoBox.setWindowTitle(tr("Update check failed"));
                updateInfoBox.setText(tr("<b>An error occurred while checking for CEmu updates.</b>"
                                         "<br/>"
                                         "You can however <a href='https://github.com/CE-Programming/CEmu/releases/latest'>go here</a> to check yourself."));
                updateInfoBox.setTextFormat(Qt::RichText);
                updateInfoBox.show();
                updateInfoBox.exec();
            }
        }
    });

    manager->get(QNetworkRequest(QUrl(QStringLiteral("https://github.com/CE-Programming/CEmu/releases/latest"))));
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

void MainWindow::adjustScreen() {
    float scale = ui->scaleSlider->value() / 100.0;
    bool skin = ui->checkSkin->isChecked();
    ui->calcSkinTop->setVisible(skin);
    float w, h;
    w = 320 * scale;
    h = 240 * scale;
    ui->lcdWidget->setFixedSize(w, h);
    ui->lcdWidget->move(skin ? 60 * scale : 0, skin ? 78 * scale : 0);
    if (skin) {
        w = 440 * scale;
        h = 351 * scale;
    }
    ui->calcSkinTop->resize(w, h);
    ui->screenWidgetContents->setFixedSize(w, h);
}

int MainWindow::reprintScale(int scale) {
    int roundedScale = round(scale / 50.0) * 50;
    ui->scaleLabel->setText(QString::number(roundedScale) + "%");
    return roundedScale;
}

void MainWindow::changeScale(int scale) {
    int roundedScale = reprintScale(scale);
    settings->setValue(QStringLiteral("scale"), roundedScale);
    ui->scaleSlider->setValue(roundedScale);
    adjustScreen();
}

void MainWindow::toggleSkin(bool enable) {
    settings->setValue(QStringLiteral("skin"), enable);
    ui->checkSkin->setChecked(enable);
    adjustScreen();
}

void MainWindow::changeLCDRefresh(int value) {
    settings->setValue(QStringLiteral("refreshRate"), value);
    ui->refreshLabel->setText(QString::number(value)+" FPS");
    ui->refreshSlider->setValue(value);
    ui->lcdWidget->refreshRate(value);
    changeFramerate();
}

void MainWindow::changeEmulatedSpeed(int value) {
    int actualSpeed = value*10;
    settings->setValue(QStringLiteral("emuRate"), value);
    ui->emulationSpeedLabel->setText(QString::number(actualSpeed).rightJustified(3, '0')+QStringLiteral("%"));
    ui->emulationSpeed->setValue(value);
    emit changedEmuSpeed(actualSpeed);
}

void MainWindow::consoleOutputChanged() {
    native_console = ui->radioStderr->isChecked();
}

void MainWindow::isBusy(bool busy) {
    if(busy) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    } else {
        QApplication::restoreOverrideCursor();
    }
}

void MainWindow::keymapChanged() {
    if (ui->radioCEmuKeys->isChecked()) {
        changeKeymap(QStringLiteral("cemu"));
    } else if (ui->radioTilEmKeys->isChecked()) {
        changeKeymap(QStringLiteral("tilem"));
    } else if (ui->radioWabbitemuKeys->isChecked()) {
        changeKeymap(QStringLiteral("wabbitemu"));
    } else if (ui->radiojsTIfiedKeys->isChecked()) {
        changeKeymap(QStringLiteral("jsTIfied"));
    }
}

void MainWindow::changeKeymap(const QString & value) {
    settings->setValue(QStringLiteral("keyMap"), value);
    qt_keypad_bridge.setKeymap(value);
}

void MainWindow::alwaysOnTop(int state) {
    if (!state) {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    } else {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    }
    show();
    settings->setValue(QStringLiteral("onTop"), state);
    ui->checkAlwaysOnTop->setCheckState(Qt::CheckState(state));
}

/* ================================================ */
/* Linking Things                                   */
/* ================================================ */

QStringList MainWindow::showVariableFileDialog(QFileDialog::AcceptMode mode, QString name_filter) {
    QFileDialog dialog(this);
    int good;

    dialog.setAcceptMode(mode);
    dialog.setFileMode(mode == QFileDialog::AcceptOpen ? QFileDialog::ExistingFiles : QFileDialog::AnyFile);
    dialog.setDirectory(currentDir);
    dialog.setNameFilter(name_filter);
    dialog.setDefaultSuffix("8xg");
    good = dialog.exec();

    currentDir = dialog.directory();

    if (good) {
        return dialog.selectedFiles();
    }

    return QStringList();
}

void MainWindow::sendFiles(QStringList fileNames) {
    if (inDebugger) {
        return;
    }

    setSendState(true);
    const unsigned int fileNum = fileNames.size();

    if (fileNum == 0) {
        setSendState(false);
        return;
    }

    /* Wait for an open link */
    emu_thread->waitForLink = true;
    do {
        QThread::msleep(50);
    } while(emu_thread->waitForLink);

    QProgressDialog progress("Sending Files...", QString(), 0, fileNum, this);
    progress.setWindowModality(Qt::WindowModal);

    progress.show();
    QApplication::processEvents();

    for (unsigned int i = 0; i < fileNum; i++) {
        if(!sendVariableLink(fileNames.at(i).toUtf8())) {
            QMessageBox::warning(this, tr("Failed Transfer"), tr("A failure occured during transfer of: ")+fileNames.at(i));
        }
        progress.setLabelText(fileNames.at(i).toUtf8());
        progress.setValue(progress.value()+1);
        QApplication::processEvents();
    }

    progress.setValue(progress.value()+1);
    QApplication::processEvents();
    QThread::msleep(100);

    setSendState(false);
}

void MainWindow::selectFiles() {
    if (debuggerOn) {
       return;
    }

    QStringList fileNames = showVariableFileDialog(QFileDialog::AcceptOpen, tr("TI Variable (*.8xp *.8xv *.8xl *.8xn *.8xm *.8xy *.8xg *.8xs *.8xd *.8xw *.8xc *.8xl *.8xz *.8xt *.8ca, *.8cg, *.8ci, *.8ek);;All Files (*.*)"));

    sendFiles(fileNames);
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
    int currentRow;
    calc_var_t var;

    while(ui->emuVarView->rowCount() > 0) {
        ui->emuVarView->removeRow(0);
    }

    if (debuggerOn) {
        return;
    }

    if (inReceivingMode) {
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
        QThread::msleep(200);

        vat_search_init(&var);
        vars.clear();
        while (vat_search_next(&var)) {
            if (var.size > 2) {
                vars.append(var);
                currentRow = ui->emuVarView->rowCount();
                ui->emuVarView->setRowCount(currentRow + 1);

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
                var_name->setData(Qt::UserRole, currentRow);

                var_name->setCheckState(Qt::Unchecked);

                if (var_preview_needs_gray) {
                    var_preview->setForeground(Qt::gray);
                }

                ui->emuVarView->setItem(currentRow, 0, var_name);
                ui->emuVarView->setItem(currentRow, 1, var_type);
                ui->emuVarView->setItem(currentRow, 2, var_size);
                ui->emuVarView->setItem(currentRow, 3, var_preview);
            }
        }
        ui->emuVarView->resizeColumnsToContents();
        ui->emuVarView->horizontalHeader()->setStretchLastSection(true);
        ui->emuVarView->setVisible(false);  // This is needed
        ui->emuVarView->setVisible(true);   // to refresh
    }

    ui->emuVarView->blockSignals(false);
    inReceivingMode = !inReceivingMode;
}

void MainWindow::saveSelected() {
    uint8_t i;
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
    for (int currentRow = 0; currentRow < ui->emuVarView->rowCount(); currentRow++) {
        if (ui->emuVarView->item(currentRow, 0)->checkState()) {
            selectedVars.append(vars[currentRow]);
        }
    }
    if (selectedVars.size() < 1) {
        QMessageBox::warning(this, tr("No transfer to do"), tr("Select at least one file to transfer"));
    } else {
        if (selectedVars.size() == 1) {
            i = selectedVars.at(0).type1;
            fileNames = showVariableFileDialog(QFileDialog::AcceptSave, "TI "+QString(calc_var_type_names[i])+" (*."+var_extension[i]+");;All Files (*.*)");
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

/* ================================================ */
/* Autotester Things                                */
/* ================================================ */

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
        ui->buttonReloadJSONconfig->setEnabled(true);
        chdir(QDir::toNativeSeparators(QFileInfo(jsonPath).absoluteDir().path()).toStdString().c_str());
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
        QThread::msleep(1000);
    }

    if (ui->checkBoxTestClear->isChecked()) {
        // Clear home screen
        autotester::sendKey(0x09);
    }

    QStringList filesList;
    for (const auto& file : autotester::config.transfer_files) {
        filesList << QString::fromStdString(file);
    }
    sendFiles(filesList);
    QThread::msleep(200);

    // Follow the sequence
    if (!autotester::doTestSequence()) {
        dispAutotesterError(1);
        return;
    }
    if(!opts.suppressTestDialog) {
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


/* ================================================ */
/* Debugger Things                                  */
/* ================================================ */

void MainWindow::setFont(int fontSize) {
    ui->textSizeSlider->setValue(fontSize);
    settings->setValue(QStringLiteral("textSize"), ui->textSizeSlider->value());

    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    monospace.setPointSize(fontSize);
    ui->console->setFont(monospace);
    ui->opView->setFont(monospace);
    ui->vatView->setFont(monospace);
    ui->disassemblyView->setFont(monospace);

    ui->stackView->setFont(monospace);

    ui->afregView->setFont(monospace);
    ui->hlregView->setFont(monospace);
    ui->deregView->setFont(monospace);
    ui->bcregView->setFont(monospace);
    ui->ixregView->setFont(monospace);
    ui->iyregView->setFont(monospace);
    ui->af_regView->setFont(monospace);
    ui->hl_regView->setFont(monospace);
    ui->de_regView->setFont(monospace);
    ui->bc_regView->setFont(monospace);
    ui->splregView->setFont(monospace);
    ui->spsregView->setFont(monospace);
    ui->mbregView->setFont(monospace);
    ui->iregView->setFont(monospace);
    ui->rregView->setFont(monospace);
    ui->imregView->setFont(monospace);
    ui->freqView->setFont(monospace);
    ui->pcregView->setFont(monospace);
    ui->lcdbaseView->setFont(monospace);
    ui->lcdcurrView->setFont(monospace);
}

static int hex2int(QString str) {
    return (int)strtol(str.toStdString().c_str(), nullptr, 16);
}

static QString int2hex(uint32_t a, uint8_t l) {
    return QString::number(a, 16).rightJustified(l, '0', true).toUpper();
}

void MainWindow::raiseDebugger() {
    populateDebugWindow();
    setDebuggerState(true);
    connect(stepInShortcut, &QShortcut::activated, this, &MainWindow::stepInPressed);
    connect(stepOverShortcut, &QShortcut::activated, this, &MainWindow::stepOverPressed);
    connect(stepNextShortcut, &QShortcut::activated, this, &MainWindow::stepNextPressed);
    connect(stepOutShortcut, &QShortcut::activated, this, &MainWindow::stepOutPressed);
}

void MainWindow::leaveDebugger() {
    setDebuggerState(false);
}

void MainWindow::updateDebuggerChanges() {
    if (debuggerOn == true) {
        return;
    }

    ui->disassemblyView->verticalScrollBar()->blockSignals(true);

    /* Update all the changes in the core */
    cpu.registers.AF = static_cast<uint16_t>(hex2int(ui->afregView->text()));
    cpu.registers.HL = static_cast<uint32_t>(hex2int(ui->hlregView->text()));
    cpu.registers.DE = static_cast<uint32_t>(hex2int(ui->deregView->text()));
    cpu.registers.BC = static_cast<uint32_t>(hex2int(ui->bcregView->text()));
    cpu.registers.IX = static_cast<uint32_t>(hex2int(ui->ixregView->text()));
    cpu.registers.IY = static_cast<uint32_t>(hex2int(ui->iyregView->text()));

    cpu.registers._AF = static_cast<uint16_t>(hex2int(ui->af_regView->text()));
    cpu.registers._HL = static_cast<uint32_t>(hex2int(ui->hl_regView->text()));
    cpu.registers._DE = static_cast<uint32_t>(hex2int(ui->de_regView->text()));
    cpu.registers._BC = static_cast<uint32_t>(hex2int(ui->bc_regView->text()));

    cpu.registers.SPL = static_cast<uint32_t>(hex2int(ui->splregView->text()));
    cpu.registers.SPS = static_cast<uint16_t>(hex2int(ui->spsregView->text()));

    cpu.registers.MBASE = static_cast<uint8_t>(hex2int(ui->mbregView->text()));
    cpu.registers.I = static_cast<uint16_t>(hex2int(ui->iregView->text()));
    cpu.registers.R = static_cast<uint8_t>(hex2int(ui->rregView->text()));
    cpu.registers.R = cpu.registers.R << 1 | cpu.registers.R >> 7;
    cpu.IM = static_cast<uint8_t>(hex2int(ui->imregView->text()));
    cpu.IM += !!cpu.IM;

    cpu.registers.flags.Z = ui->checkZ->isChecked();
    cpu.registers.flags.C = ui->checkC->isChecked();
    cpu.registers.flags.H = ui->checkHC->isChecked();
    cpu.registers.flags.PV = ui->checkPV->isChecked();
    cpu.registers.flags.N = ui->checkN->isChecked();
    cpu.registers.flags.S = ui->checkS->isChecked();
    cpu.registers.flags._5 = ui->check5->isChecked();
    cpu.registers.flags._3 = ui->check3->isChecked();

    cpu.halted = ui->checkHalted->isChecked();
    cpu.ADL = ui->checkADL->isChecked();
    cpu.MADL = ui->checkMADL->isChecked();
    cpu.halted = ui->checkHalted->isChecked();
    cpu.IEF1 = ui->checkIEF1->isChecked();
    cpu.IEF2 = ui->checkIEF2->isChecked();

    uint32_t uiPC = static_cast<uint32_t>(hex2int(ui->pcregView->text()));
    if (cpu.registers.PC != uiPC) {
        cpu_flush(uiPC, ui->checkADL->isChecked());
    }

    backlight.brightness = static_cast<uint8_t>(ui->brightnessSlider->value());

    lcd.upbase = static_cast<uint32_t>(hex2int(ui->lcdbaseView->text()));
    lcd.upcurr = static_cast<uint32_t>(hex2int(ui->lcdcurrView->text()));
    lcd.control &= ~14;

    uint8_t bpp = 0;
    switch(ui->bppView->text().toInt()) {
        case 1:
            bpp = 0; break;
        case 2:
            bpp = 1; break;
        case 4:
            bpp = 2; break;
        case 8:
            bpp = 3; break;
        case 24:
            bpp = 5; break;
        case 16:
            bpp = 6; break;
        case 12:
            bpp = 7; break;
    }

    lcd.control |= bpp<<1;

    if (ui->checkPowered->isChecked()) {
        lcd.control |= 0x800;
    } else {
        lcd.control &= ~0x800;
    }
    if (ui->checkBEPO->isChecked()) {
        lcd.control |= 0x400;
    } else {
        lcd.control &= ~0x400;
    }
    if (ui->checkBEBO->isChecked()) {
        lcd.control |= 0x200;
    } else {
        lcd.control &= ~0x200;
    }
    if (ui->checkBGR->isChecked()) {
        lcd.control |= 0x100;
    } else {
        lcd.control &= ~0x100;
    }
}

void MainWindow::setDebuggerState(bool state) {
    debuggerOn = state;

    if (debuggerOn) {
        ui->buttonRun->setText("Run");
        ui->buttonRun->setIcon(runIcon);
        debug_clear_run_until();
    } else {
        ui->buttonRun->setText("Stop");
        ui->buttonRun->setIcon(stopIcon);
        ui->portChangeLabel->clear();
        ui->portTypeLabel->clear();
        ui->breakChangeLabel->clear();
        ui->breakTypeLabel->clear();
        ui->opView->clear();
        ui->vatView->clear();
        ui->portChangeLabel->clear();
        ui->portTypeLabel->clear();
        ui->breakChangeLabel->clear();
        ui->breakTypeLabel->clear();
        ui->watchChangeLabel->clear();
        ui->watchTypeLabel->clear();
    }
    setReceiveState(false);

    ui->tabDebugging->setEnabled( debuggerOn );
    ui->buttonGoto->setEnabled( debuggerOn );
    ui->buttonStepIn->setEnabled( debuggerOn );
    ui->buttonStepOver->setEnabled( debuggerOn );
    ui->buttonStepNext->setEnabled( debuggerOn );
    ui->buttonStepOut->setEnabled( debuggerOn );
    ui->groupCPU->setEnabled( debuggerOn );
    ui->groupFlags->setEnabled( debuggerOn );
    ui->groupRegisters->setEnabled( debuggerOn );
    ui->groupInterrupts->setEnabled( debuggerOn );
    ui->groupStack->setEnabled( debuggerOn );
    ui->groupFlash->setEnabled( debuggerOn );
    ui->groupRAM->setEnabled( debuggerOn );
    ui->groupMem->setEnabled( debuggerOn );

    ui->actionRestoreState->setEnabled( !debuggerOn );
    ui->actionImportCalculatorState->setEnabled( !debuggerOn );
    ui->buttonSend->setEnabled( !debuggerOn );
    ui->buttonRefreshList->setEnabled( !debuggerOn );
    ui->emuVarView->setEnabled( !debuggerOn );
    ui->buttonReceiveFiles->setEnabled( !debuggerOn && inReceivingMode);
}

void MainWindow::changeDebuggerState() {
    if (emu.rom.empty()) {
        return;
    }

    debuggerOn = !debuggerOn;
    if (!debuggerOn) {
        setDebuggerState(false);
        updateDebuggerChanges();
        if (inReceivingMode) {
            inReceivingMode = false;
            refreshVariableList();
        }
    }
    emit debuggerChangedState( debuggerOn );
}

void MainWindow::populateDebugWindow() {
    QString tmp;

    tmp = int2hex(cpu.registers.AF, 4);
    ui->afregView->setPalette(tmp == ui->afregView->text() ? nocolorback : colorback);
    ui->afregView->setText(tmp);

    tmp = int2hex(cpu.registers.HL, 6);
    ui->hlregView->setPalette(tmp == ui->hlregView->text() ? nocolorback : colorback);
    ui->hlregView->setText(tmp);

    tmp = int2hex(cpu.registers.DE, 6);
    ui->deregView->setPalette(tmp == ui->deregView->text() ? nocolorback : colorback);
    ui->deregView->setText(tmp);

    tmp = int2hex(cpu.registers.BC, 6);
    ui->bcregView->setPalette(tmp == ui->bcregView->text() ? nocolorback : colorback);
    ui->bcregView->setText(tmp);

    tmp = int2hex(cpu.registers.IX, 6);
    ui->ixregView->setPalette(tmp == ui->ixregView->text() ? nocolorback : colorback);
    ui->ixregView->setText(tmp);

    tmp = int2hex(cpu.registers.IY, 6);
    ui->iyregView->setPalette(tmp == ui->iyregView->text() ? nocolorback : colorback);
    ui->iyregView->setText(tmp);

    tmp = int2hex(cpu.registers._AF, 4);
    ui->af_regView->setPalette(tmp == ui->af_regView->text() ? nocolorback : colorback);
    ui->af_regView->setText(tmp);

    tmp = int2hex(cpu.registers._HL, 6);
    ui->hl_regView->setPalette(tmp == ui->hl_regView->text() ? nocolorback : colorback);
    ui->hl_regView->setText(tmp);

    tmp = int2hex(cpu.registers._DE, 6);
    ui->de_regView->setPalette(tmp == ui->de_regView->text() ? nocolorback : colorback);
    ui->de_regView->setText(tmp);

    tmp = int2hex(cpu.registers._BC, 6);
    ui->bc_regView->setPalette(tmp == ui->bc_regView->text() ? nocolorback : colorback);
    ui->bc_regView->setText(tmp);

    tmp = int2hex(cpu.registers.SPS, 4);
    ui->spsregView->setPalette(tmp == ui->spsregView->text() ? nocolorback : colorback);
    ui->spsregView->setText(tmp);

    tmp = int2hex(cpu.registers.SPL, 6);
    ui->splregView->setPalette(tmp == ui->splregView->text() ? nocolorback : colorback);
    ui->splregView->setText(tmp);

    tmp = int2hex(cpu.registers.MBASE, 2);
    ui->mbregView->setPalette(tmp == ui->mbregView->text() ? nocolorback : colorback);
    ui->mbregView->setText(tmp);

    tmp = int2hex(cpu.registers.I, 4);
    ui->iregView->setPalette(tmp == ui->iregView->text() ? nocolorback : colorback);
    ui->iregView->setText(tmp);

    tmp = int2hex(cpu.IM - !!cpu.IM, 1);
    ui->imregView->setPalette(tmp == ui->imregView->text() ? nocolorback : colorback);
    ui->imregView->setText(tmp);

    tmp = int2hex(cpu.registers.PC, 6);
    ui->pcregView->setPalette(tmp == ui->pcregView->text() ? nocolorback : colorback);
    ui->pcregView->setText(tmp);

    tmp = int2hex(cpu.registers.R >> 1 | cpu.registers.R << 7, 2);
    ui->rregView->setPalette(tmp == ui->rregView->text() ? nocolorback : colorback);
    ui->rregView->setText(tmp);

    tmp = int2hex(lcd.upbase, 6);
    ui->lcdbaseView->setPalette(tmp == ui->lcdbaseView->text() ? nocolorback : colorback);
    ui->lcdbaseView->setText(tmp);

    tmp = int2hex(lcd.upcurr, 6);
    ui->lcdcurrView->setPalette(tmp == ui->lcdcurrView->text() ? nocolorback : colorback);
    ui->lcdcurrView->setText(tmp);

    tmp = QString::number(sched.clockRates[CLOCK_CPU]);
    ui->freqView->setPalette(tmp == ui->freqView->text() ? nocolorback : colorback);
    ui->freqView->setText(tmp);

    changeBatteryCharging(control.batteryCharging);
    changeBatteryStatus(control.setBatteryStatus);

    switch((lcd.control>>1)&7) {
        case 0:
            tmp = "01"; break;
        case 1:
            tmp = "02"; break;
        case 2:
            tmp = "04"; break;
        case 3:
            tmp = "08"; break;
        case 4:
            tmp = "16"; break;
        case 5:
            tmp = "24"; break;
        case 6:
            tmp = "16"; break;
        case 7:
            tmp = "12"; break;
    }

    ui->bppView->setPalette(tmp == ui->bppView->text() ? nocolorback : colorback);
    ui->bppView->setText(tmp);

    /* Mwhahaha */
    ui->checkSleep->setChecked(false);

    ui->check3->setChecked(cpu.registers.flags._3);
    ui->check5->setChecked(cpu.registers.flags._5);
    ui->checkZ->setChecked(cpu.registers.flags.Z);
    ui->checkC->setChecked(cpu.registers.flags.C);
    ui->checkHC->setChecked(cpu.registers.flags.H);
    ui->checkPV->setChecked(cpu.registers.flags.PV);
    ui->checkN->setChecked(cpu.registers.flags.N);
    ui->checkS->setChecked(cpu.registers.flags.S);

    ui->checkADL->setChecked(cpu.ADL);
    ui->checkMADL->setChecked(cpu.MADL);
    ui->checkHalted->setChecked(cpu.halted);
    ui->checkIEF1->setChecked(cpu.IEF1);
    ui->checkIEF2->setChecked(cpu.IEF2);

    ui->checkPowered->setChecked(lcd.control & 0x800);
    ui->checkBEPO->setChecked(lcd.control & 0x400);
    ui->checkBEBO->setChecked(lcd.control & 0x200);
    ui->checkBGR->setChecked(lcd.control & 0x100);
    ui->brightnessSlider->setValue(backlight.brightness);

    for(int i=0; i<ui->portView->rowCount(); ++i) {
        updatePortData(i);
    }

    for(int i=0; i<ui->watchpointView->rowCount(); ++i) {
        updateWatchpointData(i);
    }

    updateTIOSView();
    updateStackView();
    prevDisasmAddress = cpu.registers.PC;
    updateDisasmView(prevDisasmAddress, true);

    ramUpdate();
    flashUpdate();
    memUpdate(prevDisasmAddress);
}

void MainWindow::updateTIOSView() {
    calc_var_t var;
    QString formattedLine;
    QString calcData,calcData2;
    QString opType;
    uint8_t gotData[11];
    uint8_t index;

    ui->opView->clear();
    ui->vatView->clear();

    for(uint32_t i = 0xD005F8; i<0xD005F8+11*6; i+=11) {
        calcData.clear();
        opType.clear();
        index = 0;
        for(uint32_t j = i; j < i+11; j++) {
            gotData[index] = mem_peek_byte(j);
            calcData += int2hex(gotData[index], 2)+" ";
            index++;
        }
        if (*gotData < 0x40) {
            opType = QString(calc_var_type_names[*gotData]);
        }

        formattedLine = QString("<pre><b><font color='#444'>%1</font></b><font color='darkblue'>    %2    </font>%3 <font color='green'>%4</font></pre>")
                                       .arg(int2hex(i, 6), "OP"+QString::number(((i-0xD005F8)/11)+1), calcData, opType);

        ui->opView->appendHtml(formattedLine);
    }

    vat_search_init(&var);
    while (vat_search_next(&var)) {
        uint8_t j;
        calcData.clear();
        calcData2.clear();
        for(j = 0; j < var.namelen; j++) {
            calcData += int2hex(var.name[j], 2)+" ";
        }
        for(; j < 8; j++) {
            calcData2 += "00 ";
        }
        formattedLine = QString("<pre><b><font color='#444'>%1</font></b>  <font color='darkblue'>%2</font>  <font color='green'>%3</font>  %4<font color='gray'>%5</font><font color='green'> %6</font></pre>")
                                        .arg(int2hex(var.address,6), int2hex(var.vat,6), int2hex(var.size,4), calcData, calcData2, calc_var_type_names[var.type]);
        ui->vatView->appendHtml(formattedLine);
    }
    ui->vatView->moveCursor(QTextCursor::Start);
}

void MainWindow::updateDisassembly(int tab) {
    ui->disassemblyView->verticalScrollBar()->blockSignals(true);
    if (tab == 0) {
        updateDisasmView(prevDisasmAddress, true);
    } else {
        ui->portView->clearSelection();
        ui->breakpointView->clearSelection();
        ui->watchpointView->clearSelection();
        prevDisasmAddress = ui->disassemblyView->getSelectedAddress().toUInt(nullptr,16);
    }
}

void MainWindow::updateDisasmView(const int sentBase, const bool newPane) {
    addressPane = sentBase;
    fromPane = newPane;
    disasmOffsetSet = false;
    disasm.adl = ui->checkADL->isChecked();
    disasm.base_address = -1;
    disasm.new_address = addressPane - ((newPane) ? 0x40 : 0);
    if (disasm.new_address < 0) { disasm.new_address = 0; }
    int32_t last_address = disasm.new_address + 0x120;
    if (last_address > 0xFFFFFF) { last_address = 0xFFFFFF; }

    ui->disassemblyView->cursorState(false);
    ui->disassemblyView->blockSignals(true);
    ui->disassemblyView->clear();
    ui->disassemblyView->clearAllHighlights();

    while (disasm.new_address < last_address) {
        drawNextDisassembleLine();
    }

    ui->disassemblyView->blockSignals(false);
    ui->disassemblyView->verticalScrollBar()->blockSignals(false);

    ui->disassemblyView->setTextCursor(disasmOffset);

    ui->disassemblyView->cursorState(true);
    ui->disassemblyView->updateAllHighlights();
    ui->disassemblyView->centerCursor();
}

void MainWindow::addPort() {
    uint8_t read;
    uint16_t port;

    const int currentRow = ui->portView->rowCount();

    if (currPortAddress.isEmpty()) {
        currPortAddress = "0000";
    }

    std::string s = currPortAddress.toUpper().toStdString();
    if (s.find_first_not_of("0123456789ABCDEF") != std::string::npos) {
        return;
    }

    /* Mark the port as read active */
    port = static_cast<uint16_t>(hex2int(QString::fromStdString(s)));
    read = static_cast<uint8_t>(port_peek_byte(port));

    QString portString = int2hex(port,4);

    /* Return if port is already set */
    for (int i=0; i<currentRow; i++) {
        if (ui->portView->item(i, 0)->text() == portString) {
            if (portString != "0000") {
                return;
            }
        }
    }

    ui->portView->setRowCount(currentRow + 1);
    ui->portView->setUpdatesEnabled(false);
    ui->portView->blockSignals(true);

    QTableWidgetItem *port_range = new QTableWidgetItem(portString);
    QTableWidgetItem *port_data = new QTableWidgetItem(int2hex(read, 2));
    QTableWidgetItem *port_rBreak = new QTableWidgetItem();
    QTableWidgetItem *port_wBreak = new QTableWidgetItem();
    QTableWidgetItem *port_freeze = new QTableWidgetItem();

    port_rBreak->setFlags(port_rBreak->flags() & ~Qt::ItemIsEditable);
    port_wBreak->setFlags(port_wBreak->flags() & ~Qt::ItemIsEditable);
    port_freeze->setFlags(port_freeze->flags() & ~Qt::ItemIsEditable);

    port_rBreak->setCheckState(Qt::Unchecked);
    port_wBreak->setCheckState(Qt::Unchecked);
    port_freeze->setCheckState(Qt::Unchecked);

    ui->portView->setItem(currentRow, 0, port_range);
    ui->portView->setItem(currentRow, 1, port_data);
    ui->portView->setItem(currentRow, 2, port_rBreak);
    ui->portView->setItem(currentRow, 3, port_wBreak);
    ui->portView->setItem(currentRow, 4, port_freeze);

    ui->portView->selectRow(currentRow);
    ui->portView->setUpdatesEnabled(true);
    prevPortAddress = port;
    currPortAddress.clear();
    ui->portView->blockSignals(false);
}

void MainWindow::changePortValues(QTableWidgetItem *item) {
    auto row = item->row();
    auto col = item->column();

    if (col > 1) {
        uint16_t port = static_cast<uint16_t>(hex2int(ui->portView->item(row, 0)->text()));
        unsigned int value = DBG_NO_HANDLE;

        if (col == 2) { // Break on read
            value = DBG_PORT_READ;
        }
        if (col == 3) { // Break on write
            value = DBG_PORT_WRITE;
        }
        if (col == 4) { // Freeze
            value = DBG_PORT_FREEZE;
        }
        debug_pmonitor_set(port, value, item->checkState() == Qt::Checked);
    } else if (col == 0) {
        std::string s = item->text().toUpper().toStdString();
        if (s.find_first_not_of("0123456789ABCDEF") != std::string::npos || s.empty()) {
            item->setText(int2hex(prevPortAddress, 4));
            return;
        }

        uint16_t port = static_cast<uint16_t>(hex2int(QString::fromStdString(s)));
        QString portString = int2hex(port, 4);

        ui->portView->blockSignals(true);
        /* Return if port is already set */
        for (int i=0; i<ui->portView->rowCount(); i++) {
            if (ui->portView->item(i, 0)->text() == portString && i != row) {
                item->setText(int2hex(prevPortAddress, 4));
                ui->portView->blockSignals(false);
                return;
            }
        }

        debug_pmonitor_remove(prevPortAddress);

        unsigned int value = ((ui->portView->item(row, 2)->checkState() == Qt::Checked) ? DBG_PORT_READ : DBG_NO_HANDLE)  |
                             ((ui->portView->item(row, 3)->checkState() == Qt::Checked) ? DBG_PORT_WRITE : DBG_NO_HANDLE) |
                             ((ui->portView->item(row, 4)->checkState() == Qt::Checked) ? DBG_PORT_FREEZE : DBG_NO_HANDLE);

        debug_pmonitor_set(port, value, true);
        item->setText(portString);
        ui->portView->item(row, 1)->setText(int2hex(port_peek_byte(port), 2));
    } else if (col == 1) {
        uint8_t pdata = static_cast<uint8_t>(hex2int(item->text()));
        uint16_t port = static_cast<uint16_t>(hex2int(ui->portView->item(row, 0)->text()));

        port_poke_byte(port, pdata);

        item->setText(int2hex(port_peek_byte(port), 2));
    }
    ui->portView->blockSignals(false);
}

void MainWindow::removePort() {
    if (!ui->portView->rowCount() || !ui->portView->selectionModel()->isSelected(ui->portView->currentIndex())) {
        return;
    }

    const int currentRow = ui->portView->currentRow();
    uint16_t port = static_cast<uint16_t>(hex2int(ui->portView->item(currentRow, 0)->text()));

    debug_pmonitor_remove(port);
    ui->portView->removeRow(currentRow);
}

void MainWindow::setPreviousPortValues(QTableWidgetItem *curr_item) {
    if (curr_item->text().isEmpty()) {
        return;
    }
    prevPortAddress = static_cast<uint16_t>(hex2int(ui->portView->item(curr_item->row(), 0)->text()));
}

void MainWindow::setPreviousBreakpointAddress(QTableWidgetItem *curr_item) {
    if (curr_item->text().isEmpty()) {
        return;
    }
    prevBreakpointAddress = static_cast<uint32_t>(hex2int(ui->breakpointView->item(curr_item->row(), 0)->text()));
}

void MainWindow::setPreviousWatchpointAddress(QTableWidgetItem *curr_item) {
    if (curr_item->text().isEmpty()) {
        return;
    }
    prevWatchpointAddress = static_cast<uint32_t>(hex2int(ui->watchpointView->item(curr_item->row(), 0)->text()));
}

void MainWindow::changeBreakpointAddress(QTableWidgetItem *item) {
    auto row = item->row();
    auto col = item->column();
    QString addressString;
    uint32_t address;

    if (col > 0) {
        address = static_cast<uint32_t>(hex2int(ui->breakpointView->item(row, 0)->text()));
        unsigned int value = DBG_NO_HANDLE;

        if (col == 1) { // Break on execution
            value = DBG_EXEC_BREAKPOINT;
        }
        debug_breakwatch(address, value, item->checkState() == Qt::Checked);
    } else {
        std::string s = item->text().toUpper().toStdString();
        if (s.find_first_not_of("0123456789ABCDEF") != std::string::npos || s.empty()) {
            item->setText(int2hex(prevBreakpointAddress, 6));
            return;
        }

        address = static_cast<uint32_t>(hex2int(QString::fromStdString(s)));
        addressString = int2hex(address,6);

        ui->breakpointView->blockSignals(true);
        /* Return if address is already set */
        for (int i=0; i<ui->breakpointView->rowCount(); i++) {
            if (ui->breakpointView->item(i, 0)->text() == addressString && i != row) {
                item->setText(int2hex(prevBreakpointAddress, 6));
                ui->breakpointView->blockSignals(false);
                return;
            }
        }

        unsigned int value = ((ui->breakpointView->item(row, 1)->checkState() == Qt::Checked) ? DBG_EXEC_BREAKPOINT : DBG_NO_HANDLE);

        debug_breakwatch(prevBreakpointAddress, DBG_EXEC_BREAKPOINT, false);
        item->setText(addressString);
        debug_breakwatch(address, value, true);
        ui->breakpointView->blockSignals(false);
    }
}

void MainWindow::changeWatchpointAddress(QTableWidgetItem *item) {
    auto row = item->row();
    auto col = item->column();
    QString newString;
    uint32_t address;

    ui->watchpointView->blockSignals(true);

    if (col == 2) { // update the data located at this address
        uint8_t i,wLength = ui->watchpointView->item(row, 1)->text().toUInt();
        uint32_t wData;

        address = static_cast<uint32_t>(ui->watchpointView->item(row, 0)->text().toUInt());

        std::string s = item->text().toUpper().toStdString();
        if (s.find_first_not_of("0123456789ABCDEF") != std::string::npos || s.empty()) {
            item->setText(int2hex(0, wLength << 1));
            ui->watchpointView->blockSignals(false);
            return;
        }

        wData = static_cast<uint32_t>(item->text().toUpper().toUInt(nullptr, 16));
        newString = int2hex(wData, wLength << 1);

        item->setText(newString);

        for(i=0; i<wLength; i++) {
            mem_poke_byte(address+i, (wData >> ((i << 3))&0xFF));
        }

        ramUpdate();
        flashUpdate();
        memUpdate(address);
    } else if (col == 1) { // length of data we wish to read
        unsigned int data_length = item->text().toUInt();
        if (data_length > 4) {
            data_length = 4;
        } else if (data_length < 1) {
            data_length = 1;
        }
        item->setText(QString::number(data_length));
        updateWatchpointData(row);
    } else if (col > 2) {
        address = static_cast<uint32_t>(hex2int(ui->watchpointView->item(row, 0)->text()));
        unsigned int value = DBG_NO_HANDLE;

        if (col == 3) { // Break on read
            value = DBG_READ_WATCHPOINT;
        } else
        if (col == 4) { // Break on write
            value = DBG_WRITE_WATCHPOINT;
        }
        debug_breakwatch(address, value, item->checkState() == Qt::Checked);
    } else {
        std::string s = item->text().toUpper().toStdString();
        if (s.find_first_not_of("0123456789ABCDEF") != std::string::npos || s.empty()) {
            item->setText(int2hex(prevWatchpointAddress, 6));
            ui->watchpointView->blockSignals(false);
            return;
        }

        address = static_cast<uint32_t>(hex2int(item->text().toUpper()));
        newString = int2hex(address,6);

        /* Return if address is already set */
        for (int i=0; i<ui->watchpointView->rowCount(); i++) {
            if (ui->watchpointView->item(i, 0)->text() == newString && i != row) {
                item->setText(int2hex(prevWatchpointAddress, 6));
                ui->watchpointView->blockSignals(false);
                return;
            }
        }

        unsigned int value = ((ui->watchpointView->item(row, 3)->checkState() == Qt::Checked) ? DBG_READ_WATCHPOINT : DBG_NO_HANDLE)|
                             ((ui->watchpointView->item(row, 4)->checkState() == Qt::Checked) ? DBG_WRITE_WATCHPOINT : DBG_NO_HANDLE);

        debug_breakwatch(prevWatchpointAddress, DBG_WRITE_WATCHPOINT | DBG_READ_WATCHPOINT, false);
        item->setText(newString);
        debug_breakwatch(address, value, true);
        updateWatchpointData(row);
    }
    ui->watchpointView->blockSignals(false);
}

bool MainWindow::addBreakpoint() {
    const int currentRow = ui->breakpointView->rowCount();

    currAddress %= 0xFFFFFF;
    currAddressString = int2hex(currAddress, 6).toUpper();

    if (currAddressString.toStdString().find_first_not_of("0123456789ABCDEF") != std::string::npos) {
        return false;
    }

    /* Return if address is already set */
    for (int i=0; i<currentRow; ++i) {
        if (ui->breakpointView->item(i, 0)->text() == currAddressString) {
            if(currAddressString != "000000") {
                ui->breakpointView->selectRow(i);
                return false;
            }
        }
    }

    ui->breakpointView->setUpdatesEnabled(false);
    ui->breakpointView->blockSignals(true);

    ui->breakpointView->setRowCount(currentRow + 1);

    QTableWidgetItem *iaddress = new QTableWidgetItem(currAddressString);
    QTableWidgetItem *eBreak = new QTableWidgetItem();

    eBreak->setCheckState(Qt::Checked);
    eBreak->setFlags(eBreak->flags() & ~Qt::ItemIsEditable);

    ui->breakpointView->setItem(currentRow, 0, iaddress);
    ui->breakpointView->setItem(currentRow, 1, eBreak);

    ui->breakpointView->selectRow(currentRow);
    ui->breakpointView->setUpdatesEnabled(true);

    debug_breakwatch(currAddress, DBG_EXEC_BREAKPOINT, true);

    prevBreakpointAddress = currAddress;
    currAddressString.clear();
    ui->breakpointView->blockSignals(false);
    return true;
}

bool MainWindow::removeBreakpoint() {
    if(!ui->breakpointView->rowCount() || !ui->breakpointView->selectionModel()->isSelected(ui->breakpointView->currentIndex())) {
        return false;
    }

    const int currentRow = ui->breakpointView->currentRow();
    uint32_t address = static_cast<uint32_t>(hex2int(ui->breakpointView->item(currentRow, 0)->text()));

    debug_breakwatch(address, DBG_EXEC_BREAKPOINT, false);

    ui->breakpointView->removeRow(currentRow);
    return true;
}

bool MainWindow::addWatchpoint() {
    const int currentRow = ui->watchpointView->rowCount();

    currAddress %= 0xFFFFFF;
    currAddressString = int2hex(currAddress, 6).toUpper();

    if (watchLength.isEmpty()) {
        watchLength = "1";
    }

    if (watchpointType == DBG_NO_HANDLE) {
        watchpointType = DBG_WRITE_WATCHPOINT | DBG_READ_WATCHPOINT;
    }

    if (watchpointType == DBG_EMPTY_WATCHPOINT) {
        watchpointType = DBG_NO_HANDLE;
    }

    if (currAddressString.toStdString().find_first_not_of("0123456789ABCDEF") != std::string::npos) {
        return false;
    }

    /* Return if address is already set */
    for (int i=0; i<currentRow; ++i) {
        if (ui->watchpointView->item(i, 0)->text() == currAddressString) {
            if(currAddressString != "000000") {
                ui->watchpointView->selectRow(i);
                return false;
            }
        }
    }

    ui->watchpointView->setUpdatesEnabled(false);
    ui->watchpointView->blockSignals(true);

    QTableWidgetItem *iaddress = new QTableWidgetItem(currAddressString);
    QTableWidgetItem *length = new QTableWidgetItem(watchLength);
    QTableWidgetItem *dWatch = new QTableWidgetItem();
    QTableWidgetItem *rWatch = new QTableWidgetItem();
    QTableWidgetItem *wWatch = new QTableWidgetItem();

    rWatch->setCheckState((watchpointType & DBG_READ_WATCHPOINT) ? Qt::Checked : Qt::Unchecked);
    wWatch->setCheckState((watchpointType & DBG_WRITE_WATCHPOINT) ? Qt::Checked : Qt::Unchecked);

    wWatch->setFlags(wWatch->flags() & ~Qt::ItemIsEditable);
    rWatch->setFlags(rWatch->flags() & ~Qt::ItemIsEditable);

    ui->watchpointView->setRowCount(currentRow + 1);

    ui->watchpointView->setItem(currentRow, 0, iaddress);
    ui->watchpointView->setItem(currentRow, 1, length);
    ui->watchpointView->setItem(currentRow, 2, dWatch);
    ui->watchpointView->setItem(currentRow, 3, rWatch);
    ui->watchpointView->setItem(currentRow, 4, wWatch);

    updateWatchpointData(currentRow);

    ui->watchpointView->selectRow(currentRow);
    ui->watchpointView->setUpdatesEnabled(true);

    debug_breakwatch(currAddress, DBG_WRITE_WATCHPOINT | DBG_READ_WATCHPOINT, true);

    prevWatchpointAddress = currAddress;

    /* reset these for the next one */
    currAddressString.clear();
    watchLength.clear();
    watchpointType = DBG_NO_HANDLE;

    ui->watchpointView->blockSignals(false);
    return true;
}

bool MainWindow::removeWatchpoint() {
    if(!ui->watchpointView->rowCount() || !ui->watchpointView->selectionModel()->isSelected(ui->watchpointView->currentIndex())) {
        return false;
    }

    const int currentRow = ui->watchpointView->currentRow();
    uint32_t address = static_cast<uint32_t>(hex2int(ui->watchpointView->item(currentRow, 0)->text()));

    debug_breakwatch(address, DBG_READ_WATCHPOINT | DBG_WRITE_WATCHPOINT, false);

    ui->watchpointView->removeRow(currentRow);
    return true;
}

void MainWindow::removeWatchpointAddress(QString address) {
    for (int i=0; i<ui->watchpointView->rowCount(); i++) {
        if (ui->watchpointView->item(i, 0)->text() == address) {
            ui->watchpointView->setCurrentCell(i,0);
            removeWatchpoint();
            break;
        }
    }
}

void MainWindow::removeBreakpointAddress(QString address) {
    for (int i=0; i<ui->breakpointView->rowCount(); i++) {
        if (ui->breakpointView->item(i, 0)->text() == address) {
            ui->breakpointView->setCurrentCell(i,0);
            removeBreakpoint();
            break;
        }
    }
}

void MainWindow::executeDebugCommand(uint32_t debugAddress, uint8_t command) {

    if (debugAddress == 0xFF) {
        uint8_t wLength;
        int tmp;
        switch (command) {
            case 1: // abort() routine hit
                consoleStr("[CEmu] Program Aborted.\n");
                raiseDebugger();
                return;
            case 2: // debugger() routine hit
                consoleStr("[CEmu] Program Entered Debugger.\n");
                raiseDebugger();
                return;
            case 3: // set a breakpoint with the value in DE
                currAddress = cpu.registers.DE;
                addBreakpoint();
                inDebugger = false; // continue emulation; we don't need to raise the debugger
                return;
            case 4: // remove a breakpoint with the value in DE
                currAddress = cpu.registers.DE;
                removeBreakpointAddress(int2hex(currAddress, 6));
                inDebugger = false;
                return;
            case 5: // set a read watchpoint with the value in DE; length in C
                wLength = (cpu.registers.bc.l > 4) ? 4 : cpu.registers.bc.l;
                currAddress = cpu.registers.DE;
                watchLength = QString::number(wLength);
                watchpointType = DBG_READ_WATCHPOINT;
                addWatchpoint();
                inDebugger = false;
                return;
            case 6: // set a write watchpoint with the value in DE; length in C
                wLength = (cpu.registers.bc.l > 4) ? 4 : cpu.registers.bc.l;
                currAddress = cpu.registers.DE;
                watchLength = QString::number(wLength);
                watchpointType = DBG_WRITE_WATCHPOINT;
                removeWatchpointAddress(int2hex(currAddress, 6));
                addWatchpoint();
                inDebugger = false;
                return;
            case 7: // set a read/write watchpoint with the value in DE; length in C
                wLength = (cpu.registers.bc.l > 4) ? 4 : cpu.registers.bc.l;
                currAddress = cpu.registers.DE;
                watchLength = QString::number(wLength);
                watchpointType = DBG_WRITE_WATCHPOINT | DBG_READ_WATCHPOINT;
                removeWatchpointAddress(int2hex(currAddress, 6));
                addWatchpoint();
                inDebugger = false;
                return;
            case 8: // we need to remove a watchpoint with the value in DE
                removeWatchpointAddress(int2hex(cpu.registers.DE, 6));
                inDebugger = false;
                return;
            case 9: // we need to remove all breakpoints
                tmp = ui->breakpointView->rowCount();
                for (int i=0; i<tmp; i++) {
                    ui->breakpointView->selectRow(0);
                    removeBreakpoint();
                }
                inDebugger = false;
                return;
            case 10: // we need to remove all watchpoints
                tmp = ui->watchpointView->rowCount();
                for (int i=0; i<tmp; i++) {
                    ui->watchpointView->selectRow(0);
                    removeWatchpoint();
                }
                inDebugger = false;
                return;
            case 11: // set an empty watchpoint with the value in DE; length in C
                wLength = (cpu.registers.bc.l > 4) ? 4 : cpu.registers.bc.l;
                currAddress = cpu.registers.DE;
                watchLength = QString::number(wLength);
                watchpointType = DBG_EMPTY_WATCHPOINT;
                removeWatchpointAddress(int2hex(currAddress, 6));
                addWatchpoint();
                inDebugger = false;
                return;
            default:
                break;
        }
    }
    consoleStr("[CEmu] Unknown debug Command: 0x"+QString::number(command,16).rightJustified(2,'0')+",0x"+QString::number(debugAddress+0xFFFF00,16)+"\n");
    inDebugger = false;
}

void MainWindow::processDebugCommand(int reason, uint32_t input) {
    int row = 0;

    /* This means the program is trying to send us a debug command. Let's see what we can do with that information. */
    if (reason > NUM_DBG_COMMANDS) {
       executeDebugCommand(static_cast<uint32_t>(reason-DBG_PORT_RANGE), static_cast<uint8_t>(input));
       return;
    }

    raiseDebugger();

    // We hit a normal breakpoint; raise the correct entry in the port monitor table
    if (reason == HIT_EXEC_BREAKPOINT) {
        // find the correct entry
        while( static_cast<uint32_t>(hex2int(ui->breakpointView->item(row++, 0)->text())) != input );
        row--;

        ui->breakChangeLabel->setText(ui->breakpointView->item(row, 0)->text());
        ui->breakTypeLabel->setText("Executed");
        ui->breakpointView->selectRow(row);
    }

    else if (reason == HIT_READ_BREAKPOINT || reason == HIT_WRITE_BREAKPOINT) {
        // find the correct entry
        while( static_cast<uint32_t>(hex2int(ui->watchpointView->item(row++, 0)->text())) != input );
        row--;

        ui->watchChangeLabel->setText(ui->watchpointView->item(row, 0)->text());
        ui->watchTypeLabel->setText((reason == HIT_READ_BREAKPOINT) ? "Read" : "Write");
        ui->watchpointView->selectRow(row);
        memUpdate(input);
    }

    // We hit a port read or write; raise the correct entry in the port monitor table
    else if (reason == HIT_PORT_READ_BREAKPOINT || reason == HIT_PORT_WRITE_BREAKPOINT) {
        while( static_cast<uint32_t>(hex2int(ui->portView->item(row++, 0)->text())) != input );
        row--;

        ui->portChangeLabel->setText(ui->portView->item(row, 0)->text());
        ui->portTypeLabel->setText((reason == HIT_PORT_READ_BREAKPOINT) ? "Read" : "Write");
        ui->portView->selectRow(row);
    }
}

void MainWindow::updatePortData(int currentRow) {
    uint16_t port = static_cast<uint16_t>(hex2int(ui->portView->item(currentRow, 0)->text()));
    uint8_t read = static_cast<uint8_t>(port_peek_byte(port));

    ui->portView->item(currentRow, 1)->setText(int2hex(read,2));
}

void MainWindow::updateWatchpointData(int currentRow) {
    uint8_t i,length = ui->watchpointView->item(currentRow, 1)->text().toUInt();
    uint32_t address = static_cast<uint32_t>(hex2int(ui->watchpointView->item(currentRow, 0)->text()));
    uint32_t read = 0;

    for(i=0; i<length; i++) {
        read |= mem_peek_byte(address+i) << (i << 3);
    }

    ui->watchpointView->item(currentRow, 2)->setText(int2hex(read, length << 1));
}

void MainWindow::reloadROM() {
    if (inReceivingMode) {
        refreshVariableList();
    }
    if(debuggerOn) {
        changeDebuggerState();
    }

    QFile(emu.imagePath.c_str()).remove();

    if (emu.stop()) {
        emu.start();
        qDebug("Reload Successful.");
    } else {
        qDebug("Reload Failed.");
    }
}

void MainWindow::updateStackView() {
    QString formattedLine;

    ui->stackView->blockSignals(true);
    ui->stackView->clear();

    if (cpu.ADL) {
        for(int i=0; i<60; i+=3) {
            formattedLine = QString("<pre><b><font color='#444'>%1</font></b> %2</pre>")
                                    .arg(int2hex(cpu.registers.SPL+i, 6),
                                         int2hex(mem_peek_word(cpu.registers.SPL+i, 1), 6));
            ui->stackView->appendHtml(formattedLine);
        }
    } else {
        for(int i=0; i<40; i+=2) {
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
            disasmHighlight.hit_read_breakpoint = false;
            disasmHighlight.hit_write_breakpoint = false;
            disasmHighlight.hit_exec_breakpoint = false;
            disasmHighlight.hit_run_breakpoint = false;
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
                                   .arg(((disasmHighlight.hit_read_breakpoint == true)  ? "&#9679;" : " "),
                                        ((disasmHighlight.hit_write_breakpoint == true) ? "&#9679;" : " "),
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

    if (disasmHighlight.hit_run_breakpoint == true) {
        ui->disassemblyView->addHighlight(QColor(Qt::blue).lighter(160));
    }
    if (disasmHighlight.hit_pc == true) {
        ui->disassemblyView->addHighlight(QColor(Qt::red).lighter(160));
    }
}

void MainWindow::disasmContextMenu(const QPoint& posa) {
    QString set_pc = "Set PC";
    QString toggle_break = "Toggle Breakpoint";
    QString toggle_watch = "Toggle Watchpoint";
    QString run_until = "Toggle Run Until";
    QString goto_mem = "Goto Memory View";
    ui->disassemblyView->setTextCursor(ui->disassemblyView->cursorForPosition(posa));
    QPoint globalPos = ui->disassemblyView->mapToGlobal(posa);

    QMenu contextMenu;
    contextMenu.addAction(set_pc);
    contextMenu.addAction(toggle_break);
    contextMenu.addAction(toggle_watch);
    contextMenu.addAction(run_until);
    contextMenu.addAction(goto_mem);

    QAction* selectedItem = contextMenu.exec(globalPos);
    if (selectedItem) {
        if (selectedItem->text() == set_pc) {
            ui->pcregView->setText(ui->disassemblyView->getSelectedAddress());
            uint32_t address = static_cast<uint32_t>(hex2int(ui->pcregView->text()));
            debug_set_pc_address(address);
            ui->disassemblyView->verticalScrollBar()->blockSignals(true);
            updateDisasmView(cpu.registers.PC, true);
        } else if (selectedItem->text() == toggle_break) {
            setBreakpointAddress();
        } else if (selectedItem->text() == toggle_watch) {
            setWatchpointAddress();
        } else if (selectedItem->text() == run_until) {
            uint32_t address = static_cast<uint32_t>(hex2int(ui->disassemblyView->getSelectedAddress()));
            debug_toggle_run_until(address);
            ui->disassemblyView->verticalScrollBar()->blockSignals(true);
            updateDisasmView(address, true);
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
            if (!debuggerOn) {
                changeDebuggerState();
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
    LCDPopout *p = new LCDPopout();
    p->show();
}

void MainWindow::stepInPressed() {
    if(!inDebugger) {
        return;
    }

    disconnect(stepInShortcut, &QShortcut::activated, this, &MainWindow::stepInPressed);

    debuggerOn = false;
    updateDebuggerChanges();
    emit setDebugStepInMode();
}

void MainWindow::stepOverPressed() {
    if(!inDebugger) {
        return;
    }

    disconnect(stepOverShortcut, &QShortcut::activated, this, &MainWindow::stepOverPressed);

    debuggerOn = false;
    updateDebuggerChanges();
    emit setDebugStepOverMode();
}

void MainWindow::stepNextPressed() {
    if(!inDebugger) {
        return;
    }

    disconnect(stepNextShortcut, &QShortcut::activated, this, &MainWindow::stepNextPressed);

    debuggerOn = false;
    updateDebuggerChanges();
    emit setDebugStepNextMode();
}

void MainWindow::stepOutPressed() {
    if(!inDebugger) {
        return;
    }

    disconnect(stepOutShortcut, &QShortcut::activated, this, &MainWindow::stepOutPressed);

    debuggerOn = false;
    updateDebuggerChanges();
    emit setDebugStepOutMode();
}

void MainWindow::disableDebugger() {
    setDebuggerState(false);
}

void MainWindow::setBreakpointAddress() {
    currAddress = static_cast<uint32_t>(hex2int(ui->disassemblyView->getSelectedAddress()));

    QTextCursor c = ui->disassemblyView->textCursor();
    c.setCharFormat(ui->disassemblyView->currentCharFormat());

    if (!addBreakpoint()) {
        removeBreakpoint();
    }

    disasm.base_address = currAddress;
    disasmHighlight.hit_exec_breakpoint = false;
    disassembleInstruction();

    c.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    c.setPosition(c.position()+9, QTextCursor::MoveAnchor);
    c.deleteChar();

    // Add the red dot
    if (disasmHighlight.hit_exec_breakpoint) {
        c.insertHtml("<font color='#FFA3A3'>&#9679;</font>");
    } else {
        c.insertText(" ");
    }
}

void MainWindow::setWatchpointAddress() {
    currAddress = static_cast<uint32_t>(hex2int(ui->disassemblyView->getSelectedAddress()));

    QTextCursor c = ui->disassemblyView->textCursor();
    c.setCharFormat(ui->disassemblyView->currentCharFormat());

    if (!addWatchpoint()) {
        removeWatchpoint();
    }

    disasm.base_address = currAddress;
    disasmHighlight.hit_read_breakpoint = false;
    disasmHighlight.hit_write_breakpoint = false;
    disassembleInstruction();

    c.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    c.setPosition(c.position()+7, QTextCursor::MoveAnchor);
    c.deleteChar();

    // Add the green dot
    if (disasmHighlight.hit_read_breakpoint) {
        c.insertHtml("<font color='#A3FFA3'>&#9679;</font>");
    } else {
        c.insertText(" ");
    }

    c.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    c.setPosition(c.position()+8, QTextCursor::MoveAnchor);
    c.deleteChar();

    // Add the blue dot
    if (disasmHighlight.hit_write_breakpoint) {
        c.insertHtml("<font color='#A3A3FF'>&#9679;</font>");
    } else {
        c.insertText(" ");
    }
}

QString MainWindow::getAddressString(bool &ok, QString String) {
    QString address = QInputDialog::getText(this, tr("Goto Address"),
                                         tr("Input Address (In Hexadecimal):"), QLineEdit::Normal,
                                         String, &ok).toUpper();

    if (!ok || (address.toStdString().find_first_not_of("0123456789ABCDEF") != std::string::npos) || (address.length() > 6) || !address.length()) {
        ok = false;
        return QStringLiteral("");
    }

    return address;
}

void MainWindow::gotoPressed() {
    bool ok;
    QString address = getAddressString(ok, ui->disassemblyView->getSelectedAddress());

    if (!ok) {
        return;
    }

    updateDisasmView(hex2int(address), false);
}

void MainWindow::changeBatteryCharging(bool checked) {
    control.batteryCharging = checked;
}

void MainWindow::changeBatteryStatus(int value) {
    control.setBatteryStatus = static_cast<uint8_t>(value);
    ui->sliderBattery->setValue(value);
    ui->labelBattery->setText(QString::number(value * 20) + "%");
}

/* ================================================ */
/* Hex Editor Things                                */
/* ================================================ */

void MainWindow::flashUpdate() {
    ui->flashEdit->setFocus();
    int line = ui->flashEdit->getLine();
    ui->flashEdit->setData(QByteArray::fromRawData((char*)mem.flash.block, 0x400000));
    ui->flashEdit->setLine(line);
}

void MainWindow::ramUpdate() {
    ui->ramEdit->setFocus();
    int line = ui->ramEdit->getLine();
    ui->ramEdit->setData(QByteArray::fromRawData((char*)mem.ram.block, 0x65800));
    ui->ramEdit->setAddressOffset(0xD00000);
    ui->ramEdit->setLine(line);
}

void MainWindow::memUpdate(uint32_t addressBegin) {
    ui->memEdit->setFocus();
    QByteArray mem_data;

    bool locked = ui->checkLockPosition->isChecked();
    int32_t start, line = 0;

    if (locked) {
        start = static_cast<int32_t>(ui->memEdit->addressOffset());
        line = ui->memEdit->getLine();
    } else {
        start = static_cast<int32_t>(addressBegin) - 0x1000;
    }

    if (start < 0) { start = 0; }
    int32_t end = start+0x2000;
    if (end > 0xFFFFFF) { end = 0xFFFFFF; }

    memSize = end-start;

    for (int32_t i=start; i<end; i++) {
        mem_data.append(mem_peek_byte(i));
    }

    ui->memEdit->setData(mem_data);
    ui->memEdit->setAddressOffset(start);

    if (locked) {
        ui->memEdit->setLine(line);
    } else {
        ui->memEdit->setCursorPosition((addressBegin-start)<<1);
        ui->memEdit->ensureVisible();
    }
}

void MainWindow::searchEdit(QHexEdit *editor) {
    SearchWidget search;
    search.setSearchString(searchingString);
    search.setInputMode(hexSearch);

    search.show();
    search.exec();

    hexSearch = search.getInputMode();
    searchingString = search.getSearchString();

    if(!search.getStatus()) {
        return;
    }

    QString searchString;
    if(hexSearch == true) {
        searchString = searchingString;
    } else {
        searchString = QString::fromStdString(searchingString.toLatin1().toHex().toStdString());
    }

    editor->setFocus();
    std::string s = searchString.toUpper().toStdString();
    if(searchString.isEmpty()) {
        return;
    }
    if((searchString.length() & 1) || s.find_first_not_of("0123456789ABCDEF") != std::string::npos) {
        QMessageBox::warning(this,"Error", "Error when reading input string");
        return;
    }

    QByteArray string_int;
    for (int i=0; i<searchString.length(); i+=2) {
        QString a = searchString.at(i);
        a.append(searchString.at(i+1));
        string_int.append(hex2int(a));
    }
    if(editor->indexOf(string_int, editor->cursorPosition()) == -1) {
        QMessageBox::warning(this,"Not Found","Hex string not found.");
    }
}

void MainWindow::flashSearchPressed() {
    searchEdit(ui->flashEdit);
}

void MainWindow::flashGotoPressed() {
    bool ok;
    QString address = getAddressString(ok, "");

    ui->flashEdit->setFocus();
    if (!ok) {
        return;
    }
    int int_address = hex2int(address);
    if (int_address > 0x3FFFFF) {
        return;
    }

    ui->flashEdit->setCursorPosition(int_address<<1);
    ui->flashEdit->ensureVisible();
}

void MainWindow::ramSearchPressed() {
    searchEdit(ui->ramEdit);
}

void MainWindow::ramGotoPressed() {
    bool ok;
    QString address = getAddressString(ok, "");

    ui->ramEdit->setFocus();
    if (!ok) {
        return;
    }
    int int_address = hex2int(address)-0xD00000;
    if (int_address > 0x657FF || address < 0) {
        return;
    }

    ui->ramEdit->setCursorPosition(int_address<<1);
    ui->ramEdit->ensureVisible();
}
void MainWindow::memSearchPressed() {
    searchEdit(ui->memEdit);
}

void MainWindow::memGoto(QString address) {
    ui->memEdit->setFocus();
    int int_address = hex2int(address);
    if (int_address > 0xFFFFFF || int_address < 0) {
        return;
    }

    QByteArray mem_data;
    int start = int_address-0x500;
    if (start < 0) { start = 0; }
    int end = start+0x1000;
    if (end > 0xFFFFFF) { end = 0xFFFFFF; }

    memSize = end-start;

    for (int i=start; i<end; i++) {
        mem_data.append(mem_peek_byte(i));
    }

    ui->memEdit->setData(mem_data);
    ui->memEdit->setAddressOffset(start);
    ui->memEdit->setCursorPosition((int_address-start)<<1);
    ui->memEdit->ensureVisible();
}

void MainWindow::memGotoPressed() {
    bool ok;
    QString address = getAddressString(ok, "");

    if (!ok) {
        return;
    }
    memGoto(address);
}

void MainWindow::syncHexView(int posa, QHexEdit *hex_view) {
    populateDebugWindow();
    updateDisasmView(addressPane, fromPane);
    hex_view->setFocus();
    hex_view->setCursorPosition(posa);
}

void MainWindow::flashSyncPressed() {
    qint64 posa = ui->flashEdit->cursorPosition();
    memcpy(mem.flash.block, reinterpret_cast<uint8_t*>(ui->flashEdit->data().data()), 0x400000);
    syncHexView(posa, ui->flashEdit);
}

void MainWindow::ramSyncPressed() {
    qint64 posa = ui->ramEdit->cursorPosition();
    memcpy(mem.ram.block, reinterpret_cast<uint8_t*>(ui->ramEdit->data().data()), 0x65800);
    syncHexView(posa, ui->ramEdit);
}

void MainWindow::memSyncPressed() {
    int start = ui->memEdit->addressOffset();
    qint64 posa = ui->memEdit->cursorPosition();

    for (int i = 0; i<memSize; i++) {
        mem_poke_byte(i+start, ui->memEdit->dataAt(i, 1).at(0));
    }

    syncHexView(posa, ui->memEdit);
}

void MainWindow::clearEquateFile() {
    // Reset the map
    currentEquateFile.clear();
    disasm.addressMap.clear();
    QMessageBox::warning(this, tr("Equates Cleared"), tr("Cleared disassembly equates."));
    updateDisasmView(ui->disassemblyView->getSelectedAddress().toInt(nullptr,16), true);
}

void MainWindow::refreshEquateFile() {
    // Reset the map
    if(fileExists(currentEquateFile.toStdString())) {
        disasm.addressMap.clear();
        addEquateFile(currentEquateFile);
        updateDisasmView(ui->disassemblyView->getSelectedAddress().toInt(nullptr,16), true);
    } else {
        QMessageBox::warning(this, tr("Error Opening"), tr("Couldn't open equates file."));
    }
}

void MainWindow::addEquateFileDialog() {
    QFileDialog dialog(this);
    int good;
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setDirectory(currentDir);

    QStringList extFilters;
    extFilters << tr("ASM equates file (*.inc)")
               << tr("Symbol Table File (*.lab)");
    dialog.setNameFilters(extFilters);

    good = dialog.exec();
    currentDir = dialog.directory();

    if (!good) { return; }

    std::string current;
    std::ifstream in;
    addEquateFile(dialog.selectedFiles().first());
}

void MainWindow::addEquateFile(QString fileName) {
    std::string current;
    std::ifstream in;
    currentEquateFile = fileName;
    in.open(fileName.toStdString());

    if (in.good()) {
        QRegularExpression equatesRegexp("^\\h*([^\\W\\d]\\w*)\\h*(?:=|\\h\\.?equ(?!\\d))\\h*(?|\\$([\\da-f]{4,})|(\\d[\\da-f]{3,})h)\\h*(?:;.*)?$",
                                         QRegularExpression::CaseInsensitiveOption);
        // Reset the map
        disasm.addressMap.clear();
        while (std::getline(in, current)) {
            QRegularExpressionMatch matches = equatesRegexp.match(QString::fromStdString(current));
            if (matches.hasMatch()) {
                uint32_t address = static_cast<uint32_t>(matches.capturedRef(2).toUInt(nullptr, 16));
                std::string &item = disasm.addressMap[address];
                if (item.empty()) {
                    item = matches.captured(1).toStdString();
                    uint8_t *ptr = phys_mem_ptr(address - 4, 9);
                    if (ptr && ptr[4] == 0xC3 && (ptr[0] == 0xC3 || ptr[8] == 0xC3)) { // jump table?
                        uint32_t address2  = ptr[5] | ptr[6] << 8 | ptr[7] << 16;
                        if (phys_mem_ptr(address2, 1)) {
                            std::string &item2 = disasm.addressMap[address2];
                            if (item2.empty()) {
                                item2 = "_" + item;
                            }
                        }
                    }
                }
            }
        }
        in.close();
        updateDisasmView(ui->disassemblyView->getSelectedAddress().toInt(nullptr,16), true);
    } else {
        QMessageBox messageBox;
        messageBox.critical(0, tr("Error"), tr("Couldn't open this file"));
        messageBox.setFixedSize(500,200);
    }
}

void MainWindow::scrollDisasmView(int value) {
    if (value >= ui->disassemblyView->verticalScrollBar()->maximum() && value > 0x100) {
        ui->disassemblyView->verticalScrollBar()->blockSignals(true);
        drawNextDisassembleLine();
        ui->disassemblyView->verticalScrollBar()->setValue(ui->disassemblyView->verticalScrollBar()->maximum()-1);
        ui->disassemblyView->verticalScrollBar()->blockSignals(false);
    }
}

void MainWindow::resetCalculator() {
    if (inReceivingMode) {
        refreshVariableList();
    }
    if(debuggerOn) {
        changeDebuggerState();
    }
    emit resetTriggered();
}
