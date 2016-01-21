/* Copyright (C) 2015  Fabian Vogt
 * Modified for the CE calculator by CEmu developers
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
#include <QtWidgets/QInputDialog>
#include <QtQuickWidgets/QQuickWidget>
#include <QtGui/QFont>
#include <QtGui/QPixmap>

#include <fstream>
#include <iostream>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "emuthread.h"
#include "qmlbridge.h"
#include "qtframebuffer.h"
#include "qtkeypadbridge.h"

#include "utils.h"
#include "capture/gif.h"
#include "../../core/schedule.h"
#include "../../core/debug/disasm.h"
#include "../../core/link.h"
#include "../../core/os/os.h"

static const constexpr int WindowStateVersion = 0;

MainWindow::MainWindow(QWidget *p) : QMainWindow(p), ui(new Ui::MainWindow) {
    // Setup the UI
    ui->setupUi(this);
    ui->statusBar->addWidget(&statusLabel);

    // Register QtKeypadBridge for the virtual keyboard functionality
    this->installEventFilter(&qt_keypad_bridge);
    detachedLCD.installEventFilter(&qt_keypad_bridge);
    // Same for all the tabs/docks (iterate over them instead of harcoding their names)
    for (const auto& tab : ui->tabWidget->children()[0]->children()) {
        tab->installEventFilter(&qt_keypad_bridge);
    }

    ui->keypadWidget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    ui->disassemblyView->setContextMenuPolicy(Qt::CustomContextMenu);

    // View
    detachedLCD.setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->actionDetached_LCD, &QAction::triggered, this, &MainWindow::popoutLCD);
    connect(&detachedLCD, &LCDWidget::closed, this, &MainWindow::popoutLCD);
    connect(&detachedLCD, &LCDWidget::lcdOpenRequested, this, &MainWindow::selectFiles);
    connect(ui->lcdWidget, &LCDWidget::lcdOpenRequested, this, &MainWindow::selectFiles);

    // Emulator -> GUI
    connect(&emu, &EmuThread::consoleStr, this, &MainWindow::consoleStr);

    // Console actions
    connect(ui->buttonConsoleclear, &QPushButton::clicked, this, &MainWindow::clearConsole);

    // Debugger
    connect(ui->buttonRun, &QPushButton::clicked, this, &MainWindow::changeDebuggerState);
    connect(this, &MainWindow::debuggerChangedState, &emu, &EmuThread::setDebugMode);
    connect(&emu, &EmuThread::debuggerEntered, this, &MainWindow::raiseDebugger, Qt::QueuedConnection);
    connect(&emu, &EmuThread::sendDebugCommand, this, &MainWindow::processDebugCommand, Qt::QueuedConnection);
    connect(ui->buttonAddPort, &QPushButton::clicked, this, &MainWindow::addPort);
    connect(ui->portRequest, &QLineEdit::returnPressed, this, &MainWindow::addPort);
    connect(ui->buttonDeletePort, &QPushButton::clicked, this, &MainWindow::deletePort);
    connect(ui->portView, &QTableWidget::itemChanged, this, &MainWindow::portMonitorCheckboxToggled);
    connect(ui->buttonAddBreakpoint, &QPushButton::clicked, this, &MainWindow::addBreakpoint);
    connect(ui->breakRequest, &QLineEdit::returnPressed, this, &MainWindow::addBreakpoint);
    connect(ui->buttonRemoveBreakpoint, &QPushButton::clicked, this, &MainWindow::deleteBreakpoint);
    connect(ui->breakpointView, &QTableWidget::itemChanged, this, &MainWindow::breakpointCheckboxToggled);
    connect(ui->actionReset_Calculator, &QAction::triggered, this, &MainWindow::resetCalculator );
    connect(ui->buttonStep, &QPushButton::clicked, this, &MainWindow::stepPressed);
    connect(this, &MainWindow::setDebugStepMode, &emu, &EmuThread::setDebugStepMode);
    connect(ui->buttonStepOver, &QPushButton::clicked, this, &MainWindow::stepOverPressed);
    connect(this, &MainWindow::setDebugStepOverMode, &emu, &EmuThread::setDebugStepOverMode);
    connect(ui->buttonStepOut, &QPushButton::clicked, this, &MainWindow::stepOutPressed);
    connect(this, &MainWindow::setDebugStepOutMode, &emu, &EmuThread::setDebugStepOutMode);
    connect(ui->buttonGoto, &QPushButton::clicked, this, &MainWindow::gotoPressed);
    connect(ui->disassemblyView, &QWidget::customContextMenuRequested, this, &MainWindow::disasmContextMenu);
    connect(ui->portView, &QTableWidget::itemChanged, this, &MainWindow::changePortData);

    // Debugger Options
    connect(ui->buttonAddEquateFile, &QPushButton::clicked, this, &MainWindow::addEquateFile);
    connect(ui->buttonClearEquates, &QPushButton::clicked, this, &MainWindow::clearEquateFile);
    connect(ui->textSizeSlider, &QSlider::valueChanged, this, &MainWindow::setFont);

    // Linking
    connect(ui->buttonSend, &QPushButton::clicked, this, &MainWindow::selectFiles);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::selectFiles);
    connect(this, &MainWindow::setSendState, &emu, &EmuThread::setSendState);
    connect(ui->buttonRefreshList, &QPushButton::clicked, this, &MainWindow::refreshVariableList);
    connect(this, &MainWindow::setReceiveState, &emu, &EmuThread::setReceiveState);
    connect(ui->buttonReceiveFiles, &QPushButton::clicked, this, &MainWindow::saveSelected);

    // Toolbar Actions
    connect(ui->actionSetup, &QAction::triggered, this, &MainWindow::runSetup);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionScreenshot, &QAction::triggered, this, &MainWindow::screenshot);
    connect(ui->actionRecord_GIF, &QAction::triggered, this, &MainWindow::recordGIF);
    connect(ui->actionTake_GIF_Screenshot, &QAction::triggered, this, &MainWindow::screenshotGIF);

    // Capture
    connect(ui->buttonScreenshot, &QPushButton::clicked, this, &MainWindow::screenshot);
    connect(ui->buttonGIF, &QPushButton::clicked, this, &MainWindow::recordGIF);
    connect(ui->buttonGIF_Screenshot, &QPushButton::clicked, this, &MainWindow::screenshotGIF);
    connect(ui->frameskipSlider, &QSlider::valueChanged, this, &MainWindow::changeFrameskip);

    // About
    connect(ui->actionCheck_for_updates, &QAction::triggered, this, [=](){ this->checkForUpdates(true); });
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->actionAbout_Qt, &QAction::triggered, qApp, &QApplication::aboutQt);

    // Other GUI actions
    connect(ui->buttonRunSetup, &QPushButton::clicked, this, &MainWindow::runSetup);
    connect(ui->refreshSlider, &QSlider::valueChanged, this, &MainWindow::changeLCDRefresh);
    connect(ui->checkAlwaysOnTop, &QCheckBox::stateChanged, this, &MainWindow::alwaysOnTop);
    connect(ui->emulationSpeed, &QSlider::valueChanged, this, &MainWindow::changeEmulatedSpeed);
    connect(ui->checkThrottle, &QCheckBox::stateChanged, this, &MainWindow::changeThrottleMode);
    connect(this, &MainWindow::changedEmuSpeed, &emu, &EmuThread::changeEmuSpeed);
    connect(this, &MainWindow::changedThrottleMode, &emu, &EmuThread::changeThrottleMode);
    connect(&emu, &EmuThread::actualSpeedChanged, this, &MainWindow::showActualSpeed, Qt::QueuedConnection);

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
    connect(ui->radioTilEmKeys, &QPushButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radioWabbitemuKeys, &QPushButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radioPindurTIKeys, &QPushButton::clicked, this, &MainWindow::keymapChanged);
    connect(ui->radioSmartViewKeys, &QPushButton::clicked, this, &MainWindow::keymapChanged);

    // Auto Updates
    connect(ui->checkUpdates, &QCheckBox::stateChanged, this, &MainWindow::autoCheckForUpdates);

    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<std::string>("std::string");

    ui->portView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->breakpointView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    setUIMode(true);
    setAcceptDrops(true);
    debuggerOn = false;

    settings = new QSettings();

    emu.rom = settings->value(QStringLiteral("romImage")).toString().toStdString();
    changeThrottleMode(Qt::Checked);

    if (fileExists(emu.rom)) {
        emu.start();
    } else if (!runSetup()) {
        exit(0);
    }

    restoreGeometry(settings->value(QStringLiteral("windowGeometry")).toByteArray());
    restoreState(settings->value(QStringLiteral("windowState")).toByteArray(), WindowStateVersion);
    changeFrameskip(settings->value(QStringLiteral("frameskip"), 3).toUInt());
    changeLCDRefresh(settings->value(QStringLiteral("refreshRate"), 60).toUInt());
    changeEmulatedSpeed(settings->value(QStringLiteral("emuRate"), 100).toUInt());
    alwaysOnTop(settings->value(QStringLiteral("onTop"), 0).toUInt());
    setFont(settings->value(QStringLiteral("textSize"), 9).toUInt());
    autoCheckForUpdates(settings->value(QStringLiteral("autoUpdate"), false).toBool());

    currentDir.setPath((settings->value(QStringLiteral("currDir"), QDir::homePath()).toString()));

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
    else if (QStringLiteral("pindurti").compare(currKeyMap, Qt::CaseInsensitive) == 0) {
        ui->radioPindurTIKeys->setChecked(true);
    }
    else if (QStringLiteral("smartview").compare(currKeyMap, Qt::CaseInsensitive) == 0) {
        ui->radioSmartViewKeys->setChecked(true);
    }
    changeKeymap(currKeyMap);

    ui->rompathView->setText(QString::fromStdString(emu.rom));
    ui->emuVarView->setSelectionBehavior(QAbstractItemView::SelectRows);
}

MainWindow::~MainWindow() {
    settings->setValue(QStringLiteral("windowState"), saveState(WindowStateVersion));
    settings->setValue(QStringLiteral("windowGeometry"), saveGeometry());
    settings->setValue(QStringLiteral("currDir"), currentDir.absolutePath());

    delete settings;
    delete ui->flashEdit;
    delete ui->ramEdit;
    delete ui->memEdit;
    delete ui;
}

void MainWindow::dropEvent(QDropEvent *e) {
    const QMimeData* mime_data = e->mimeData();
    if(!mime_data->hasUrls()) {
        return;
    }

    QStringList files;
    for(auto &&url : mime_data->urls()) {
        files.append(url.toLocalFile());
    }
    setSendState(true);
    QThread::msleep(300);

    sendFiles(files);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e) {
    if(e->mimeData()->hasUrls() == false) {
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
    qDebug("Terminating emulator thread...");

    if (emu.stop()) {
        qDebug("Successful!");
    } else {
        qDebug("Failed.");
    }

    detachedLCD.close();
    QMainWindow::closeEvent(e);
}

void MainWindow::consoleStr(QString str) {
    ui->console->moveCursor(QTextCursor::End);
    ui->console->insertPlainText(str);
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

void MainWindow::popoutLCD() {
    detachedState = !detachedState;
    if (detachedState) {
        detachedLCD.show();
    } else {
        detachedLCD.hide();
    }
    ui->actionDetached_LCD->setChecked(detachedState);
}

bool MainWindow::runSetup() {
    RomSelection romSelection;
    romSelection.show();
    romSelection.exec();

    emu.rom = romImagePath;

    if (!romImagePath.empty()) {
        settings->setValue(QStringLiteral("romImage"), QVariant(romImagePath.c_str()));
        if(emu.stop()) {
            ui->rompathView->setText(romImagePath.c_str());
            emu.start();
        }
    } else {
        return false;
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

void MainWindow::screenshot() {
    QImage image = renderFramebuffer();

    QString filename = QFileDialog::getSaveFileName(this, tr("Save Screenshot"), QString(), tr("PNG images (*.png)"));
    if (filename.isEmpty()) {
        return;
    }

    if (!image.save(filename, "PNG", 0)) {
        QMessageBox::critical(this, tr("Screenshot failed"), tr("Failed to save screenshot!"));
    }
}

void MainWindow::screenshotGIF() {
    if (ui->actionRecord_GIF->isChecked()) {
        QMessageBox::warning(this, tr("Recording GIF"), tr("Currently recording GIF."));
        return;
    }

    QString filename = QFileDialog::getSaveFileName(this, tr("Save Screenshot"), QString(), tr("GIF images (*.gif)"));
    if (filename.isEmpty()) {
        return;
    }

    if (!gif_single_frame(filename.toStdString().c_str())) {
        QMessageBox::critical(this, tr("Screenshot failed"), tr("Failed to save screenshot!"));
    }
}

void MainWindow::recordGIF() {
  static QString path;

  if (path.isEmpty()) {
      // TODO: Use QTemporaryFile?
      path = QDir::tempPath() + QDir::separator() + QStringLiteral("cemu_tmp.gif");

        gif_start_recording(path.toStdString().c_str(), ui->frameskipSlider->value());
    } else {
        if (gif_stop_recording()) {
            QString filename = QFileDialog::getSaveFileName(this, tr("Save Recording"), QString(), tr("GIF images (*.gif)"));
            if(filename.isEmpty()) {
                QFile(path).remove();
            } else {
                QFile(filename).remove();
                QFile(path).rename(filename);
            }
        } else {
            QMessageBox::warning(this, tr("Failed recording GIF"), tr("A failure occured during recording"));
        }
        path = QString();
    }

    ui->frameskipSlider->setEnabled(path.isEmpty());
    ui->actionRecord_GIF->setChecked(!path.isEmpty());
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

void MainWindow::clearConsole(void) {
    ui->console->clear();
    consoleStr("Console Cleared.\n");
}


void MainWindow::autoCheckForUpdates(int state) {
    settings->setValue(QStringLiteral("autoUpdate"), state);
    ui->checkUpdates->setChecked(state);

    if(state == Qt::Checked) {
        checkForUpdates(true);
    }
}

void MainWindow::checkForUpdates(bool forceInfoBox) {
    #define STRINGIFYMAGIC(x) #x
    #define STRINGIFY(x) STRINGIFYMAGIC(x)

    if (QStringLiteral(STRINGIFY(CEMU_VERSION)).endsWith(QStringLiteral("dev")))
    {
        if (forceInfoBox)
        {
            QMessageBox::warning(this, tr("Update check disabled"), tr("Checking updates is disabled for developer builds"));
        }
        return;
    }

    static const QString currentVersionReleaseURL = QStringLiteral("https://github.com/MateoConLechuga/CEmu/releases/tag/" STRINGIFY(CEMU_VERSION));
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply) {
        QString newVersionURL = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
        if (!newVersionURL.isEmpty())
        {
            if (newVersionURL.compare(currentVersionReleaseURL) == 0)
            {
                if (forceInfoBox)
                {
                    QMessageBox::information(this, tr("No update available"), tr("You already have the latest CEmu version (" STRINGIFY(CEMU_VERSION) ")"));
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
                                         "You can however <a href='https://github.com/MateoConLechuga/CEmu/releases/latest'>go here</a> to check yourself."));
                updateInfoBox.setTextFormat(Qt::RichText);
                updateInfoBox.show();
                updateInfoBox.exec();
            }
        }
    });

    manager->get(QNetworkRequest(QUrl(QStringLiteral("https://github.com/MateoConLechuga/CEmu/releases/latest"))));

    #undef STRINGIFY
    #undef STRINGIFYMAGIC
}

void MainWindow::showAbout() {
    #define STRINGIFYMAGIC(x) #x
    #define STRINGIFY(x) STRINGIFYMAGIC(x)
    QMessageBox about_box(this);
    about_box.setIconPixmap(QPixmap(":/icons/resources/icons/icon.png"));
    about_box.setWindowTitle(tr("About CEmu"));

    QAbstractButton* buttonUpdateCheck = about_box.addButton(tr("Check for updates"), QMessageBox::ActionRole);
    connect(buttonUpdateCheck, &QAbstractButton::clicked, this, [=](){ this->checkForUpdates(true); });

    QAbstractButton* okButton = about_box.addButton(QMessageBox::Ok);
    okButton->setFocus();

    about_box.setText(tr("<h3>CEmu %1</h3>"
                         "<a href='https://github.com/MateoConLechuga/CEmu'>On GitHub</a><br>"
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
                         "Many thanks to the <a href='https://github.com/KnightOS/z80e'>z80e</a> (MIT license <a href='https://github.com/KnightOS/z80e/blob/master/LICENSE'>here</a>) and <a href='https://github.com/nspire-emus/firebird'>Firebird</a> (GPLv3 license <a href='https://github.com/nspire-emus/firebird/blob/master/LICENSE'>here</a>) projects<br>"
                         "<br>"
                         "This work is licensed under the GPLv3.<br>"
                         "To view a copy of this license, visit <a href='https://www.gnu.org/licenses/gpl-3.0.html'>https://www.gnu.org/licenses/gpl-3.0.html</a>")
                         .arg(QStringLiteral(STRINGIFY(CEMU_VERSION))));
    about_box.setTextFormat(Qt::RichText);
    about_box.show();
    about_box.exec();
    #undef STRINGIFY
    #undef STRINGIFYMAGIC
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

void MainWindow::keymapChanged() {
    if (ui->radioCEmuKeys->isChecked()) {
        changeKeymap(QStringLiteral("cemu"));
    } else if (ui->radioTilEmKeys->isChecked()) {
        changeKeymap(QStringLiteral("tilem"));
    } else if (ui->radioWabbitemuKeys->isChecked()) {
        changeKeymap(QStringLiteral("wabbitemu"));
    } else if (ui->radioPindurTIKeys->isChecked()) {
        changeKeymap(QStringLiteral("pindurti"));
    } else if (ui->radioSmartViewKeys->isChecked()) {
        changeKeymap(QStringLiteral("smartview"));
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

QStringList MainWindow::showVariableFileDialog(QFileDialog::AcceptMode mode) {
    QFileDialog dialog(this);
    int good;

    dialog.setAcceptMode(mode);
    dialog.setFileMode(mode == QFileDialog::AcceptOpen ? QFileDialog::ExistingFiles : QFileDialog::AnyFile);
    dialog.setDirectory(currentDir);
    dialog.setNameFilter(tr("TI Variable (*.8xp *.8xv *.8xl *.8xn *.8xm *.8xy *.8xg *.8xs *.8ci *.8xd *.8xw *.8xc *.8xl *.8xz *.8xt *.8ca);;All Files (*.*)"));
    dialog.setDefaultSuffix("8xg");
    good = dialog.exec();

    currentDir = dialog.directory();

    if (good) {
        return dialog.selectedFiles();
    }

    return QStringList();
}

void MainWindow::sendFiles(QStringList fileNames) {
    ui->sendBar->setMaximum(fileNames.size());

    for (int i = 0; i < fileNames.size(); i++) {
        if(!sendVariableLink(fileNames.at(i).toUtf8())) {
            QMessageBox::warning(this, tr("Failed Transfer"), tr("A failure occured during transfer of: ")+fileNames.at(i));
        }
        ui->sendBar->setValue(ui->sendBar->value()+1);
    }

    setSendState(false);
    QThread::msleep(300);
    ui->sendBar->setMaximum(1);
    ui->sendBar->setValue(0);
}

void MainWindow::selectFiles() {
    if (debuggerOn) {
       return;
    }

    setSendState(true);

    QStringList fileNames = showVariableFileDialog(QFileDialog::AcceptOpen);

    sendFiles(fileNames);
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
        ui->actionReset_Calculator->setEnabled(true);
        setReceiveState(false);
    } else {
        ui->buttonRefreshList->setText(tr("Resume emulation"));
        ui->buttonReceiveFiles->setEnabled(true);
        ui->actionReset_Calculator->setEnabled(false);
        ui->buttonRun->setEnabled(false);
        setReceiveState(true);
        QThread::msleep(500);

        vat_search_init(&var);
        vars.clear();
        while (vat_search_next(&var)) {
            if (var.size > 2) {
                vars.append(var);
                currentRow = ui->emuVarView->rowCount();
                ui->emuVarView->setRowCount(currentRow + 1);

                QTableWidgetItem *var_name = new QTableWidgetItem(calc_var_name_to_utf8(var.name));
                QTableWidgetItem *var_type = new QTableWidgetItem(calc_var_type_names[var.type]);
                QTableWidgetItem *var_size = new QTableWidgetItem(QString::number(var.size));

                var_name->setCheckState(Qt::Unchecked);

                ui->emuVarView->setItem(currentRow, 0, var_name);
                ui->emuVarView->setItem(currentRow, 1, var_type);
                ui->emuVarView->setItem(currentRow, 2, var_size);
            }
        }
    }

    inReceivingMode = !inReceivingMode;
}

void MainWindow::saveSelected() {
    setReceiveState(true);

    QStringList fileNames = showVariableFileDialog(QFileDialog::AcceptSave);
    if (fileNames.size() == 1) {
        QVector<calc_var_t> selectedVars;
        for (int currentRow = 0; currentRow < ui->emuVarView->rowCount(); currentRow++) {
            if (ui->emuVarView->item(currentRow, 0)->checkState()) {
                selectedVars.append(vars[currentRow]);
            }
        }
        if (!receiveVariableLink(selectedVars.size(), selectedVars.constData(), fileNames.at(0).toUtf8())) {
            QMessageBox::warning(this, tr("Failed Transfer"), tr("A failure occured during transfer of: ")+fileNames.at(0));
        }
    }
}

void MainWindow::setFont(int fontSize) {
    ui->textSizeSlider->setValue(fontSize);
    settings->setValue(QStringLiteral("textSize"), ui->textSizeSlider->value());

    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    monospace.setPointSize(fontSize);
    ui->console->setFont(monospace);
    ui->opView->setFont(monospace);
    ui->disassemblyView->setFont(monospace);

    ui->portRequest->setFont(monospace);
    ui->breakRequest->setFont(monospace);
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

/* ================================================ */
/* Debugger Things                                  */
/* ================================================ */

