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

#include "../../core/schedule.h"
#include "../../core/debug/disasmc.h"
#include "../../core/link.h"
#include "../../core/capture/gif.h"
#include "utils.h"
#include "../../core/os/os.h"

static const constexpr int WindowStateVersion = 0;
static const int flash_size = 0x400000;
static const uint32_t ram_size = 0x65800;

MainWindow::MainWindow(QWidget *p) : QMainWindow(p), ui(new Ui::MainWindow) {
    // Setup the UI
    ui->setupUi(this);

    // Register QtKeypadBridge for the virtual keyboard functionality
    this->installEventFilter(&qt_keypad_bridge);
    detached_lcd.installEventFilter(&qt_keypad_bridge);
    // Same for all the tabs/docks (iterate over them instead of harcoding their names)
    for (const auto& tab : ui->tabWidget->children()[0]->children()) {
        tab->installEventFilter(&qt_keypad_bridge);
    }

    ui->keypadWidget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    ui->disassemblyView->setContextMenuPolicy(Qt::CustomContextMenu);

    // View
    detached_lcd.setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->actionDetached_LCD, &QAction::triggered, this, &MainWindow::popoutLCD);
    connect(&detached_lcd, &LCDWidget::closed, this, &MainWindow::popoutLCD);
    connect(&detached_lcd, &LCDWidget::lcdOpenRequested, this, &MainWindow::selectFiles);
    connect(ui->lcdWidget, &LCDWidget::lcdOpenRequested, this, &MainWindow::selectFiles);

    // Emulator -> GUI
    connect(&emu, &EmuThread::consoleStr, this, &MainWindow::consoleStr);
    // Console actions
    connect(ui->buttonConsoleclear, &QPushButton::clicked, this, &MainWindow::clearConsole);

    // Debugger
    connect(ui->buttonRun, &QPushButton::clicked, this, &MainWindow::changeDebuggerState);
    connect(this, &MainWindow::debuggerChangedState, &emu, &EmuThread::setDebugMode);
    connect(&emu, &EmuThread::debuggerEntered, this, &MainWindow::raiseDebugger);
    connect(&emu, &EmuThread::sendDebugCommand, this, &MainWindow::processDebugCommand);
    connect(ui->buttonAddPort, &QPushButton::clicked, this, &MainWindow::pollPort);
    connect(ui->portRequest, &QLineEdit::returnPressed, this, &MainWindow::pollPort);
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
    connect(ui->buttonBreakpoint, &QPushButton::clicked, this, &MainWindow::breakpointPressed);
    connect(ui->buttonGoto, &QPushButton::clicked, this, &MainWindow::gotoPressed);
    connect(ui->disassemblyView, &QWidget::customContextMenuRequested, this, &MainWindow::setPCaddress);

    // Debugger Options
    connect(ui->buttonAddEquateFile, &QPushButton::clicked, this, &MainWindow::addEquateFile);
    connect(ui->buttonClearEquates, &QPushButton::clicked, this, &MainWindow::clearEquateFile);

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

    // About
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->actionAbout_Qt, &QAction::triggered, qApp, &QApplication::aboutQt);

    // Other GUI actions
    connect(ui->buttonRunSetup, &QPushButton::clicked, this, &MainWindow::runSetup);
    connect(ui->refreshSlider, &QSlider::valueChanged, this, &MainWindow::changeLCDRefresh);
    connect(ui->checkAlwaysOnTop, &QCheckBox::stateChanged, this, &MainWindow::alwaysOnTop);
    connect(ui->emulationSpeed, &QSlider::valueChanged, this, &MainWindow::changeEmulatedSpeed);
    connect(this, &MainWindow::changedEmuSpeed, &emu, &EmuThread::changeEmuSpeed);

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

    // Set up monospace fonts
    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->console->setFont(monospace);
    ui->disassemblyView->setFont(monospace);
    ui->stackView->setFont(monospace);

    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<std::string>("std::string");

    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    setUIMode(true);
    setAcceptDrops(true);
    debugger_on = false;

#ifdef Q_OS_ANDROID
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    settings = new QSettings(path + QStringLiteral("/cemu.ini"), QSettings::IniFormat);
#else
    settings = new QSettings();
