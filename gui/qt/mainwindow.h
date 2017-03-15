#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QShortcut>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QFileDialog>
#include <QtCore/QSettings>
#include <QtGui/QTextCursor>
#include <QtWidgets/QMessageBox>

#include "ipc.h"
#include "cemuopts.h"
#include "lcdwidget.h"
#include "romselection.h"
#include "emuthread.h"
#include "lcdpopout.h"
#include "keyhistory.h"
#include "keypad/qtkeypadbridge.h"
#include "qhexedit/qhexedit.h"

#include "../../core/vat.h"
#include "../../core/debug/debug.h"
#include "../../core/debug/disasm.h"

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(CEmuOpts opts,QWidget *p = Q_NULLPTR);
    ~MainWindow();
    bool IsInitialized();

public slots:
    // Console
    void consoleStr(QString);
    void consoleErrStr(QString);

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
    void setImagePath();

    // Debugging
    void debuggerGUIDisable();
    void debuggerGUIEnable();

    // Sending keys
    void sendASMKey();

    // LCD Popouts
    void createLCD();

signals:
    // Debugging
    void debuggerSendNewState(bool);
    void triggerEmuSendState();
    void debugInputRequested();
    void debuggerCommand(QString);
    void setDebugStepInMode();
    void setDebugStepOverMode();
    void setDebugStepNextMode();
    void setDebugStepOutMode();
    void setRunUntilMode();

    // Linking
    void setSendState(bool);
    void sendVariable(std::string);
    void setReceiveState(bool);

    // Speed
    void setEmuSpeed(int);
    void changedThrottleMode(bool);

    // Reset
    void resetTriggered();

protected:
    // Misc.
    virtual void closeEvent(QCloseEvent*) Q_DECL_OVERRIDE;
    virtual bool eventFilter(QObject*, QEvent*) Q_DECL_OVERRIDE;
    virtual void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

    // Drag & Drop
    virtual void dropEvent(QDropEvent*) Q_DECL_OVERRIDE;
    virtual void dragEnterEvent(QDragEnterEvent*) Q_DECL_OVERRIDE;

