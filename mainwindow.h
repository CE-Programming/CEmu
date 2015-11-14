#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>

#include <romselection.h>
#include <emuthread.h>

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

    // Actions
    void runSetup(void);
    void showAbout(void);
    void actionExit(void);

    // Console
    void clearConsole(void);
    void consoleStr(QString str);

private slots:

private:
    Ui::MainWindow *ui = nullptr;

    EmuThread emu;
};

// Used as global instance by EmuThread and friends
extern MainWindow *main_window;

#endif // MAINWINDOW_H
