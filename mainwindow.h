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
    void showStatusMsg(QString str);

    // ROM

    // Actions
    void runSetup(void);
    void screenshot(void);
    void recordGIF();
    void showAbout(void);
    void actionExit(void);
    void setUIMode(bool docks_enabled);

    // Console
    void clearConsole(void);
    void consoleStr(QString str);

    // Debugger
    void raiseDebugger();
    void populateDebugWindow();

private slots:

private:
    Ui::MainWindow *ui = nullptr;
    QSettings *settings = nullptr;
    RomSelection romSelection;

    EmuThread emu;

    QLabel status_label;
    bool in_debugger;

    // To make it possible to activate the debugger
    QDockWidget *dock_debugger = nullptr;
};

// Used as global instance by EmuThread and friends
extern MainWindow *main_window;

#endif // MAINWINDOW_H