private:
    enum breakpointIndex {
        BREAK_LABEL_LOC=0,
        BREAK_ADDR_LOC,
        BREAK_ENABLE_LOC
    };

    enum watchpointIndex {
        WATCH_LABEL_LOC=0,
        WATCH_ADDR_LOC,
        WATCH_SIZE_LOC,
        WATCH_VALUE_LOC,
        WATCH_READ_LOC,
        WATCH_WRITE_LOC
    };

    enum portIndex {
        PORT_ADDR_LOC=0,
        PORT_VALUE_LOC,
        PORT_READ_LOC,
        PORT_WRITE_LOC,
        PORT_FREEZE_LOC
    };

    // Save/Restore
    void saveToPath(QString path);
    bool restoreFromPath(QString path);

    // Actions
    bool runSetup(void);
    void screenshot(void);
    void screenshotGIF(void);
    void screenshotSave(QString, QString, QString);
    void recordGIF(void);
    void changeFrameskip(int);
    void changeFramerate(void);
    void checkForUpdates(bool);
    void showAbout(void);
    void batteryIsCharging(bool);
    void batteryChangeStatus(int);
    void setPortableConfig(bool);
    void setAutoSaveState(bool);
    void changeSnapshotPath();

    // Debugger
    void debuggerInstall();
    void breakpointGUIAdd();
    void watchpointGUIAdd();
    void debuggerGUIPopulate();
    void debuggerGUISetState(bool);

    void debugCommand();
    void debuggerRaise();
    void debuggerLeave();
    void debuggerUpdateChanges();
    void debuggerChangeState();
    void debuggerExecuteCommand(uint32_t, uint8_t);
    void debuggerProcessCommand(int, uint32_t);

    void portRemoveSelected();

    void portUpdate(int);
    void watchpointUpdate(int);

    void portSetPreviousAddress(QTableWidgetItem*);
    void breakpointSetPreviousAddress(QTableWidgetItem*);
    void watchpointSetPreviousAddress(QTableWidgetItem*);

    void portDataChanged(QTableWidgetItem*);
    void breakpointDataChanged(QTableWidgetItem*);
    void watchpointDataChanged(QTableWidgetItem*);

    void updateDisasmView(const int, const bool);
    void drawNextDisassembleLine();
    void scrollDisasmView(int);

    void stepInPressed();
    void stepOverPressed();
    void stepNextPressed();
    void stepOutPressed();

    void updateTIOSView();
    void updateStackView();

    void gotoPressed();

    void disasmContextMenu(const QPoint &);
    void variablesContextMenu(const QPoint&);
    void vatContextMenu(const QPoint &);
    void opContextMenu(const QPoint &);

    void setDebugResetTrigger(bool);
    void setEnableSoftCommands(bool);

    void breakpointRemoveAddress(uint32_t);
    void watchpointRemoveAddress(uint32_t);

    void debuggerZeroClockCounter();
    void debuggerTabSwitched(int);

    void setDataCol(bool state);
    void setMenuBarState(bool state);

    // For linking to the buttons
    void breakpointSlotAdd();
    void watchpointSlotAdd();
    void portSlotAdd();

    // Removal from widgets
    bool breakpointRemoveSelectedRow();
    bool watchpointRemoveSelectedRow();

    // Get labels
    QString watchpointNextLabel();
    QString breakpointNextLabel();

    // Adding watchpoints from disassembly
    void watchpointReadGUIAdd();
    void watchpointWriteGUIAdd();
    void watchpointReadWriteGUIAdd();

    // Debugging files
    void debuggerImportFile(QString);
    void debuggerExportFile(QString);
    QString debuggerGetFile(int);
    void debuggerImport();
    void debuggerExport();

    // Creating bootable images
    bool checkForCEmuBootImage();
    void exportCEmuBootImage();
    bool loadCEmuBootImage(const QString&);
    void resetSettingsIfLoadedCEmuBootableImage();

    // MAIN IMPLEMENTATION ROUTINES
    bool portAdd(uint16_t, unsigned int);
    bool breakpointAdd(QString, uint32_t, bool);
    bool watchpointAdd(QString, uint32_t, uint8_t, unsigned int);

    void screenContextMenu(const QPoint&);
    void updateLabels();
    void equatesAddDialog();
    void equatesAddFile(QString);
    void equatesAddEquate(QString, QString);
    void equatesClear();
    void equatesRefresh();
    void selectKeypadColor();
    void setKeypadColor(unsigned int);

    // Speed
    void setEmulatedSpeed(int);
    void setThrottleMode(int);
    void showActualSpeed(int);

    // Console
    void showStatusMsg(QString);
    void consoleOutputChanged();
    void consoleAppend(QString, const QColor &color = Qt::black);

    // Settings
    void adjustScreen();
    void setDebugPath();
    void setSkinToggle(bool);
    void setLCDScale(int);
    void setLCDRefresh(int);
    void setAlwaysOnTop(int);
    void setAutoCheckForUpdates(int);
    void setSpaceDisasm(bool);
    void setUIStyle(bool);
    void setUIEditMode(bool);
    void toggleUIEditMode(void);
    void setSaveDebug(bool);
    void saveMiscSettings();

    // Linking
    QStringList showVariableFileDialog(QFileDialog::AcceptMode, QString);
    void selectFiles();
    void refreshVariableList();
    void variableClicked(QTableWidgetItem*);
    void saveSelected();

    // Autotester
    void dispAutotesterError(int);
    int openJSONConfig(const QString&);
    void prepareAndOpenJSONConfig();
    void reloadJSONConfig();
    void launchTest();
    void updateCRCParamsFromPreset(int);
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
    void memGoto(QString);
    void memGotoPressed();
    void memSearchPressed();
    void memSyncPressed();

    // Others
    void syncHexView(int, QHexEdit*);
    void searchEdit(QHexEdit*);
    QString getAddressEquate(const std::string&);

    // Keypad
    void keymapChanged();
    void setKeymap(const QString&);

    // Font
    void setFont(int);

    // Reset
    void reloadROM();
    void resetCalculator();

    // Misc
    QString getAddressString(QString, bool*);
    void optCheckSend(CEmuOpts&);
    void optLoadFiles(CEmuOpts&);
    void optAttemptLoad(CEmuOpts&);

    // Key History
    void toggleKeyHistory();

    // IPC
    void ipcSpawnRandom();
    bool ipcSetup();
    void ipcReceived();
    void ipcChangeID();
    void ipcHandleCommandlineReceive(QDataStream &);

#ifdef _WIN32
    // Win32 Console Toggle
    void toggleConsole();
    void installToggleConsole();
