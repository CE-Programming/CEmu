#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "lcdwidget.h"
#include "optionswindow.h"

#include <QLabel>

MainWindow::MainWindow(QWidget *p) : QMainWindow(p),ui(new Ui::MainWindow)
{
    ui->setupUi(this);              // setup the UI

    running=false;                  // we're not running yet
    debug=false;                    // not in debug mode

    mAboutWindow=NULL;              // initialize members
    mRomSelection=NULL;
    mOptionsWindow=NULL;
    mLCD=NULL;
}

// window destructor
MainWindow::~MainWindow()
{
    if(mAboutWindow != NULL)
        delete mAboutWindow;        // free the memory
    if(mRomSelection != NULL)
        delete mRomSelection;       // free the memory
    if(mLCD != NULL)
        delete mLCD;                // free the memory
    delete ui;                      // free the memory
}

// enter about screen
void MainWindow::on_actionAbout_triggered()
{
    if(mAboutWindow != NULL)
        delete mAboutWindow;         // free the memory
    mAboutWindow = new AboutWindow();
    mAboutWindow->show();
}

// enter setup wizard
void MainWindow::on_actionSetup_Wizard_triggered()
{
    if(mRomSelection != NULL)
        delete mRomSelection;         // free the memory
    mRomSelection = new RomSelection();
    mRomSelection->show();
}

// close all windows
void MainWindow::on_actionExit_triggered()
{
    // exit with code=0, because technically it is okay
    // probably leaks a ton of memory too
    // better hope the OS can handle it
    exit(0);
}

void MainWindow::on_actionOptions_triggered()
{
  if(mOptionsWindow != NULL)
      delete mOptionsWindow;         // free the memory
  mOptionsWindow = new OptionsWindow();
  mOptionsWindow->exec();
}

void MainWindow::on_action100_2_triggered()
{
    this->setFixedSize(340,280);
}

void MainWindow::on_action200_2_triggered()
{
    this->setFixedSize(320*2,(280*2)-80);
}
