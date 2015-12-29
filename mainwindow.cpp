#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "emuthread.h"
#include "qmlbridge.h"
#include "qtframebuffer.h"
#include "qtkeypadbridge.h"

#include "core/gif.h"
#include "core/schedule.h"

#include <QFileDialog>
#include <QFont>
#include <QMessageBox>
#include <QPixmap>
#include <QQuickWidget>
#include <QDockWidget>
#include <QShortcut>
#include <iostream>

static char tmpBuf[20] = {0};
static const constexpr int WindowStateVersion = 0;

MainWindow::MainWindow(QWidget *p) : QMainWindow(p), ui(new Ui::MainWindow) {
    // Setup the UI
    ui->setupUi(this);

    // Register QtKeypadBridge for the virtual keyboard functionality
    ui->lcdWidget->installEventFilter(&qt_keypad_bridge);
    ui->keypadWidget->installEventFilter(&qt_keypad_bridge);

    ui->keypadWidget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);

    // Emulator -> GUI
    connect(&emu, &EmuThread::consoleStr, this, &MainWindow::consoleStr); // Not queued

    // GUI -> Emulator
    connect(ui->buttonRun, &QPushButton::clicked, this, &MainWindow::changeDebuggerState);

    // Debugger
    connect(this, &MainWindow::debuggerChangedState, &emu, &EmuThread::setDebugMode);
    connect(&emu, &EmuThread::debuggerEntered, this, &MainWindow::raiseDebugger);
    connect(ui->buttonPoll, &QPushButton::clicked, this, &MainWindow::pollPort);
    connect(ui->portRequest, &QLineEdit::returnPressed, this, &MainWindow::pollPort);
    connect(ui->buttonPortClear, &QPushButton::clicked, this, &MainWindow::clearPortConsole);

    // Console actions
    connect(ui->buttonConsoleclear, &QPushButton::clicked, this, &MainWindow::clearConsole);

    // Toolbar Actions
    connect(ui->actionSetup, &QAction::triggered, this, &MainWindow::runSetup);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::actionExit);
    connect(ui->actionScreenshot, &QAction::triggered, this, &MainWindow::screenshot);
    connect(ui->actionRecord_GIF, &QAction::triggered, this, &MainWindow::recordGIF);
    connect(ui->buttonGIF, &QPushButton::clicked, this, &MainWindow::recordGIF);

    // Other GUI actions
    connect(ui->buttonScreenshot, &QPushButton::clicked, this, &MainWindow::screenshot);
    connect(ui->buttonRunSetup, &QPushButton::clicked, this, &MainWindow::runSetup);
    connect(ui->refreshSlider, &QSlider::valueChanged, this, &MainWindow::changeLCDRefresh);

    //Set up monospace fonts
    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->console->setFont(monospace);
    ui->portView->setFont(monospace);

    setUIMode(true);

    debugger_on = false;

#ifdef Q_OS_ANDROID
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    settings = new QSettings(path + QStringLiteral("/cemu.ini"), QSettings::IniFormat);
#else
    settings = new QSettings();
#endif

    restoreGeometry(settings->value(QStringLiteral("windowGeometry")).toByteArray());
    restoreState(settings->value(QStringLiteral("windowState")).toByteArray(), WindowStateVersion);
    changeLCDRefresh(settings->value(QStringLiteral("refreshRate"), QVariant(60)).toInt());

    emu.rom = settings->value(QStringLiteral("romImage")).toString().toStdString();

    if (emu.rom.empty()) {
       runSetup();
    } else {
       emu.start();
    }

    ui->rompathView->setText(QString(emu.rom.c_str()));

    ui->lcdWidget->setFocus();
}

// window destructor
MainWindow::~MainWindow() {
    settings->setValue(QStringLiteral("windowState"), saveState(WindowStateVersion));
    settings->setValue(QStringLiteral("windowGeometry"), saveGeometry());

    delete settings;
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *e) {
    qDebug("Terminating emulator thread...");

    if (emu.stop()) {
        qDebug("Successful!");
    } else {
        qDebug("Failed.");
    }

    QMainWindow::closeEvent(e);
}

void MainWindow::consoleStr(QString str) {
    ui->console->moveCursor(QTextCursor::End);
    ui->console->insertPlainText(str);
}

