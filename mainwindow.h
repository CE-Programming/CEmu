#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "romselection.h"
#include "aboutwindow.h"
#include "optionswindow.h"
#include "lcdwidget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionAbout_triggered();
    void on_actionSetup_Wizard_triggered();
    void on_actionExit_triggered();
    void on_actionOptions_triggered();
    void on_action100_2_triggered();
    void on_action200_2_triggered();

private:
    Ui::MainWindow *ui;

    bool running;
    bool debug;

    OptionsWindow *mOptionsWindow;
    AboutWindow *mAboutWindow;
    RomSelection *mRomSelection;
    LCDWidget *mLCD;
};

#endif // MAINWINDOW_H
