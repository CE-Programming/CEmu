#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aboutwindow.h"
#include "settings.h"
#include "emuthread.h"
#include "core/debug.h"

#include <QFileDialog>
#include <QMessageBox>

char tmpBuf[20] = {0};

MainWindow::MainWindow(QWidget *p) : QMainWindow(p), ui(new Ui::MainWindow)
{
    ui->setupUi(this);              // setup the UI

    // Emulator -> GUI
    connect(&emu, SIGNAL(consoleStr(QString)), this, SLOT(consoleStr(QString))); //Not queued connection as it may cause a hang
    connect(&emu, SIGNAL(debuggerEntered(bool)), this, SLOT(raiseDebugger()));

    // GUI -> Emulator
    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(currentChangedSlot(int)));

    // Console actions
    connect(ui->buttonConsoleclear, SIGNAL(clicked()), this, SLOT(clearConsole()));

    // Toolbar Actions
    connect(ui->actionSetup, SIGNAL(triggered()), this, SLOT(runSetup()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAbout()));
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(actionExit()));
    connect(ui->actionScreenshot, SIGNAL(triggered()), this, SLOT(screenshot()));

    // Other GUI actinos
    connect(ui->buttonScreenshot, SIGNAL(clicked()), this, SLOT(screenshot()));

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
    // This seems really silly to me, but it works
    if("tabDebugger" == ui->tabWidget->currentWidget()->objectName()) {
        emu.enterDebugger();
    }
}

void MainWindow::raiseDebugger()
{
    // make sure we are set on the debug window, just in case
    ui->tabWidget->setCurrentWidget(ui->tabDebugger);

    ui->scrollArea1->setEnabled( false );
    ui->scrollArea2->setEnabled( false );
    ui->tabDebugging->setEnabled( false );
    ui->buttonBreakpoint->setEnabled( false );
    ui->buttonGoto->setEnabled( false );
    ui->buttonStep->setEnabled( false );
    ui->buttonStepOver->setEnabled( false );

    // populate the information on the degbug window
    this->populateDebugWindow();
}

static QString int2hex(uint32_t a, uint8_t l) {
    ::sprintf(tmpBuf, "%0*X", l, a);
    return QString(tmpBuf);
}

void MainWindow::populateDebugWindow()
{
  eZ80registers_t *CEreg = &emu.asic_ptr->cpu->registers;
  asic_state_t *CEasic = emu.asic_ptr;
  eZ80cpu_t *CEcpu = emu.asic_ptr->cpu;

  ui->afregView->setText( int2hex(CEreg->AF, 4) );
  ui->hlregView->setText( int2hex(CEreg->HL, 6) );
  ui->deregView->setText( int2hex(CEreg->DE, 6) );
  ui->bcregView->setText( int2hex(CEreg->BC, 6) );
  ui->ixregView->setText( int2hex(CEreg->IX, 6) );
  ui->iyregView->setText( int2hex(CEreg->IY, 6) );

  ui->af_regView->setText( int2hex(CEreg->_AF, 4) );
  ui->hl_regView->setText( int2hex(CEreg->_HL, 6) );
  ui->de_regView->setText( int2hex(CEreg->_DE, 6) );
  ui->bc_regView->setText( int2hex(CEreg->_BC, 6) );

  ui->mbregView->setText( int2hex(CEreg->MBASE, 2) );
  ui->pcregView->setText( int2hex(CEreg->PC, 6) );
  ui->spsregView->setText(  int2hex(CEreg->SPS, 4) );
  ui->splregView->setText(  int2hex(CEreg->SPL, 6) );

//  ui->checkZ->setChecked( CEreg->flags.Z );
//  ui->checkC->setChecked( CEreg->flags.C );
//  ui->checkS->setChecked( CEreg->flags.S );
//  ui->checkPV->setChecked( CEreg->flags.PV );
//  ui->checkHC->setChecked( CEreg->flags.H );
//  ui->check3->setChecked( CEreg->flags._3 );
//  ui->checkZ->setChecked( CEreg->flags._5 );

//  ui->checkIEF1->setChecked( CEcpu->IEF1 );
//  ui->checkIEF2->setChecked( CEcpu->IEF2 );

//  ui->iregView->setText( int2hex(CEreg->I, 4) );
//  ui->rregView->setText( int2hex(CEreg->R, 2) );
}

void MainWindow::showStatusMsg(QString str)
{
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
