#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "emuthread.h"
#include "qmlbridge.h"
#include "qtframebuffer.h"
#include "qtkeypadbridge.h"

#include "core/debug.h"
#include "core/gif.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QQuickWidget>
#include <QDockWidget>
#include <QShortcut>
#include <iostream>

char tmpBuf[20] = {0};
static const constexpr int WindowStateVersion = 0;

MainWindow::MainWindow(QWidget *p) : QMainWindow(p), ui(new Ui::MainWindow)
{
    ui->setupUi(this);              // setup the UI

    // Register QtKeypadBridge for the virtual keyboard functionality
    this->installEventFilter(&qt_keypad_bridge);

    // Emulator -> GUI
    connect(&emu, SIGNAL(consoleStr(QString)), this, SLOT(consoleStr(QString))); //Not queued connection as it may cause a hang

    // GUI -> Emulator
    connect(ui->buttonRun, SIGNAL(clicked(bool)), this, SLOT(raiseDebugger()));

    // Console actions
    connect(ui->buttonConsoleclear, SIGNAL(clicked()), this, SLOT(clearConsole()));

    // Toolbar Actions
    connect(ui->actionSetup, SIGNAL(triggered()), this, SLOT(runSetup()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAbout()));
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(actionExit()));
    connect(ui->actionScreenshot, SIGNAL(triggered()), this, SLOT(screenshot()));
    connect(ui->actionRecord_GIF, SIGNAL(triggered()), this, SLOT(recordGIF()));
    connect(ui->buttonGIF, SIGNAL(clicked()), this, SLOT(recordGIF()));

    // Other GUI actinos
    connect(ui->buttonScreenshot, SIGNAL(clicked()), this, SLOT(screenshot()));

    setUIMode(true);

    in_debugger = false;

#ifdef Q_OS_ANDROID
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    settings = new QSettings(path + QStringLiteral("/cemu.ini"), QSettings::IniFormat);
#else
    settings = new QSettings();
#endif

    restoreGeometry(settings->value(QStringLiteral("windowGeometry")).toByteArray());
    restoreState(settings->value(QStringLiteral("windowState")).toByteArray(), WindowStateVersion);

    emu.rom = settings->value(QStringLiteral("romImage")).toByteArray().toStdString();

    if(emu.rom.empty()) {
       runSetup();
    } else {
       emu.start();
    }

    ui->lcdWidget->setFocus();
}

// window destructor
MainWindow::~MainWindow()
{
    settings->setValue(QStringLiteral("windowState"), saveState(WindowStateVersion));
    settings->setValue(QStringLiteral("windowGeometry"), saveGeometry());

    //delete settings;
    delete ui;
}

void MainWindow::raiseDebugger()
{
    QPixmap pix;
    QIcon icon;

    // make sure we are set on the debug window, just in case
    ui->tabWidget->setCurrentWidget(ui->tabDebugger);

    if(in_debugger == false) {
        ui->buttonRun->setText("Run");
        pix.load(":/icons/resources/icons/run.png");
    } else {
        ui->buttonRun->setText("Stop");
        pix.load(":/icons/resources/icons/stop.png");
    }
    icon.addPixmap(pix);
    ui->buttonRun->setIcon(icon);
    ui->buttonRun->setIconSize(pix.size());

    in_debugger = !in_debugger;
    ui->tabDebugging->setEnabled( in_debugger );
    ui->buttonBreakpoint->setEnabled( in_debugger );
    ui->buttonGoto->setEnabled( in_debugger );
    ui->buttonStep->setEnabled( in_debugger );
    ui->buttonStepOver->setEnabled( in_debugger );

    // populate the information on the degbug window
    if (in_debugger == true) { this->populateDebugWindow(); }
}

static QString int2hex(uint32_t a, uint8_t l) {
    ::sprintf(tmpBuf, "%0*X", l, a);
    return QString(tmpBuf);
}

void MainWindow::populateDebugWindow() {
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

    ui->check3->setChecked(cpu.registers.flags._3);
    ui->check5->setChecked(cpu.registers.flags._5);
    ui->checkZ->setChecked(cpu.registers.flags.Z);
    ui->checkC->setChecked(cpu.registers.flags.C);
    ui->checkHC->setChecked(cpu.registers.flags.H);
    ui->checkPV->setChecked(cpu.registers.flags.PV);
    ui->checkN->setChecked(cpu.registers.flags.N);
    ui->checkS->setChecked(cpu.registers.flags.S);

    ui->checkHalted->setChecked(cpu.halted);
    ui->checkIEF1->setChecked(cpu.IEF1);
    ui->checkIEF2->setChecked(cpu.IEF2);
}

void MainWindow::showStatusMsg(QString str) {
    status_label.setText(str);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    qDebug("Terminating emulator thread...");

    if(emu.stop()) {
        qDebug("Successful!");
    } else {
        qDebug("Failed.");
    }

    QMainWindow::closeEvent(e);
}

void MainWindow::consoleStr(QString str)
{
    ui->console->moveCursor(QTextCursor::End);
    ui->console->insertPlainText(str);
}

void MainWindow::runSetup(void) {
    romSelection.show();
    romSelection.exec();

    if (!romImagePath.empty()) {
        settings->setValue(QStringLiteral("romImage"),QVariant(romImagePath.c_str()));
        emu.rom = romImagePath;
        emu.stop();
        emu.start();
    }
}

void MainWindow::setUIMode(bool docks_enabled)
{
    // Already in this mode?
    if (docks_enabled == ui->tabWidget->isHidden())
        return;

    //settings->setValue(QStringLiteral("docksEnabled"), docks_enabled);

    // Enabling tabs needs a restart
    if(!docks_enabled)
    {
        QMessageBox::warning(this, trUtf8("Restart needed"), trUtf8("You need to restart firebird to enable the tab interface."));
        return;
    }

    // Create "Docks" menu to make closing and opening docks more intuitive
    QMenu *docks_menu = new QMenu(tr("Docks"), this);
    ui->menubar->insertMenu(ui->menuAbout->menuAction(), docks_menu);

    //Convert the tabs into QDockWidgets
    QDockWidget *last_dock = nullptr;
    while(ui->tabWidget->count())
    {
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
    //ui->uiDocks->setChecked(docks_enabled);
}

void MainWindow::screenshot(void)
{
    QImage image = renderFramebuffer();

    QString filename = QFileDialog::getSaveFileName(this, tr("Save Screenshot"), QString(), tr("PNG images (*.png)"));
    if(filename.isEmpty())
        return;

    if(!image.save(filename, "PNG"))
        QMessageBox::critical(this, tr("Screenshot failed"), tr("Failed to save screenshot!"));
}

void MainWindow::recordGIF()
{
    static QString path;

    if(path.isEmpty())
    {
        path = QStringLiteral("c:\\cemu_tmp.gif");

        gif_start_recording(path.toStdString().c_str(), 3);
    }
    else
    {
        if(gif_stop_recording())
        {
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

    ui->actionRecord_GIF->setChecked(path.isEmpty());
}

void MainWindow::clearConsole(void) {
    this->ui->console->clear();
    this->consoleStr("Console Cleared.\n");
}

void MainWindow::showAbout()
{
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
    this->close();
}
