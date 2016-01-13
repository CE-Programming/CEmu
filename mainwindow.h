#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QFileDialog>
#include <QtCore/QSettings>
#include <QTextCursor>

#include "lcdwidget.h"
#include "romselection.h"
#include "emuthread.h"
#include "core/vat.h"
#include "core/debug/debug.h"
#include "core/debug/disasm.h"
#include "qhexedit/qhexedit.h"

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

    // Console
    void consoleStr(QString);

signals:
    void debuggerChangedState(bool);
    void triggerEmuSendState();

    void setSendState(bool);
    void sendVariable(std::string);
    void setReceiveState(bool);
    void setDebugStepMode();
    void setDebugStepOverMode();

private:
    // Actions
    bool runSetup(void);
    void screenshot(void);
    void screenshotGIF();
    void recordGIF();
    void showAbout(void);
    void setUIMode(bool);

    // Debugger
    void raiseDebugger();
    void updateDebuggerChanges();
    void populateDebugWindow();
    void setDebuggerState(bool);
    void changeDebuggerState();
    void processDebugCommand(int, uint32_t);
    void portMonitorCheckboxToggled(QTableWidgetItem *);
    void pollPort();
    void deletePort();
    void updatePortData(int);
    void deleteBreakpoint();
    void breakpointCheckboxToggled(QTableWidgetItem *);
    void drawNextDisassembleLine();
    void stepPressed();
    void stepOverPressed();
    void updateStackView();
    void updateDisasmView(const int, const bool);
    void gotoPressed();
    void breakpointPressed();
    void setPCaddress(const QPoint&);
    bool addBreakpoint();

    // Others
    void resetCalculator();
    void addEquateFile();
    void clearEquateFile();

    // Console
    void clearConsole(void);

    // Settings
    void changeLCDRefresh(int value);
    void alwaysOnTop(int state);
    void popoutLCD();

    // Linking
    QStringList showVariableFileDialog(QFileDialog::AcceptMode mode);
    void selectFiles();
    void refreshVariableList();
    void saveSelected();

    // Hex Editor
    void flashUpdate();
    void flashGotoPressed();
    void flashSearchPressed();
    void flashSyncPressed();
    void ramUpdate();
    void ramGotoPressed();
    void ramSearchPressed();
    void ramSyncPressed();
    void memUpdate();
    void memGotoPressed();
    void memSearchPressed();
    void memSyncPressed();
    void syncHexView(int, QHexEdit*);
    void searchEdit(QHexEdit*);

    QString getAddressString(bool&, QString);

    Ui::MainWindow *ui = nullptr;
    QSettings *settings = nullptr;
    QDockWidget *dock_debugger = nullptr;
    QTextCursor disasm_offset;
    bool detached_state = false;
    bool disasm_offset_set;
    bool from_pane;
    int address_pane;
    int mem_hex_size;

    QDir current_dir;
    EmuThread emu;
    LCDWidget detached_lcd;

    bool debugger_on = false;
    bool in_recieving_mode = false;

    QList<calc_var_t> vars;
};

// Used as global instance by EmuThread and Debugger class
extern MainWindow *main_window;

#endif