#endif

    // Members
    unsigned int watchpointGUIMask = DBG_NO_HANDLE;
    QString searchingString;

    Ui::MainWindow *ui = Q_NULLPTR;
    QtKeypadBridge keypadBridge{this};
    QLabel speedLabel;
    QLabel msgLabel;
    QSettings *settings = Q_NULLPTR;
    QTextCursor disasmOffset;
    bool disasmOffsetSet;
    bool fromPane;
    int32_t addressPane;
    int memSize;

    QDir currentDir;
    QStringList currentEquateFiles;
    EmuThread emu;

    bool uiEditMode;
    bool portable = false;
    bool nativeConsole = false;
    bool closeAfterSave = false;
    bool isResumed = false;
    bool hexSearch = true;
    bool canScroll = false;
    bool usingLoadedImage = false;
    bool recordingGif = false;

    bool firstTimeShown = false;

    CEmuOpts opts;

    uint32_t prevBreakpointAddress = 0;
    uint32_t prevWatchpointAddress = 0;
    uint32_t prevDisasmAddress = 0;
    uint16_t prevPortAddress = 0;
    QPalette colorback, nocolorback;

    QShortcut *stepInShortcut;
    QShortcut *stepOverShortcut;
    QShortcut *stepNextShortcut;
    QShortcut *stepOutShortcut;
    QShortcut *debuggerShortcut;
    QShortcut *asmShortcut;
    QShortcut *gifShortcut;

    QAction *toggleAction;

    QList<calc_var_t> vars;
    QIcon runIcon, stopIcon; // help speed up stepping
    QTextCharFormat consoleFormat;

    QString prevGotoAddress;
    QString prevFlashAddress;
    QString prevRAMAddress;
    QString prevMemAddress;

    QString pathSettings;
    QMenu *docksMenu;

    KeyHistory *keyHistoryWindow = Q_NULLPTR;

    ipc *com;

    bool initPassed = true;
    bool enabledSoftCommands;

    bool firstShow = false;
    bool useDataCol;
    bool loadedCEmuBootImage = false;
    static const int WindowStateVersion = 0;

    // Settings definitions

    static const QString SETTING_DEBUGGER_TEXT_SIZE;
    static const QString SETTING_DEBUGGER_ADD_DISASM_SPACE;
    static const QString SETTING_DEBUGGER_RESTORE_ON_OPEN;
    static const QString SETTING_DEBUGGER_SAVE_ON_CLOSE;
    static const QString SETTING_DEBUGGER_RESET_OPENS;
    static const QString SETTING_DEBUGGER_ENABLE_SOFT;
    static const QString SETTING_DEBUGGER_DATA_COL;
    static const QString SETTING_DEBUGGER_IMAGE_PATH;
    static const QString SETTING_DEBUGGER_FLASH_BYTES;
    static const QString SETTING_DEBUGGER_RAM_BYTES;
    static const QString SETTING_DEBUGGER_MEM_BYTES;
    static const QString SETTING_SCREEN_REFRESH_RATE;
    static const QString SETTING_SCREEN_SCALE;
    static const QString SETTING_SCREEN_SKIN;
    static const QString SETTING_KEYPAD_KEYMAP;
    static const QString SETTING_KEYPAD_COLOR;
    static const QString SETTING_WINDOW_SIZE;
    static const QString SETTING_WINDOW_STATE;
    static const QString SETTING_WINDOW_GEOMETRY;
    static const QString SETTING_CAPTURE_FRAMESKIP;
    static const QString SETTING_IMAGE_PATH;
    static const QString SETTING_ROM_PATH;
    static const QString SETTING_FIRST_RUN;
    static const QString SETTING_UI_EDIT_MODE;
    static const QString SETTING_SAVE_ON_CLOSE;
    static const QString SETTING_RESTORE_ON_OPEN;
    static const QString SETTING_EMUSPEED;
    static const QString SETTING_AUTOUPDATE;
    static const QString SETTING_DISABLE_MENUBAR;
    static const QString SETTING_ALWAYS_ON_TOP;
    static const QString SETTING_CURRENT_DIR;
    static const QString SETTING_ENABLE_WIN_CONSOLE;

    static const QString SETTING_KEYPAD_CEMU;
    static const QString SETTING_KEYPAD_TILEM;
    static const QString SETTING_KEYPAD_WABBITEMU;
    static const QString SETTING_KEYPAD_JSTIFIED;

    static const QString SETTING_DEFAULT_FILE;
    static const QString SETTING_DEFAULT_ROM_FILE;
    static const QString SETTING_DEFAULT_IMAGE_FILE;
    static const QString SETTING_DEFAULT_DEBUG_FILE;

    QMessageBox *infoBox = Q_NULLPTR;
};

#endif