static int hex2int(QString str) {
    return std::stoi(str.toStdString(), nullptr, 16);
}

static QString int2hex(uint32_t a, uint8_t l) {
    return QString::number(a, 16).rightJustified(l, '0').toUpper();
}

void MainWindow::raiseDebugger() {
    // make sure we are set on the debug window, just in case
    if (debuggerDock) {
        debuggerDock->setVisible(true);
        debuggerDock->raise();
    }
    ui->tabWidget->setCurrentWidget(ui->tabDebugger);

    populateDebugWindow();
    setDebuggerState(true);
}

void MainWindow::updateDebuggerChanges() {
  /* Update all the changes in the core */
  if (debuggerOn == false) {
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
      cpu.IM = static_cast<uint8_t>(hex2int(ui->imregView->text()));

      cpu.registers.flags.Z = ui->checkZ->isChecked();
      cpu.registers.flags.C = ui->checkC->isChecked();
      cpu.registers.flags.H = ui->checkHC->isChecked();
      cpu.registers.flags.PV = ui->checkPV->isChecked();
      cpu.registers.flags.N = ui->checkN->isChecked();
      cpu.registers.flags.S = ui->checkS->isChecked();
      cpu.registers.flags._5 = ui->check5->isChecked();
      cpu.registers.flags._3 = ui->check3->isChecked();

      cpu.halted = ui->checkHalted->isChecked();
      cpu.MADL = ui->checkMADL->isChecked();
      cpu.halted = ui->checkHalted->isChecked();
      cpu.IEF1 = ui->checkIEF1->isChecked();
      cpu.IEF2 = ui->checkIEF2->isChecked();

      control.batteryCharging = ui->checkCharging->isChecked();
      control.setBatteryStatus = static_cast<uint8_t>(ui->sliderBattery->value());

      cpu_flush(static_cast<uint32_t>(hex2int(ui->pcregView->text())), ui->checkADL->isChecked());

      backlight.brightness = static_cast<uint8_t>(ui->brightnessSlider->value());

      lcd.upbase = static_cast<uint32_t>(hex2int(ui->lcdbaseView->text()));
      lcd.upcurr = static_cast<uint32_t>(hex2int(ui->lcdcurrView->text()));
      lcd.control &= ~14;

      uint8_t bpp = 0;
      switch(ui->bppView->text().toInt()) {
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
}

void MainWindow::setDebuggerState(bool state) {
    QPixmap pix;
    QIcon icon;

    debuggerOn = state;

    if (debuggerOn) {
        ui->buttonRun->setText("Run");
        pix.load(":/icons/resources/icons/run.png");
        debug_clear_run_until();
    } else {
        ui->buttonRun->setText("Stop");
        pix.load(":/icons/resources/icons/stop.png");
        ui->portChangeView->clear();
        ui->breakChangeView->clear();
        ui->opView->clear();
    }
    setReceiveState(false);
    icon.addPixmap(pix);
    ui->buttonRun->setIcon(icon);
    ui->buttonRun->setIconSize(pix.size());

    ui->tabDebugging->setEnabled( debuggerOn );
    ui->buttonGoto->setEnabled( debuggerOn );
    ui->buttonStep->setEnabled( debuggerOn );
    ui->buttonStepOver->setEnabled( debuggerOn );
    ui->buttonStepOut->setEnabled( debuggerOn );
    ui->groupCPU->setEnabled( debuggerOn );
    ui->groupFlags->setEnabled( debuggerOn );
    ui->groupRegisters->setEnabled( debuggerOn );
    ui->groupInterrupts->setEnabled( debuggerOn );
    ui->groupStack->setEnabled( debuggerOn );
    ui->groupFlash->setEnabled( debuggerOn );
    ui->groupRAM->setEnabled( debuggerOn );
    ui->groupMem->setEnabled( debuggerOn );

    ui->buttonSend->setEnabled( !debuggerOn );
    ui->buttonRefreshList->setEnabled( !debuggerOn );
    ui->emuVarView->setEnabled( !debuggerOn );
    ui->buttonReceiveFiles->setEnabled( !debuggerOn && inReceivingMode);

    if (!debuggerOn) {
        updateDebuggerChanges();
        if (inReceivingMode) {
            inReceivingMode = false;
            refreshVariableList();
        }
    }
}

void MainWindow::changeDebuggerState() {
    if (emu.rom.empty()) {
        return;
    }

    debuggerOn = !debuggerOn;
    if (!debuggerOn) {
        setDebuggerState(false);
    }
    emit debuggerChangedState( debuggerOn );
}

void MainWindow::populateDebugWindow() {
    QPalette colorback, nocolorback;
    QString tmp;
    colorback.setColor(QPalette::Base, QColor(Qt::yellow).lighter(160));
    nocolorback.setColor(QPalette::Base, QColor(Qt::white));

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

    tmp = int2hex(cpu.IM, 1);
    ui->imregView->setPalette(tmp == ui->imregView->text() ? nocolorback : colorback);
    ui->imregView->setText(tmp);

    tmp = int2hex(cpu.registers.PC, 6);
    ui->pcregView->setPalette(tmp == ui->pcregView->text() ? nocolorback : colorback);
    ui->pcregView->setText(tmp);

    tmp = int2hex(cpu.registers.R, 2);
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

    ui->checkCharging->setChecked(control.batteryCharging);
    ui->sliderBattery->setValue(control.setBatteryStatus);

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

    QString formattedLine;
    QString opData;
    QString opType;
    uint8_t gotData[11];
    uint8_t index;

    ui->opView->clear();

    for(uint32_t i = 0xD005F8; i<0xD005F8+11*6; i+=11) {
        opData.clear();
        opType.clear();
        index = 0;
        for(uint32_t j = i; j < i+11; j++) {
            gotData[index] = debug_read_byte(j);
            opData += int2hex(gotData[index], 2)+" ";
            index++;
        }
        if (*gotData < 0x40) {
            opType = QString(calc_var_type_names[*gotData]);
        }

        formattedLine = QString("<pre><b><font color='#444'>%1</font></b><font color='darkblue'>    %2    </font>%3 <font color='green'>%4</font></pre>")
                                       .arg(int2hex(i, 6), "OP"+QString::number(((i-0xD005F8)/11)+1), opData, opType);

        ui->opView->appendHtml(formattedLine);
    }

    updateStackView();
    ramUpdate();
    flashUpdate();
    memUpdate();
}

void MainWindow::updateDisasmView(const int sentBase, const bool newPane) {
    addressPane = sentBase;
    fromPane = newPane;
    disasmOffsetSet = false;
    disasm.adl = ui->checkADL->isChecked();
    disasm.base_address = -1;
    disasm.new_address = addressPane - ((newPane) ? 0x80 : 0);
    if(disasm.new_address < 0) disasm.new_address = 0;

    ui->disassemblyView->clear();
    ui->disassemblyView->clearAllHighlights();

    ui->disassemblyView->cursorState(false);

    for(int i=0; i<0x80; i++) {
        drawNextDisassembleLine();
        if (disasm.new_address > 0xFFFFFF) break;
    }

    ui->disassemblyView->cursorState(true);

    ui->disassemblyView->updateAllHighlights();

    ui->disassemblyView->setTextCursor(disasmOffset);
    ui->disassemblyView->centerCursor();
}

void MainWindow::portMonitorCheckboxToggled(QTableWidgetItem * item) {
    auto col = item->column();
    auto row = item->row();

    uint16_t port = static_cast<uint16_t>(hex2int(ui->portView->item(row, 0)->text()));
    uint8_t value = DBG_NO_HANDLE;

    if (col > 1)
    {
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
    }
}

void MainWindow::addPort() {
    uint8_t read;
    uint16_t port;

    const int currentRow = ui->portView->rowCount();

    if (ui->portRequest->text().isEmpty()) {
        return;
    }

    std::string s = ui->portRequest->text().toUpper().toStdString();
    if (s.find_first_not_of("0123456789ABCDEF") != std::string::npos) {
        return;
    }

    /* Mark the port as read active */
    port = static_cast<uint16_t>(hex2int(QString::fromStdString(s)));
    read = static_cast<uint8_t>(debug_port_read_byte(port));

    QString portString = int2hex(port,4);

    /* Return if port is already set */
    for (int i=0; i<currentRow; ++i) {
        if (ui->portView->item(i, 0)->text() == portString) {
            return;
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

    ui->portRequest->clear();
    ui->portView->setUpdatesEnabled(true);
    ui->portView->blockSignals(false);
}

void MainWindow::changePortData(QTableWidgetItem *curr_item) {
    const int currentRow = curr_item->row();
    uint16_t port = static_cast<uint16_t>(hex2int(ui->portView->item(currentRow, 0)->text()));

    if (curr_item->column() == 0) {
        debug_pmonitor_remove(port);

        uint16_t newport = static_cast<uint16_t>(hex2int(curr_item->text()));
        curr_item->setText(int2hex(newport, 4));

        unsigned int value = ((ui->portView->item(currentRow, 2)->checkState() == Qt::Checked) ? DBG_PORT_READ : DBG_NO_HANDLE)  |
                             ((ui->portView->item(currentRow, 3)->checkState() == Qt::Checked) ? DBG_PORT_WRITE : DBG_NO_HANDLE) |
                             ((ui->portView->item(currentRow, 4)->checkState() == Qt::Checked) ? DBG_PORT_FREEZE : DBG_NO_HANDLE);

        debug_pmonitor_set(newport, value, true);
        ui->portView->item(currentRow, 1)->setText(int2hex(debug_port_read_byte(newport), 2));
    }
    if (curr_item->column() == 1) {
        uint8_t pdata = static_cast<uint8_t>(hex2int(curr_item->text()));

        debug_port_write_byte(port, pdata);

        curr_item->setText(int2hex(debug_port_read_byte(port), 2));
    }
}

void MainWindow::deletePort() {
    if (!ui->portView->rowCount() || !ui->portView->selectionModel()->isSelected(ui->portView->currentIndex())) {
        return;
    }

    const int currentRow = ui->portView->currentRow();
    uint16_t port = static_cast<uint16_t>(hex2int(ui->portView->item(currentRow, 0)->text()));

    debug_pmonitor_remove(port);
    ui->portView->removeRow(currentRow);
}

void MainWindow::breakpointCheckboxToggled(QTableWidgetItem * item) {
    auto col = item->column();
    auto row = item->row();

    uint32_t address = static_cast<uint32_t>(hex2int(ui->breakpointView->item(row, 0)->text()));
    unsigned int value = DBG_NO_HANDLE;

    if (col > 0) {
        if (col == 1) { // Break on read
            value = DBG_READ_BREAKPOINT;
        }
        if (col == 2) { // Break on write
            value = DBG_WRITE_BREAKPOINT;
        }
        if (col == 3) { // Break on execution
            value = DBG_EXEC_BREAKPOINT;
        }
    }

    debug_breakpoint_set(address, value, item->checkState() == Qt::Checked);

    updateDisasmView(address, true);
}

bool MainWindow::addBreakpoint() {
    uint32_t address;

    const int currentRow = ui->breakpointView->rowCount();

    if (ui->breakRequest->text().isEmpty()) {
        return false;
    }

    std::string s = ui->breakRequest->text().toUpper().toStdString();
    if (s.find_first_not_of("0123456789ABCDEF") != std::string::npos) {
        return false;
    }

    address = static_cast<uint32_t>(hex2int(QString::fromStdString(s)));

    QString addressString = int2hex(address,6);

    /* Return if address is already set */
    for (int i=0; i<currentRow; ++i) {
        if (ui->breakpointView->item(i, 0)->text() == addressString) {
            ui->breakpointView->selectRow(i);
            return false;
        }
    }

    ui->breakpointView->setUpdatesEnabled(false);
    ui->breakpointView->blockSignals(true);

    ui->breakpointView->setRowCount(currentRow + 1);

    QTableWidgetItem *iaddress = new QTableWidgetItem(addressString);
    QTableWidgetItem *rBreak = new QTableWidgetItem();
    QTableWidgetItem *wBreak = new QTableWidgetItem();
    QTableWidgetItem *eBreak = new QTableWidgetItem();

    rBreak->setCheckState(Qt::Unchecked);
    wBreak->setCheckState(Qt::Unchecked);
    eBreak->setCheckState(Qt::Checked);

    ui->breakpointView->setItem(currentRow, 0, iaddress);
    ui->breakpointView->setItem(currentRow, 1, rBreak);
    ui->breakpointView->setItem(currentRow, 2, wBreak);
    ui->breakpointView->setItem(currentRow, 3, eBreak);

    ui->breakpointView->selectRow(currentRow);

    ui->breakRequest->clear();
    ui->breakpointView->setUpdatesEnabled(true);
    ui->breakpointView->blockSignals(false);

    debug_breakpoint_set(address, DBG_EXEC_BREAKPOINT, true);
    updateDisasmView(address, true);
    return true;
}

void MainWindow::deleteBreakpoint() {
    if(!ui->breakpointView->rowCount() || !ui->breakpointView->selectionModel()->isSelected(ui->breakpointView->currentIndex())) {
        return;
    }

    const int currentRow = ui->breakpointView->currentRow();
    uint32_t address = static_cast<uint32_t>(hex2int(ui->breakpointView->item(currentRow, 0)->text()));

    debug_breakpoint_remove(address);

    ui->breakpointView->removeRow(currentRow);
    updateDisasmView(address, true);
}

void MainWindow::processDebugCommand(int reason, uint32_t input) {
    int row = 0;

    if (reason == DBG_STEP || reason == DBG_USER) {
        updateDisasmView(cpu.registers.PC, true);
    }

    // We hit a normal breakpoint; raise the correct entry in the port monitor table
    if (reason == HIT_READ_BREAKPOINT || reason == HIT_WRITE_BREAKPOINT || reason == HIT_EXEC_BREAKPOINT) {
        ui->tabDebugging->setCurrentIndex(0);

        // find the correct entry
        while( static_cast<uint32_t>(hex2int(ui->breakpointView->item(row++, 0)->text())) != input );
        row--;

        ui->breakChangeView->setText("Address "+ui->breakpointView->item(row, 0)->text()+" "+((reason == HIT_READ_BREAKPOINT) ? "Read" : (reason == HIT_WRITE_BREAKPOINT) ? "Write" : "Executed"));
        ui->breakpointView->selectRow(row);

        updateDisasmView(input, true);
    }

    // We hit a port read or write; raise the correct entry in the port monitor table
    if (reason == HIT_PORT_READ_BREAKPOINT || reason == HIT_PORT_WRITE_BREAKPOINT) {
        ui->tabDebugging->setCurrentIndex(1);
        // find the correct entry
        while( static_cast<uint32_t>(hex2int(ui->portView->item(row++, 0)->text())) != input );
        row--;

        ui->portChangeView->setText("Port "+ui->portView->item(row, 0)->text()+" "+((reason == HIT_PORT_READ_BREAKPOINT) ? "Read" : "Write"));
        ui->portView->selectRow(row);
    }
}

void MainWindow::updatePortData(int currentRow) {
    uint16_t port = static_cast<uint16_t>(hex2int(ui->portView->item(currentRow, 0)->text()));
    uint8_t read = static_cast<uint8_t>(debug_port_read_byte(port));

    ui->portView->item(currentRow, 1)->setText(int2hex(read,2));
}

void MainWindow::resetCalculator() {
    if (emu.stop()) {
        while(ui->portView->rowCount() > 0) {
            ui->portView->removeRow(0);
        }
        while(ui->breakpointView->rowCount() > 0) {
            ui->breakpointView->removeRow(0);
        }
        emu.start();
        if(debuggerOn) {
            emit setDebugStepMode();
        }
    } else {
        qDebug("Reset Failed.");
    }
}

void MainWindow::updateStackView() {
    ui->stackView->clear();

    QString formattedLine;

    for(int i=0; i<30; i+=3) {
       formattedLine = QString("<pre><b><font color='#444'>%1</font></b> %2</pre>")
                                .arg(int2hex(cpu.registers.SPL+i, 6),
                                     int2hex(debug_read_long(cpu.registers.SPL+i), 6));
        ui->stackView->appendHtml(formattedLine);
    }
    ui->stackView->moveCursor(QTextCursor::Start);
}

void MainWindow::drawNextDisassembleLine() {
    std::string *label = 0;
    if (disasm.base_address != disasm.new_address) {
        disasm.base_address = disasm.new_address;
        addressMap_t::iterator item = disasm.address_map.find(disasm.new_address);
        if (item != disasm.address_map.end()) {
            disasmHighlight.hit_read_breakpoint = false;
            disasmHighlight.hit_write_breakpoint = false;
            disasmHighlight.hit_exec_breakpoint = false;
            disasmHighlight.hit_run_breakpoint = false;
            disasmHighlight.hit_pc = false;

            disasm.instruction.data = "";
            disasm.instruction.opcode = "";
            disasm.instruction.mode_suffix = " ";
            disasm.instruction.arguments = "";
            disasm.instruction.size = 0;

            label = &item->second;
        } else {
            disassembleInstruction();
        }
    } else {
        disassembleInstruction();
    }

    // Watch out, maintainers: the (unformatted) line is later "parsed" in DisasmWidget::getSelectedAddress()
    // with a cursor getting the address from it. Make sure the start position is correct.

    // Some round symbol things
    QString breakpointSymbols = QString("<font color='#A3FFA3'><big>%1</big></font><font color='#A3A3FF'><big>%2</big></font><font color='#FFA3A3'><big>%3</big></font>")
                                   .arg(((disasmHighlight.hit_read_breakpoint == true)  ? "&#9679;" : " "),
                                        ((disasmHighlight.hit_write_breakpoint == true) ? "&#9679;" : " "),
                                        ((disasmHighlight.hit_exec_breakpoint == true)  ? "&#9679;" : " "));

    // Simple syntax highlighting
    QString instructionArgsHighlighted = QString::fromStdString(disasm.instruction.arguments)
                                        .replace(QRegularExpression("(\\$[0-9a-fA-F]+)"), "<font color='green'>\\1</font>") // hex numbers
                                        .replace(QRegularExpression("(^\\d)"), "<font color='blue'>\\1</font>")             // dec number
                                        .replace(QRegularExpression("([()])"), "<font color='#600'>\\1</font>");            // parentheses

    QString formattedLine = QString("<pre><b>%1<font color='#444'>%2</font></b>    %3  <font color='darkblue'>%4%5</font>%6</pre>")
                               .arg(breakpointSymbols,
                                    int2hex(disasm.base_address, 6),
                                    label ? QString::fromStdString(*label) + ":" : ui->checkDataCol->isChecked() ? QString::fromStdString(disasm.instruction.data).leftJustified(12, ' ') : "",
                                    QString::fromStdString(disasm.instruction.opcode),
                                    QString::fromStdString(disasm.instruction.mode_suffix),
                                    instructionArgsHighlighted);

    ui->disassemblyView->appendHtml(formattedLine);

    if (addressPane == disasm.base_address) {
        disasmOffsetSet = true;
        disasmOffset = ui->disassemblyView->textCursor();
        disasmOffset.movePosition(QTextCursor::StartOfLine);
    } else if (disasmOffsetSet == false && addressPane <= disasm.base_address+7) {
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
    QString set_pc = "Set PC to this address";
    QString run_until = "Toggle Run Until this address";
    QString toggle_break = "Toggle Breakpoint at this address";
    ui->disassemblyView->setTextCursor(ui->disassemblyView->cursorForPosition(posa));
    QPoint globalPos = ui->disassemblyView->mapToGlobal(posa);

    QMenu contextMenu;
    contextMenu.addAction(set_pc);
    contextMenu.addAction(toggle_break);
    contextMenu.addAction(run_until);

    QAction* selectedItem = contextMenu.exec(globalPos);
    if (selectedItem) {
        if (selectedItem->text() == set_pc) {
            ui->pcregView->setText(ui->disassemblyView->getSelectedAddress());
            uint32_t address = static_cast<uint32_t>(hex2int(ui->pcregView->text()));
            debug_set_pc_address(address);
            updateDisasmView(cpu.registers.PC, true);
        } else if (selectedItem->text() == toggle_break) {
            setBreakpointAddress();
        } else if (selectedItem->text() == run_until) {
            uint32_t address = static_cast<uint32_t>(hex2int(ui->disassemblyView->getSelectedAddress()));
            debug_toggle_run_until(address);
            updateDisasmView(address, true);
        }
    }
}

void MainWindow::stepPressed() {
    debuggerOn = false;
    updateDebuggerChanges();
    emit setDebugStepMode();
}

void MainWindow::stepOverPressed() {
    setDebuggerState(false);
    emit setDebugStepOverMode();
}

void MainWindow::stepOutPressed() {
    setDebuggerState(false);
    emit setDebugStepOutMode();
}

void MainWindow::setBreakpointAddress() {
    QString address = ui->disassemblyView->getSelectedAddress();

    ui->breakRequest->setText(address);
    if(!addBreakpoint()) {
        deleteBreakpoint();
    }

    ui->breakRequest->clear();
}

QString MainWindow::getAddressString(bool &ok, QString String) {
    QString address = QInputDialog::getText(this, tr("Goto Address"),
                                         tr("Input Address (In Hexadecimal):"), QLineEdit::Normal,
                                         String, &ok).toUpper();

    if (!ok || (address.toStdString().find_first_not_of("0123456789ABCDEF") != std::string::npos) || (address.length() > 6)) {
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

void MainWindow::memUpdate() {
    ui->memEdit->setFocus();
    QByteArray mem_data;

    bool locked = ui->checkLockPosition->isChecked();
    int32_t start, line = 0;

    if (locked) {
        start = static_cast<int32_t>(ui->memEdit->addressOffset());
        line = ui->memEdit->getLine();
    } else {
        start = static_cast<int32_t>(cpu.registers.PC) - 0x1000;
    }

    if (start < 0) { start = 0; }
    int32_t end = start+0x2000;
    if (end > 0xFFFFFF) { end = 0xFFFFFF; }

    memSize = end-start;

    for (int32_t i=start; i<end; i++) {
        mem_data.append(debug_read_byte(i));
    }

    ui->memEdit->setData(mem_data);
    ui->memEdit->setAddressOffset(start);

    if (locked) {
        ui->memEdit->setLine(line);
    } else {
        ui->memEdit->setCursorPosition((cpu.registers.PC-start)<<1);
        ui->memEdit->ensureVisible();
    }
}

void MainWindow::searchEdit(QHexEdit *editor) {
    bool ok;
    QString searchString = QInputDialog::getText(this, tr("Search"),
                                                  tr("Input Hexadecimal Search String:"), QLineEdit::Normal,
                                                  "", &ok).toUpper();
    editor->setFocus();
    if(!ok || (searchString.length() & 1)) {
        return;
    }
    QByteArray string_int;
    for (int i=0; i<searchString.length(); i+=2) {
        QString a = searchString.at(i);
        a.append(searchString.at(i+1));
        string_int.append(hex2int(a));
    }
    editor->indexOf(string_int, editor->cursorPosition());
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

void MainWindow::memGotoPressed() {
    bool ok;
    QString address = getAddressString(ok, "");

    ui->memEdit->setFocus();
    if (!ok) {
        return;
    }
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
        mem_data.append(debug_read_byte(i));
    }

    ui->memEdit->setData(mem_data);
    ui->memEdit->setAddressOffset(start);
    ui->memEdit->setCursorPosition((int_address-start)<<1);
    ui->memEdit->ensureVisible();
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
        debug_write_byte(i+start, ui->memEdit->dataAt(i, 1).at(0));
    }

    syncHexView(posa, ui->memEdit);
}

void MainWindow::clearEquateFile() {
    // Reset the map
    disasm.address_map.clear();
    QMessageBox::warning(this, tr("Equates Cleared"), tr("Cleared disassembly equates."));
}

void MainWindow::addEquateFile() {
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
    in.open(dialog.selectedFiles().at(0).toStdString());

    if (in.good()) {
        QRegularExpression equatesRegexp("^\\h*([^\\W\\d]\\w*)\\h*(?:=|\\h\\.?equ(?!\\d))\\h*(?|\\$([\\da-f]{6,})|(\\d[\\da-f]{5,})h)\\h*(?:;.*)?$",
                                         QRegularExpression::CaseInsensitiveOption);
        // Reset the map
        disasm.address_map.clear();
        while (std::getline(in, current)) {
            QRegularExpressionMatch matches = equatesRegexp.match(QString::fromStdString(current));
            if (matches.hasMatch()) {
                uint32_t address = static_cast<uint32_t>(matches.capturedRef(2).toUInt(nullptr, 16));
                std::string &item = disasm.address_map[address];
                if (item.empty()) {
                    item = matches.captured(1).toStdString();
                    uint8_t *ptr = phys_mem_ptr(address - 4, 9);
                    if (ptr && ptr[4] == 0xC3 && (ptr[0] == 0xC3 || ptr[8] == 0xC3)) { // jump table?
                        uint32_t address2  = ptr[5] | ptr[6] << 8 | ptr[7] << 16;
                        if (phys_mem_ptr(address2, 1)) {
                            std::string &item2 = disasm.address_map[address2];
                            if (item2.empty()) {
                                item2 = "_" + item;
                            }
                        }
                    }
                }
            }
        }
        in.close();
        QMessageBox::information(this, tr("Equates Loaded"), tr("Loaded disassembly equates."));
    } else {
        QMessageBox messageBox;
        messageBox.critical(0, tr("Error"), tr("Couldn't open this file"));
        messageBox.setFixedSize(500,200);
    }
}
