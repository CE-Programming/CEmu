#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QLabel>
#include <iostream>

#include "settings.h"

MainWindow::MainWindow(QWidget *p) : QMainWindow(p), ui(new Ui::MainWindow)
{
    ui->setupUi(this);              // setup the UI

    // Emulator -> GUI
    connect(&emu, SIGNAL(debugStr(QString)), this, SLOT(debugStr(QString))); //Not queued connection as it may cause a hang

    // GUI -> Emulator
    connect(ui->buttonReset, SIGNAL(clicked()), &emu, SLOT(test()));

    QString rompath = CEmuSettings::Instance()->getROMLocation();
    emu.rom = rompath.toStdString();
    emu.start();
    //std::cout<<emu.rom<<std::endl;
}

// window destructor
MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    qDebug("Terminating emulator thread...");

    if(emu.stop())
        qDebug("Successful!");
    else
        qDebug("Failed.");

    QMainWindow::closeEvent(e);
}

void MainWindow::debugStr(QString str)
{
    ui->console->moveCursor(QTextCursor::End);
    ui->console->insertPlainText(str);
}