void MainWindow::runSetup(void) {
    RomSelection romSelection;
    romSelection.show();
    romSelection.exec();

    if (!romImagePath.empty()) {
        settings->setValue(QStringLiteral("romImage"),QVariant(romImagePath.c_str()));
        emu.stop();
        emu.rom = romImagePath;
        ui->rompathView->setText(romImagePath.c_str());
        emu.start();
    }
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

void MainWindow::screenshot(void) {
    QImage image = renderFramebuffer();

    QString filename = QFileDialog::getSaveFileName(this, tr("Save Screenshot"), QString(), tr("PNG images (*.png)"));
    if (filename.isEmpty())
        return;

    if (!image.save(filename, "PNG"))
        QMessageBox::critical(this, tr("Screenshot failed"), tr("Failed to save screenshot!"));
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

void MainWindow::clearPortConsole(void) {
    ui->portView->clear();
}

void MainWindow::showAbout() {
    #define STRINGIFYMAGIC(x) #x
    #define STRINGIFY(x) STRINGIFYMAGIC(x)
    QMessageBox about_box(this);
    about_box.addButton(QMessageBox::Ok);
    about_box.setIconPixmap(QPixmap(":/icons/resources/icons/icon.png"));
    about_box.setWindowTitle(tr("About CEmu"));
    about_box.setText(tr("<h3>CEmu %1</h3>"
                         "<a href='https://github.com/nspire-emus/firebird'>On GitHub</a><br>"
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
                         "Many thanks to the <a href='https://github.com/KnightOS/z80e'>z80e</a> and <a href='https://github.com/nspire-emus/firebird'>Firebird</a> projects<br>"
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

void MainWindow::actionExit(void) {
    close();
}


/* ================================================ */
/* Debugger Things                                  */
/* ================================================ */

static int hex2int(QString str) {
    return std::stoi(str.toStdString(),nullptr,16);
}

static QString int2hex(uint32_t a, uint8_t l) {
    ::sprintf(tmpBuf, "%0*X", l, a);
    return QString(tmpBuf);
}

void MainWindow::raiseDebugger() {
    // make sure we are set on the debug window, just in case
    if(dock_debugger)
    {
        dock_debugger->setVisible(true);
        dock_debugger->raise();
    }
    ui->tabWidget->setCurrentWidget(ui->tabDebugger);

    /* Set to false because we change it immediately with changeDebuggerState */
    debugger_on = false;
    changeDebuggerState();
    populateDebugWindow();
}

void MainWindow::changeDebuggerState() {
    QPixmap pix;
    QIcon icon;

    debugger_on = !debugger_on;

    if(debugger_on) {
        ui->buttonRun->setText("Run");
        pix.load(":/icons/resources/icons/run.png");
    } else {
        ui->buttonRun->setText("Stop");
        pix.load(":/icons/resources/icons/stop.png");
    }
    icon.addPixmap(pix);
    ui->buttonRun->setIcon(icon);
    ui->buttonRun->setIconSize(pix.size());

    ui->tabDebugging->setEnabled( debugger_on );
    ui->buttonBreakpoint->setEnabled( debugger_on );
    ui->buttonGoto->setEnabled( debugger_on );
    ui->buttonStep->setEnabled( debugger_on );
    ui->buttonStepOver->setEnabled( debugger_on );
    ui->groupCPU->setEnabled( debugger_on );
    ui->groupFlags->setEnabled( debugger_on );
    ui->groupDisplay->setEnabled( debugger_on );
    ui->groupRegisters->setEnabled( debugger_on );
    ui->groupInterrupts->setEnabled( debugger_on );

    /* Update all the changes in the core */
    if (debugger_on == false) {
        cpu.registers.AF = (uint16_t)hex2int(ui->afregView->text());
        cpu.registers.HL = (uint32_t)hex2int(ui->hlregView->text());
        cpu.registers.DE = (uint32_t)hex2int(ui->deregView->text());
        cpu.registers.BC = (uint32_t)hex2int(ui->bcregView->text());
        cpu.registers.IX = (uint32_t)hex2int(ui->ixregView->text());
        cpu.registers.IY = (uint32_t)hex2int(ui->ixregView->text());

        cpu.registers._AF = (uint16_t)hex2int(ui->af_regView->text());
        cpu.registers._HL = (uint32_t)hex2int(ui->hl_regView->text());
        cpu.registers._DE = (uint32_t)hex2int(ui->de_regView->text());
        cpu.registers._BC = (uint32_t)hex2int(ui->bc_regView->text());

        cpu.registers.SPL = (uint32_t)hex2int(ui->splregView->text());
        cpu.registers.SPS = (uint16_t)hex2int(ui->spsregView->text());

        cpu.registers.PC = (uint32_t)hex2int(ui->pcregView->text());
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
        cpu.IEF1 = ui->checkIEF1->isChecked();
        cpu.IEF2 = ui->checkIEF2->isChecked();

        backlight.brightness = (uint8_t)ui->brightnessSlider->value();
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

    ui->check3->setChecked(cpu.registers.flags._3);
    ui->check5->setChecked(cpu.registers.flags._5);
    ui->checkZ->setChecked(cpu.registers.flags.Z);
    ui->checkC->setChecked(cpu.registers.flags.C);
    ui->checkHC->setChecked(cpu.registers.flags.H);
    ui->checkPV->setChecked(cpu.registers.flags.PV);
    ui->checkN->setChecked(cpu.registers.flags.N);
    ui->checkS->setChecked(cpu.registers.flags.S);

    ui->checkPowered->setChecked(lcd.control & 0x800);
    ui->checkHalted->setChecked(cpu.halted);
    ui->checkIEF1->setChecked(cpu.IEF1);
    ui->checkIEF2->setChecked(cpu.IEF2);

    ui->brightnessSlider->setValue(backlight.brightness);
}

void MainWindow::pollPort() {
    if (ui->portRequest->text().isEmpty()) {
        return;
    }

    std::string s = ui->portRequest->text().toUpper().toStdString();
    if (s.find_first_not_of("0123456789ABCDEF") != std::string::npos) {
        return;
    }

    QString port = QString::fromStdString(s);
    QString read = int2hex((uint32_t)port_read_byte((uint16_t)hex2int(port)),2);

    ui->portView->moveCursor(QTextCursor::End);
    ui->portView->insertPlainText("Port: "+port+" -> "+read+"\n");
    ui->portRequest->clear();
}

void MainWindow::changeLCDRefresh(int value) {
    settings->setValue(QStringLiteral("refreshRate"),QVariant(value));
    ui->refreshLabel->setText(QString::fromStdString(std::to_string(value))+" FPS");
    ui->refreshSlider->setValue(value);
    ui->lcdWidget->refreshRate(value);
}
