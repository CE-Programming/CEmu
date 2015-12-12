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

    // Actions
    void runSetup(void);
    void showAbout(void);
    void actionExit(void);

    // Console
    void clearConsole(void);
    void consoleStr(QString str);

    // Debugger
    void initDebugger();
    void checkDebuggerState();
    void raiseDebugger();
    void populateDebugWindow();

private slots:
    void enableDebugger( bool );

private:
    typedef struct gui_debug_state {
        bool stopped;
    } gui_debug_state_t;
    gui_debug_state_t gui_debug;

    Ui::MainWindow *ui = nullptr;
    EmuThread emu;

    QLabel status_label;
};

// Used as global instance by EmuThread and friends
extern MainWindow *main_window;

#endif // MAINWINDOW_H
