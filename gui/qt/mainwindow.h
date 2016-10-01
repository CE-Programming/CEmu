#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QShortcut>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QFileDialog>
#include <QtCore/QSettings>
#include <QtGui/QTextCursor>
#include "cemuopts.h"
#include "lcdwidget.h"
#include "romselection.h"
#include "emuthread.h"
#include "keypad/qtkeypadbridge.h"
#include "../../core/vat.h"
#include "../../core/debug/debug.h"
#include "../../core/debug/disasm.h"
#include "qhexedit/qhexedit.h"

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow {
    Q_OBJECT

public:

    explicit MainWindow(CEmuOpts opts,QWidget *p = Q_NULLPTR);
    ~MainWindow();

public slots:
    // Misc.
    void closeEvent(QCloseEvent*) override;

    //Drag & Drop
    void dropEvent(QDropEvent*) override;
    void dragEnterEvent(QDragEnterEvent*) override;

    // Console
    void consoleStr(QString);
    void errConsoleStr(QString);

    // Saved/Restored State
    void saved(bool);
    void started(bool);
    void restored(bool);

    // Other
    void isBusy(bool busy);
    bool restoreEmuState();
    void saveEmuState();
    void restoreFromFile();
    void saveToFile();
    void exportRom();
    void changeImagePath();
    void disableDebugger();

signals:
    // Debugging
    void debuggerChangedState(bool);
    void triggerEmuSendState();
    void debugInputRequested();
    void debuggerCommand(QString);
    void setDebugStepInMode();
    void setDebugStepOverMode();
    void setDebugStepNextMode();
    void setDebugStepOutMode();

    // Linking
    void setSendState(bool);
    void sendVariable(std::string);
    void setReceiveState(bool);

    // Speed
    void changedEmuSpeed(int);
    void changedThrottleMode(bool);

    // Reset
    void resetTriggered();

private:
    // Save/Restore
    void saveToPath(QString path);
    bool restoreFromPath(QString path);

    // Actions
    bool runSetup(void);
    void screenshot(void);
    void screenshotGIF(void);
    void saveScreenshot(QString,QString,QString);
    void recordGIF(void);
    void changeFrameskip(int);
    void changeFramerate(void);
    void checkForUpdates(bool);
    void showAbout(void);
    void setUIMode(bool);
    void changeBatteryCharging(bool);
    void changeBatteryStatus(int);
    void setSaveOnClose(bool b);
    void setRestoreOnOpen(bool b);
    void changeSnapshotPath();

    // Debugger
    void debugCommand();
    void raiseDebugger();
    void leaveDebugger();
    void updateDebuggerChanges();
    void populateDebugWindow();
    void setDebuggerState(bool);
    void changeDebuggerState();
    void executeDebugCommand(uint32_t, uint8_t);
    void processDebugCommand(int, uint32_t);
    void addPort();
    void removePort();
    void updatePortData(int);
    void updateWatchpointData(int);
    void changePortValues(QTableWidgetItem*);
    void changeBreakpointAddress(QTableWidgetItem*);
    void setPreviousBreakpointAddress(QTableWidgetItem*);
    void changeWatchpointAddress(QTableWidgetItem*);
    void setPreviousWatchpointAddress(QTableWidgetItem*);
    void setPreviousPortValues(QTableWidgetItem*);
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
    void setWatchpointAddress();
    void disasmContextMenu(const QPoint &);
    void variablesContextMenu(const QPoint&);
    void vatContextMenu(const QPoint &);
    void opContextMenu(const QPoint &);
    void scrollDisasmView(int);
    void removeBreakpointAddress(QString);
    void removeWatchpointAddress(QString);
    void zeroClockCounter();
    void updateDisassembly(int);
    void addSpaceDisasm(bool);
    bool removeBreakpoint();
    bool removeWatchpoint();
    bool addBreakpoint();
    bool addWatchpoint();

    // Others
    void createLCD();
    void screenContextMenu(const QPoint &);
    void addEquateFileDialog();
    void addEquateFile(QString);
    void clearEquateFile();
    void refreshEquateFile();
    void selectKeypadColor();
    void setKeypadColor(unsigned color);

    // Speed
    void changeEmulatedSpeed(int);
    void changeThrottleMode(int);
    void showActualSpeed(int);

    // Console
    void showStatusMsg(QString);
    void consoleOutputChanged();
    void appendToConsole(QString str, QColor color = Qt::black);

    // Settings
    void adjustScreen();
    void changeScale(int);
    void toggleSkin(bool);
    void changeLCDRefresh(int);
    void alwaysOnTop(int);
    void autoCheckForUpdates(int);
    int reprintScale(int);

    // Linking
    QStringList showVariableFileDialog(QFileDialog::AcceptMode, QString name_filter);
    void selectFiles();
    void refreshVariableList();
    void variableClicked(QTableWidgetItem*);
    void saveSelected();

    // Autotester
    void dispAutotesterError(int errCode);
    int openJSONConfig(const QString& jsonPath);
    void prepareAndOpenJSONConfig();
    void reloadJSONConfig();
    void launchTest();
    void updateCRCParamsFromPreset(int comboBoxIndex);
    void refreshCRC();

    // Hex Editor
    void flashUpdate();
    void flashGotoPressed();
    void flashSearchPressed();
    void flashSyncPressed();
    void ramUpdate();
    void ramGotoPressed();
    void ramSearchPressed();
    void ramSyncPressed();
    void memUpdate(uint32_t);
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

    // Reset
    void reloadROM();
    void resetCalculator();

#ifdef _WIN32
    // Win32 Console Toggle
    void toggleConsole();
    void installToggleConsole();
#endif

    // Members
    QString getAddressString(bool &, QString);
    QString searchingString;

    Ui::MainWindow *ui = nullptr;
    QtKeypadBridge keypadBridge{this};
    QLabel statusLabel;
    QSettings *settings = nullptr;
    QDockWidget *debuggerDock = nullptr;
    QTextCursor disasmOffset;
    bool disasmOffsetSet;
    bool fromPane;
    int32_t addressPane;
    int memSize;

    QDir currentDir;
    QString currentEquateFile;
    EmuThread emu;

    bool debuggerOn = false;
    bool inReceivingMode = false;
    bool native_console = false;
    bool closeAfterSave = false;
    bool isResumed = false;
    bool hexSearch = true;
    bool canScroll = false;

    CEmuOpts opts;

    uint16_t prevPortAddress = 0;
    uint32_t prevBreakpointAddress = 0;
    uint32_t prevWatchpointAddress = 0;
    uint32_t prevDisasmAddress = 0;
    uint32_t currAddress = 0;
    uint8_t watchpointType = 0;
    QString currAddressString, currPortAddress, watchLength;
    QPalette colorback, nocolorback;

    QShortcut *stepInShortcut;
    QShortcut *stepOverShortcut;
    QShortcut *stepNextShortcut;
    QShortcut *stepOutShortcut;
    QShortcut *debuggerShortcut;

    QList<calc_var_t> vars;
    QIcon runIcon, stopIcon; // help speed up stepping
    QTextCharFormat console_format;

    int pc_line;
    int curr_line;
};

#endif
