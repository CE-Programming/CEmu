#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTableWidgetItem>
#include <QtCore/QSettings>

#include "romselection.h"
#include "emuthread.h"
#include "core/debug/debug.h"

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
    void screenshot(void);
    void recordGIF();
    void showAbout(void);
    void setUIMode(bool docks_enabled);

    // Console
    void consoleStr(QString str);

signals:
    void debuggerChangedState(bool running);
    void triggerEmuSendState();

    void setSendState(bool);
    void sendVariable(std::string);
    void setRecieveState(bool);

private:
    // Debugger
    void raiseDebugger();
    void updateDebuggerChanges();
    void populateDebugWindow();
    void changeDebuggerState();
    void processDebugCommand(int reason, uint32_t input);
    void portMonitorCheckboxToggled(QTableWidgetItem * item);
    void pollPort();
    void deletePort();
    void updatePortData(int currentRow);

    // Console
    void clearConsole(void);

    // Settings
    void changeLCDRefresh(int value);
    void alwaysOnTop(int state);

    // Linking
    void selectFiles();
    void refreshVariableList();
    void saveSelected();

    Ui::MainWindow *ui = nullptr;
    QSettings *settings = nullptr;
    QDockWidget *dock_debugger = nullptr;

    EmuThread emu;

    bool debugger_on = false;
    bool in_recieving_mode = false;
};

// Used as global instance by EmuThread and Debugger class
extern MainWindow *main_window;

#endif
