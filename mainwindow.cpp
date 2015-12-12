#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QLabel>
#include <iostream>
#include <QDockWidget>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

#include "aboutwindow.h"
#include "settings.h"
#include "emuthread.h"
#include "core/debug.h"

MainWindow::MainWindow(QWidget *p) : QMainWindow(p), ui(new Ui::MainWindow)
{
    ui->setupUi(this);              // setup the UI

    // Emulator -> GUI
    connect(&emu, SIGNAL(consoleStr(QString)), this, SLOT(consoleStr(QString))); //Not queued connection as it may cause a hang
    connect(&emu, SIGNAL(debuggerEntered()), this, SLOT(raiseDebugger()));

    // GUI -> Emulator

    // Console actions
    connect(ui->buttonConsoleclear, SIGNAL(clicked()), this, SLOT(clearConsole()));

    // Toolbar Actions
    connect(ui->actionSetup, SIGNAL(triggered()), this, SLOT(runSetup()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAbout()));
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(actionExit()));

    // Debugger GUI
    connect(ui->buttonRun, SIGNAL(clicked()), this, SLOT(checkDebuggerState()));

    this->initDebugger();

    emu.rom = CEmuSettings::Instance()->getROMLocation().toStdString();
    if(emu.rom == "") {
        this->runSetup();
    } else {
        emu.start();
    }
}

// window destructor
MainWindow::~MainWindow()
{
    delete ui;
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