#endif

    emu.rom = settings->value(QStringLiteral("romImage")).toString().toStdString();

    if (fileExists(emu.rom)) {
        emu.start();
    } else if (!runSetup()) {
        exit(0);
    }

    restoreGeometry(settings->value(QStringLiteral("windowGeometry")).toByteArray());
    restoreState(settings->value(QStringLiteral("windowState")).toByteArray(), WindowStateVersion);
    changeLCDRefresh(settings->value(QStringLiteral("refreshRate"), 60).toInt());
    changeEmulatedSpeed(settings->value(QStringLiteral("emuRate"), 100).toInt());
    alwaysOnTop(settings->value(QStringLiteral("onTop"), 0).toInt());
    ui->textSizeSlider->setValue(settings->value(QStringLiteral("disasmTextSize"), 9).toInt());

    ui->rompathView->setText(QString::fromStdString(emu.rom));
    ui->portView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->emuVarView->setSelectionBehavior(QAbstractItemView::SelectRows);
    current_dir = QDir::homePath();
}

MainWindow::~MainWindow() {
    settings->setValue(QStringLiteral("windowState"), saveState(WindowStateVersion));
    settings->setValue(QStringLiteral("windowGeometry"), saveGeometry());
    settings->setValue(QStringLiteral("disasmTextSize"), ui->textSizeSlider->value());

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
    QThread::usleep(500);

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

    detached_lcd.close();
    QMainWindow::closeEvent(e);
}

void MainWindow::consoleStr(QString str) {
    ui->console->moveCursor(QTextCursor::End);
    ui->console->insertPlainText(str);
}

