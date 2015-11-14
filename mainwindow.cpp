#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QLabel>
#include <iostream>

#include "aboutwindow.h"
#include "settings.h"

MainWindow::MainWindow(QWidget *p) : QMainWindow(p), ui(new Ui::MainWindow)
{
    ui->setupUi(this);              // setup the UI

    // Emulator -> GUI
    connect(&emu, SIGNAL(debugStr(QString)), this, SLOT(debugStr(QString))); //Not queued connection as it may cause a hang

    // GUI -> Emulator
    connect(ui->buttonReset, SIGNAL(clicked()), &emu, SLOT(test()));

    // Toolbar Actions
    connect(ui->actionSetup, SIGNAL(triggered()), this, SLOT(runSetup()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAbout()));
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(actionExit()));

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

void MainWindow::debugStr(QString str)
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

void MainWindow::showAbout(void) {
    AboutWindow w;
    w.exec();
}

void MainWindow::actionExit(void) {
    this->close();
}
