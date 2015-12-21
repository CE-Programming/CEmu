#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aboutwindow.h"
#include "settings.h"
#include "emuthread.h"
#include "qmlbridge.h"
#include "qtframebuffer.h"
#include "qtkeypadbridge.h"

#include "core/debug.h"

#include <QFileDialog>
#include <QMessageBox>

char tmpBuf[20] = {0};

MainWindow::MainWindow(QWidget *p) : QMainWindow(p), ui(new Ui::MainWindow)
{
    ui->setupUi(this);              // setup the UI

    // Register QtKeypadBridge for the virtual keyboard functionality
    ui->keypadWidget->installEventFilter(&qt_keypad_bridge);
    ui->widget->installEventFilter(&qt_keypad_bridge);

    // Emulator -> GUI
    connect(&emu, SIGNAL(consoleStr(QString)), this, SLOT(consoleStr(QString))); //Not queued connection as it may cause a hang

    // GUI -> Emulator
    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(currentChangedSlot(int)));
    connect(ui->actionSetup, SIGNAL(triggered()), this, SLOT(runSetup()));
    connect(ui->buttonRun, SIGNAL(clicked(bool)), this, SLOT(raiseDebugger()));

    // Console actions
    connect(ui->buttonConsoleclear, SIGNAL(clicked()), this, SLOT(clearConsole()));

    // Toolbar Actions
    connect(ui->actionSetup, SIGNAL(triggered()), this, SLOT(runSetup()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAbout()));
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(actionExit()));
    connect(ui->actionScreenshot, SIGNAL(triggered()), this, SLOT(screenshot()));

    // Other GUI actinos
    connect(ui->buttonScreenshot, SIGNAL(clicked()), this, SLOT(screenshot()));

    in_debugger = false;

    emu.rom = CEmuSettings::Instance()->getROMLocation().toStdString();
    if(emu.rom == "") {
        runSetup();
    } else {
        emu.start();
    }
}

// window destructor
MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::currentChangedSlot(int index) {

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
    ui->scrollArea1->setEnabled( in_debugger );
    ui->scrollArea2->setEnabled( in_debugger );
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
    RomSelection m;
    m.exec();

    emu.stop();
    QString rompath = CEmuSettings::Instance()->getROMLocation();
    emu.rom = rompath.toStdString();
    if(emu.rom == "") {
        this->close();
        return;
    }
    emu.start();
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

void MainWindow::clearConsole(void) {
    this->ui->console->clear();
    this->consoleStr("Console Cleared.\n");
}

void MainWindow::showAbout(void) {
    AboutWindow w;
    w.exec();
}

void MainWindow::actionExit(void) {
    this->close();
}

void MainWindow::on_pushButton_2_clicked()
{
    keypad.key_map[1] = 1;
}
