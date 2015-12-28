#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QLabel>

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
    // Misc.
    void closeEvent(QCloseEvent *) override;

    // ROM

    // Actions
    void runSetup(void);
    void screenshot(void);
    void recordGIF();
    void showAbout(void);
    void actionExit(void);
    void setUIMode(bool docks_enabled);

    // Console
    void consoleStr(QString str);

signals:
    void debuggerChangedState(bool running);

private:
    // Debugger
    void raiseDebugger();
    void populateDebugWindow();
    void changeDebuggerState();

    // Console
    void clearConsole(void);

    Ui::MainWindow *ui = nullptr;
    QSettings *settings = nullptr;
    QDockWidget *dock_debugger = nullptr;

    EmuThread emu;

    bool debugger_on;
};

// Used as global instance by EmuThread and Debugger class
extern MainWindow *main_window;

#endif // MAINWINDOW_H