void MainWindow::popoutLCD() {
    detached_state = !detached_state;
    if (detached_state) {
        detached_lcd.show();
    } else {
        detached_lcd.hide();
    }
    ui->actionDetached_LCD->setChecked(detached_state);
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
    QMenu *docks_menu = new QMenu(tr("Docks"), this);
    ui->menubar->insertMenu(ui->menuAbout->menuAction(), docks_menu);

    //Convert the tabs into QDockWidgets
    QDockWidget *last_dock = nullptr;
    while(ui->tabWidget->count()) {
        QDockWidget *dw = new QDockWidget(ui->tabWidget->tabText(0));
        dw->setWindowIcon(ui->tabWidget->tabIcon(0));
        dw->setObjectName(dw->windowTitle());

        // Fill "Docks" menu
        QAction *action = dw->toggleViewAction();
        action->setIcon(dw->windowIcon());
        docks_menu->addAction(action);

        QWidget *tab = ui->tabWidget->widget(0);
        if(tab == ui->tabDebugger)
            dock_debugger = dw;

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

    if (!image.save(filename, "PNG")) {
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

        gif_start_recording(path.toStdString().c_str(), ui->gif_frame_skip_slider->value());
    } else {
        if (gif_stop_recording()) {
            QString filename = QFileDialog::getSaveFileName(this, tr("Save Recording"), QString(), tr("GIF images (*.gif)"));
            if(filename.isEmpty())
                QFile(path).remove();
            else
                QFile(path).rename(filename);
        } else {
            QMessageBox::warning(this, tr("Failed recording GIF"), tr("A failure occured during recording"));
        }
        path = QString();
    }

    ui->actionRecord_GIF->setChecked(!path.isEmpty());
    ui->buttonGIF->setText((!path.isEmpty()) ? QString("Stop Recording") : QString("Record GIF"));
}

void MainWindow::clearConsole(void) {
    ui->console->clear();
    consoleStr("Console Cleared.\n");
}

void MainWindow::showAbout() {
    #define STRINGIFYMAGIC(x) #x
    #define STRINGIFY(x) STRINGIFYMAGIC(x)
    QMessageBox about_box(this);
    about_box.addButton(QMessageBox::Ok);
    about_box.setIconPixmap(QPixmap(":/icons/resources/icons/icon.png"));
    about_box.setWindowTitle(tr("About CEmu"));
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
    ui->refreshLabel->setText(QString::fromStdString(std::to_string(value))+" FPS");
    ui->refreshSlider->setValue(value);
    ui->lcdWidget->refreshRate(value);
}

void MainWindow::changeEmulatedSpeed(int value) {
    settings->setValue(QStringLiteral("emuRate"), value);
    ui->emulationSpeedLabel->setText(QString::fromStdString(std::to_string(value)).rightJustified(3,'0')+"%");
    ui->emulationSpeed->setValue(value);
    emit changedEmuSpeed(value);
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
    dialog.setDirectory(current_dir);
    dialog.setNameFilter(tr("TI Variable (*.8xp *.8xv *.8xl *.8xn *.8xm *.8xy *.8xg *.8xs *.8ci *.8xd *.8xw *.8xc *.8xl *.8xz *.8xt *.8ca);;All Files (*.*)"));
    dialog.setDefaultSuffix("8xg");
    good = dialog.exec();

    current_dir = dialog.directory();

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
    if (debugger_on) {
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

    if (debugger_on) {
        return;
    }

    if (in_recieving_mode) {
        ui->buttonRefreshList->setText("Refresh Emulator Variable List...");
        ui->buttonReceiveFiles->setEnabled(false);
        setReceiveState(false);
    } else {
        ui->buttonRefreshList->setText("Continue Emulation");
        ui->buttonReceiveFiles->setEnabled(true);
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

    in_recieving_mode = !in_recieving_mode;
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

/* ================================================ */
/* Debugger Things                                  */
/* ================================================ */

static int hex2int(QString str) {
    return std::stoi(str.toStdString(),nullptr,16);
}

static QString int2hex(uint32_t a, uint8_t l) {
    return QString::number(a, 16).rightJustified(l, '0');
}

void MainWindow::raiseDebugger() {
    // make sure we are set on the debug window, just in case
    if (dock_debugger) {
        dock_debugger->setVisible(true);
        dock_debugger->raise();
    }
    ui->tabWidget->setCurrentWidget(ui->tabDebugger);

    populateDebugWindow();
    setDebuggerState(true);
}

void MainWindow::updateDebuggerChanges() {
  /* Update all the changes in the core */
  if (debugger_on == false) {
      cpu.registers.AF = (uint16_t)hex2int(ui->afregView->text());
      cpu.registers.HL = (uint32_t)hex2int(ui->hlregView->text());
      cpu.registers.DE = (uint32_t)hex2int(ui->deregView->text());
      cpu.registers.BC = (uint32_t)hex2int(ui->bcregView->text());
      cpu.registers.IX = (uint32_t)hex2int(ui->ixregView->text());
      cpu.registers.IY = (uint32_t)hex2int(ui->iyregView->text());

      cpu.registers._AF = (uint16_t)hex2int(ui->af_regView->text());
      cpu.registers._HL = (uint32_t)hex2int(ui->hl_regView->text());
      cpu.registers._DE = (uint32_t)hex2int(ui->de_regView->text());
      cpu.registers._BC = (uint32_t)hex2int(ui->bc_regView->text());

      cpu.registers.SPL = (uint32_t)hex2int(ui->splregView->text());
      cpu.registers.SPS = (uint16_t)hex2int(ui->spsregView->text());

      cpu.registers.MBASE = (uint8_t)hex2int(ui->mbregView->text());
      cpu.registers.I = (uint16_t)hex2int(ui->iregView->text());
      cpu.registers.R = (uint8_t)hex2int(ui->rregView->text());;
      cpu.IM = (uint8_t)hex2int(ui->imregView->text());

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

      cpu_flush((uint32_t)hex2int(ui->pcregView->text()), ui->checkADL->isChecked());

      backlight.brightness = (uint8_t)ui->brightnessSlider->value();
  }
}

void MainWindow::setDebuggerState(bool state) {
    QPixmap pix;
    QIcon icon;

    debugger_on = state;

    if (debugger_on) {
        ui->buttonRun->setText("Run");
        pix.load(":/icons/resources/icons/run.png");
    } else {
        ui->buttonRun->setText("Stop");
        pix.load(":/icons/resources/icons/stop.png");
        ui->portChangeView->clear();
        ui->breakChangeView->clear();
    }
    setReceiveState(false);
    icon.addPixmap(pix);
    ui->buttonRun->setIcon(icon);
    ui->buttonRun->setIconSize(pix.size());

    ui->tabDebugging->setEnabled( debugger_on );
    ui->buttonBreakpoint->setEnabled( debugger_on );
    ui->buttonGoto->setEnabled( debugger_on );
    ui->buttonStep->setEnabled( debugger_on );
    ui->buttonStepOver->setEnabled( debugger_on );
    ui->buttonStepOut->setEnabled( debugger_on );
    ui->groupCPU->setEnabled( debugger_on );
    ui->groupFlags->setEnabled( debugger_on );
    ui->groupDisplay->setEnabled( debugger_on );
    ui->groupRegisters->setEnabled( debugger_on );
    ui->groupInterrupts->setEnabled( debugger_on );
    ui->groupStack->setEnabled( debugger_on );
    ui->groupFlash->setEnabled( debugger_on );
    ui->groupRAM->setEnabled( debugger_on );
    ui->groupMem->setEnabled( debugger_on );

    ui->buttonSend->setEnabled( !debugger_on );
    ui->buttonRefreshList->setEnabled( !debugger_on );
    ui->emuVarView->setEnabled( !debugger_on );
    ui->buttonReceiveFiles->setEnabled( !debugger_on && in_recieving_mode);

    if (!debugger_on) {
        updateDebuggerChanges();
        if (in_recieving_mode) {
            in_recieving_mode = false;
            refreshVariableList();
        }
    }
}

void MainWindow::changeDebuggerState() {
    if (emu.rom.empty()) {
        return;
    }

    debugger_on = !debugger_on;
    if (!debugger_on) {
        setDebuggerState(false);
    }
    emit debuggerChangedState( debugger_on );
}

void MainWindow::populateDebugWindow() {
    ui->portRequest->clear();
    ui->afregView->setText(int2hex(cpu.registers.AF, 4));
    ui->hlregView->setText(int2hex(cpu.registers.HL, 6));
    ui->deregView->setText(int2hex(cpu.registers.DE, 6));
    ui->bcregView->setText(int2hex(cpu.registers.BC, 6));
    ui->ixregView->setText(int2hex(cpu.registers.IX, 6));
    ui->iyregView->setText(int2hex(cpu.registers.IY, 6));

    ui->af_regView->setText(int2hex(cpu.registers._AF, 4));
    ui->hl_regView->setText(int2hex(cpu.registers._HL, 6));
    ui->de_regView->setText(int2hex(cpu.registers._DE, 6));
    ui->bc_regView->setText(int2hex(cpu.registers._BC, 6));

    ui->spsregView->setText(int2hex(cpu.registers.SPS, 4));
    ui->splregView->setText(int2hex(cpu.registers.SPL, 6));

    ui->pcregView->setText(int2hex(cpu.registers.PC, 6));
    ui->mbregView->setText(int2hex(cpu.registers.MBASE, 2));
    ui->iregView->setText(int2hex(cpu.registers.I,4));
    ui->rregView->setText(int2hex(cpu.registers.R,2));
    ui->imregView->setText(int2hex(cpu.IM,1));

    ui->freqView->setText(QString::number(sched.clock_rates[CLOCK_CPU]));

    ui->check3->setChecked(cpu.registers.flags._3);
    ui->check5->setChecked(cpu.registers.flags._5);
    ui->checkZ->setChecked(cpu.registers.flags.Z);
    ui->checkC->setChecked(cpu.registers.flags.C);
    ui->checkHC->setChecked(cpu.registers.flags.H);
    ui->checkPV->setChecked(cpu.registers.flags.PV);
    ui->checkN->setChecked(cpu.registers.flags.N);
    ui->checkS->setChecked(cpu.registers.flags.S);

    ui->checkPowered->setChecked(lcd.control & 0x800);
    ui->checkADL->setChecked(cpu.ADL);
    ui->checkMADL->setChecked(cpu.MADL);
    ui->checkHalted->setChecked(cpu.halted);
    ui->checkIEF1->setChecked(cpu.IEF1);
    ui->checkIEF2->setChecked(cpu.IEF2);

    ui->brightnessSlider->setValue(backlight.brightness);

    for(int i=0; i<ui->portView->rowCount(); ++i) {
        updatePortData(i);
    }

    updateStackView();
    ramUpdate();
    flashUpdate();
    memUpdate();
}

void MainWindow::updateDisasmView(const int sentBase, const bool fromPane) {
    address_pane = sentBase;
    from_pane = fromPane;
    disasm_offset_set = false;
    disasm.adl = ui->checkADL->isChecked();
    disasm.base_address = -1;
    disasm.new_address = address_pane - ((fromPane) ? 0x80 : 0);
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

    QFont disasmFont = ui->disassemblyView->font();
    disasmFont.setPointSize(ui->textSizeSlider->value());
    ui->disassemblyView->setFont(disasmFont);

    ui->disassemblyView->setTextCursor(disasm_offset);
    ui->disassemblyView->centerCursor();
}

void MainWindow::portMonitorCheckboxToggled(QTableWidgetItem * item) {
    auto col = item->column();
    auto row = item->row();
    uint8_t value = DBG_NO_HANDLE;

    uint16_t port = (uint16_t)ui->portView->item(row, 0)->text().toInt(nullptr,16);

    // Handle R_Break, W_Break, and Freeze
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
        if (item->checkState() != Qt::Checked) {
            mem.debug.ports[port] &= ~value;
        } else {
            mem.debug.ports[port] |= value;
        }
    }
}

void MainWindow::pollPort() {
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
    port = (uint16_t)hex2int(QString::fromStdString(s));
    read = (uint8_t)debug_port_read_byte(port);

    QString port_string = int2hex(port,4).toUpper();

    /* return if port is already set */
    for (int i=0; i<currentRow; ++i) {
        if (ui->portView->item(i, 0)->text() == port_string) {
            return;
        }
    }

    ui->portView->setRowCount(currentRow + 1);

    QTableWidgetItem *port_range = new QTableWidgetItem(port_string.toUpper());
    QTableWidgetItem *port_data = new QTableWidgetItem(int2hex(read, 2));
    QTableWidgetItem *port_rBreak = new QTableWidgetItem();
    QTableWidgetItem *port_wBreak = new QTableWidgetItem();
    QTableWidgetItem *port_freeze = new QTableWidgetItem();

    port_rBreak->setCheckState(Qt::Unchecked);
    port_wBreak->setCheckState(Qt::Unchecked);
    port_freeze->setCheckState(Qt::Unchecked);

    ui->portView->setItem(currentRow, 0, port_range);
    ui->portView->setItem(currentRow, 1, port_data);
    ui->portView->setItem(currentRow, 2, port_rBreak);
    ui->portView->setItem(currentRow, 3, port_wBreak);
    ui->portView->setItem(currentRow, 4, port_freeze);

    ui->portRequest->clear();
}

void MainWindow::deletePort() {
    if(!ui->portView->rowCount() || !ui->portView->currentIndex().isValid()) {
        return;
    }

    const int currentRow = ui->portView->currentRow();

    uint16_t port = (uint16_t)ui->portView->item(currentRow, 0)->text().toInt(nullptr,16);
    mem.debug.ports[port] = DBG_NO_HANDLE;

    ui->portView->removeRow(currentRow);
}

void MainWindow::breakpointCheckboxToggled(QTableWidgetItem * item) {
    auto col = item->column();
    auto row = item->row();
    uint8_t value = DBG_NO_HANDLE;

    uint32_t address = (uint32_t)ui->breakpointView->item(row, 0)->text().toInt(nullptr,16)&0xFFFFFF;

    // Handle R_Break, W_Break, and E_Break
    if (col > 0)
    {
        if (col == 1) { // Break on read
            value = DBG_READ_BREAKPOINT;
        }
        if (col == 2) { // Break on write
            value = DBG_WRITE_BREAKPOINT;
        }
        if (col == 3) { // Break on execution
            value = DBG_EXEC_BREAKPOINT;
        }
        if (item->checkState() != Qt::Checked) {
            mem.debug.block[address] &= ~value;
        } else {
            mem.debug.block[address] |= value;
        }
    }

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

    address = (uint32_t)hex2int(QString::fromStdString(s));

    QString address_string = int2hex(address,6).toUpper();

    /* return if address is already set */
    for (int i=0; i<currentRow; ++i) {
        if (ui->breakpointView->item(i, 0)->text() == address_string) {
            ui->breakpointView->selectRow(i);
            return false;
        }
    }

    ui->breakpointView->setRowCount(currentRow + 1);

    QTableWidgetItem *iaddress = new QTableWidgetItem(address_string.toUpper());
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
    return true;
}

void MainWindow::deleteBreakpoint() {
    if(!ui->breakpointView->rowCount() || !ui->breakpointView->currentIndex().isValid()) {
        return;
    }

    const int currentRow = ui->breakpointView->currentRow();

    uint32_t address = (uint32_t)ui->breakpointView->item(currentRow, 0)->text().toInt(nullptr,16);
    mem.debug.block[address] = DBG_NO_HANDLE;

    ui->breakpointView->removeRow(currentRow);
}

void MainWindow::processDebugCommand(int reason, uint32_t input) {
    int row = 0;
    bool ok;

    if (reason == DBG_STEP || reason == DBG_USER) {
        if (reason == DBG_STEP) { ui->tabDebugging->setCurrentIndex(0); }
        updateDisasmView(cpu.registers.PC, true);
    }

    // We hit a normal breakpoint; raise the correct entry in the port monitor table
    if (reason == HIT_READ_BREAKPOINT || reason == HIT_WRITE_BREAKPOINT || reason == HIT_EXEC_BREAKPOINT) {
        ui->tabDebugging->setCurrentIndex(0);

        // find the correct entry
        while( (uint32_t)ui->breakpointView->item(row++, 0)->text().toInt(&ok,16) != input );
        row--;

        ui->breakChangeView->setText("Address "+ui->breakpointView->item(row, 0)->text()+" "+((reason == HIT_READ_BREAKPOINT) ? "Read" : (reason == HIT_WRITE_BREAKPOINT) ? "Write" : "Executed"));
        ui->breakpointView->selectRow(row);

        updateDisasmView(input, true);
    }

    // We hit a port read or write; raise the correct entry in the port monitor table
    if (reason == HIT_PORT_READ_BREAKPOINT || reason == HIT_PORT_WRITE_BREAKPOINT) {
        ui->tabDebugging->setCurrentIndex(1);
        // find the correct entry
        while( (uint32_t)ui->portView->item(row++, 0)->text().toInt(&ok,16) != input );
        row--;

        ui->portChangeView->setText("Port "+ui->portView->item(row, 0)->text()+" "+((reason == HIT_PORT_READ_BREAKPOINT) ? "Read" : "Write"));
        ui->portView->selectRow(row);
    }
}

void MainWindow::updatePortData(int currentRow) {
    uint16_t port = (uint16_t)ui->portView->item(currentRow, 0)->text().toInt(nullptr,16);
    uint8_t read = (uint8_t)debug_port_read_byte(port);

    ui->portView->item(currentRow, 1)->setText(int2hex(read,2).toUpper());
}

void MainWindow::resetCalculator() {
    if (emu.stop()) {
        emu.start();
    } else {
        qDebug("Reset Failed.");
    }
}

void MainWindow::updateStackView() {
    ui->stackView->clear();

    QString formattedLine;

    for(int i=0; i<30; i+=3) {
       formattedLine = QString("<pre><b><font color='#444'>%1</font></b> %2</pre>")
                                .arg(int2hex(cpu.registers.SPL+i, 6).toUpper(),
                                     int2hex(memory_read_byte(cpu.registers.SPL+i) | memory_read_byte(cpu.registers.SPL+1+i)<<8 | memory_read_byte(cpu.registers.SPL+2+i)<<16,6).toUpper());
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
                                    int2hex(disasm.base_address, 6).toUpper(),
                                    label ? QString::fromStdString(*label) + ":" : ui->checkDataCol->isChecked() ? QString::fromStdString(disasm.instruction.data).leftJustified(12, ' ') : "",
                                    QString::fromStdString(disasm.instruction.opcode),
                                    QString::fromStdString(disasm.instruction.mode_suffix),
                                    instructionArgsHighlighted);

    ui->disassemblyView->appendHtml(formattedLine);

    if (address_pane == disasm.base_address) {
        disasm_offset_set = true;
        disasm_offset = ui->disassemblyView->textCursor();
        disasm_offset.movePosition(QTextCursor::StartOfLine);
    } else if (disasm_offset_set == false && address_pane <= disasm.base_address+7) {
        disasm_offset_set = true;
        disasm_offset = ui->disassemblyView->textCursor();
        disasm_offset.movePosition(QTextCursor::StartOfLine);
    }

    if (disasmHighlight.hit_pc == true) {
        ui->disassemblyView->addHighlight(QColor(Qt::red).lighter(160));
    }
}

void MainWindow::setPCaddress(const QPoint& posa) {
    QString set_pc = "Set PC to this address";
    QString toggle_break = "Toggle breakpoint";
    QPoint globalPos = ui->disassemblyView->mapToGlobal(posa);

    QMenu contextMenu;
    contextMenu.addAction(set_pc);
    contextMenu.addAction(toggle_break);

    QAction* selectedItem = contextMenu.exec(globalPos);
    if (selectedItem) {
        if (selectedItem->text() == set_pc) {
            ui->pcregView->setText(ui->disassemblyView->getSelectedAddress());
            cpu_flush((uint32_t)hex2int(ui->pcregView->text()), cpu.ADL);
            updateDisasmView(cpu.registers.PC, true);
        } else  if (selectedItem->text() == toggle_break) {
            breakpointPressed();
        }
    }
}

void MainWindow::stepPressed() {
    // Since we are just stepping, there's no point in disasbling the GUI
    debugger_on = false;
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

void MainWindow::breakpointPressed() {
    bool ok;
    QString address = ui->disassemblyView->getSelectedAddress();

    ui->breakRequest->setText(address);
    if(!addBreakpoint()) {
        deleteBreakpoint();
    }

    ui->breakRequest->clear();

    updateDisasmView(address.toInt(&ok, 16), true);
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
    int start;
    int line = 0;

    if (locked) {
        start = (int)ui->memEdit->addressOffset();
        line = ui->memEdit->getLine();
    } else {
        start = cpu.registers.PC-0x1000;
    }

    if (start < 0) { start = 0; }
    int end = start+0x2000;
    if (end > 0xFFFFFF) { end = 0xFFFFFF; }

    mem_hex_size = end-start;

    for (int i=start; i<end; i++) {
        mem_data.append(memory_read_byte(i));
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
    QString search_string = QInputDialog::getText(this, tr("Search"),
                                                  tr("Input Hexadecimal Search String:"), QLineEdit::Normal,
                                                  "", &ok).toUpper();
    editor->setFocus();
    if(!ok || (search_string.length() & 1)) {
        return;
    }
    QByteArray string_int;
    for (int i=0; i<search_string.length(); i+=2) {
        QString a = search_string.at(i);
        a.append(search_string.at(i+1));
        string_int.append(a.toInt(&ok, 16));
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

    mem_hex_size = end-start;

    for (int i=start; i<end; i++) {
        mem_data.append(memory_read_byte(i));
    }

    ui->memEdit->setData(mem_data);
    ui->memEdit->setAddressOffset(start);
    ui->memEdit->setCursorPosition((int_address-start)<<1);
    ui->memEdit->ensureVisible();
}

void MainWindow::syncHexView(int posa, QHexEdit *hex_view) {
    populateDebugWindow();
    updateDisasmView(address_pane, from_pane);
    hex_view->setFocus();
    hex_view->setCursorPosition(posa);
}

void MainWindow::flashSyncPressed() {
    qint64 posa = ui->flashEdit->cursorPosition();
    memcpy(mem.flash.block, (uint8_t*)ui->flashEdit->data().data(), flash_size);
    syncHexView(posa, ui->flashEdit);
}

void MainWindow::ramSyncPressed() {
    qint64 posa = ui->ramEdit->cursorPosition();
    memcpy(mem.ram.block, (uint8_t*)ui->ramEdit->data().data(), ram_size);
    syncHexView(posa, ui->ramEdit);
}

void MainWindow::memSyncPressed() {
    int start = ui->memEdit->addressOffset();
    qint64 posa = ui->memEdit->cursorPosition();

    for (int i = 0; i<mem_hex_size; i++) {
        memory_force_write_byte(i+start, ui->memEdit->dataAt(i, 1).at(0));
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
    dialog.setDirectory(current_dir);

    QStringList extFilters;
    extFilters << tr("ASM equates file (*.inc)")
               << tr("Symbol Table File (*.lab)");
    dialog.setNameFilters(extFilters);

    good = dialog.exec();
    current_dir = dialog.directory();

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
                uint32_t address = (uint32_t)matches.capturedRef(2).toUInt(0, 16);
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
