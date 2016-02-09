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
#include "../../core/vat.h"
#include "../../core/debug/debug.h"
#include "../../core/debug/disasm.h"
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
    void closeEvent(QCloseEvent*) override;

    //Drag & Drop
    void dropEvent(QDropEvent*) override;
    void dragEnterEvent(QDragEnterEvent*) override;

    // Console
    void consoleStr(QString);
    void consoleChar(const char c);

signals:
    // Debugging
    void debuggerChangedState(bool);
    void triggerEmuSendState();
    void debugInputRequested();
    void debuggerCommand(QString);

    // Linking
    void setSendState(bool);
    void sendVariable(std::string);
    void setReceiveState(bool);
    void setDebugStepInMode();
    void setDebugStepOverMode();
    void setDebugStepNextMode();
    void setDebugStepOutMode();

    // Speed
    void changedEmuSpeed(int);
    void changedThrottleMode(bool);

private:
    // Actions
    bool runSetup(void);
    void screenshot(void);
    void screenshotGIF(void);
    void recordGIF(void);
    void changeFrameskip(int);
    void changeFramerate(void);
    void checkForUpdates(bool);
    void showAbout(void);
    void setUIMode(bool);
    void changeBatteryCharging(bool);
    void changeBatteryStatus(int);

    // Debugger
    void debugCommand();
    void raiseDebugger();
    void updateDebuggerChanges();
    void populateDebugWindow();
    void setDebuggerState(bool);
    void changeDebuggerState();
    void executeDebugCommand(uint32_t, uint8_t);
    void processDebugCommand(int, uint32_t);
    void portMonitorCheckboxToggled(QTableWidgetItem *);
    void addPort();
    void deletePort();
    void updatePortData(int);
    void changePortData(QTableWidgetItem*);
    void deleteBreakpoint();
    void breakpointCheckboxToggled(QTableWidgetItem *);
    void drawNextDisassembleLine();
    void stepInPressed();
    void stepOverPressed();
    void stepNextPressed();
    void stepOutPressed();
    void updateTIOSView();
    void updateStackView();
    void updateDisasmView(const int, const bool);
    void gotoPressed();
    void setBreakpointAddress();
    void disasmContextMenu(const QPoint &);
    void vatContextMenu(const QPoint &);
    void opContextMenu(const QPoint &);
    bool addBreakpoint();

    // Others
    void screenContextMenu(const QPoint &);
    void resetCalculator();
    void addEquateFileDialog();
    void addEquateFile(QString);
    void clearEquateFile();
    void refreshEquateFile();

    // Speed
    void changeEmulatedSpeed(int);
    void changeThrottleMode(int);
    void showActualSpeed(int);

    // Console
    void showStatusMsg(QString);
    void consoleOutputChanged();

    // Settings
    void adjustScreen();
    void changeScale(int);
    void toggleSkin(bool);
    void changeLCDRefresh(int);
    void alwaysOnTop(int);
    void autoCheckForUpdates(int);
    int reprintScale(int);

    // Linking
    QStringList showVariableFileDialog(QFileDialog::AcceptMode mode);
    void sendFiles(QStringList fileNames);
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
    void memGoto(QString address);
    void memGotoPressed();
    void memSearchPressed();
    void memSyncPressed();
    void syncHexView(int, QHexEdit *);
    void searchEdit(QHexEdit *);

    // Keypad
    void keymapChanged();
    void changeKeymap(const QString &);

    // Font
    void setFont(int);
    QString getAddressString(bool &, QString);

    Ui::MainWindow *ui = nullptr;
    QLabel statusLabel;
    QSettings *settings = nullptr;
    QDockWidget *debuggerDock = nullptr;
    QTextCursor disasmOffset;
    bool disasmOffsetSet;
    bool fromPane;
    int addressPane;
    int memSize;

    QDir currentDir;
    QString currentEquateFile;
    EmuThread emu;

    bool debuggerOn = false;
    bool inReceivingMode = false;
    bool stderrConsole = false;

    QList<calc_var_t> vars;
};

// Used as global instance by EmuThread and Debugger class
extern MainWindow *main_window;

#endif
