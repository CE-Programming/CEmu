#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>

#include "emuthread.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *p = 0);
    ~MainWindow();

public slots:
    void closeEvent(QCloseEvent *) override;

    //Debugging
    void debugStr(QString str);

private slots:

private:
    Ui::MainWindow *ui = nullptr;

    EmuThread emu;
};

// Used as global instance by EmuThread and friends
extern MainWindow *main_window;

#endif // MAINWINDOW_H
