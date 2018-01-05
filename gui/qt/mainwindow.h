#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QProgressBar>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QFileDialog>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtGui/QTextCursor>
#include <QtWidgets/QMessageBox>

#include "ipc.h"
#include "searchwidget.h"
#include "cemuopts.h"
#include "lcdwidget.h"
#include "romselection.h"
#include "emuthread.h"
#include "keyhistory.h"
#include "keypad/qtkeypadbridge.h"
#include "qhexedit/qhexedit.h"
#include "png.h"

#include "../../core/vat.h"
#include "../../core/debug/debug.h"
#include "../../core/debug/disasm.h"

#ifdef PNG_WRITE_APNG_SUPPORTED
class RecordingThread : public QThread {
    Q_OBJECT
protected:
    virtual void run() Q_DECL_OVERRIDE;
public:
    QString filename;
    bool optimize;
signals:
    void done();
};
#endif

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(CEmuOpts opts,QWidget *p = Q_NULLPTR);
    ~MainWindow();
    bool IsInitialized();
    bool IsReload();

public slots:
    // Console
    void consoleStr(const QString& str);
    void consoleErrStr(const QString& str);

    // Saved/Restored State
    void saved(bool success);
    void started(bool success);
    void emuStopped();
    void restored(bool success);

    // ROM Image setting
    void setRom(const QString& name);

    // Other
    bool restoreEmuState();
    void saveEmuState();
    void restoreFromFile();
    void saveToFile();
    void exportRom();
    void setImagePath();
#ifdef PNG_WRITE_APNG_SUPPORTED
    void updateAnimatedControls();
#endif

    // Debugging
    void debuggerGUIDisable();
    void debuggerGUIEnable();

    // Sending keys
    void sendASMKey();

    // LCD Popouts
    void newMemoryVisualizer();

signals:
    // Debugging
    void setDebugState(bool state);
    void setDebugStepInMode();
    void setDebugStepOverMode();
    void setDebugStepNextMode();
    void setDebugStepOutMode();
    void setRunUntilMode();

    // Speed
    void changedEmuSpeed(int speed);
    void changedThrottleMode(bool throttled);

    // Reset
    void reset();
    void load();

    // Linking
    void receive();
    void receiveDone();

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
        BREAK_ENABLE_LOC,
        BREAK_REMOVE_LOC
    };

    enum watchpointIndex {
        WATCH_LABEL_LOC=0,
        WATCH_ADDR_LOC,
        WATCH_SIZE_LOC,
        WATCH_VALUE_LOC,
        WATCH_READ_LOC,
        WATCH_WRITE_LOC,
        WATCH_REMOVE_LOC
    };

    enum portIndex {
        PORT_ADDR_LOC=0,
        PORT_VALUE_LOC,
        PORT_READ_LOC,
        PORT_WRITE_LOC,
        PORT_FREEZE_LOC,
        PORT_REMOVE_LOC
    };

    enum opIndex {
        OP_ADDRESS=0,
        OP_NUMBER,
        OP_DATA,
        OP_DATASTRING
    };

    enum vatIndex {
        VAT_ADDRESS=0,
        VAT_VAT_ADDRESS,
        VAT_SIZE,
        VAT_NAME,
        VAT_TYPE
    };

    enum varIndex {
        VAR_NAME=0,
        VAR_LOCATION,
        VAR_TYPE,
        VAR_SIZE,
        VAR_PREVIEW
    };

    enum recentIndex {
        RECENT_SELECT=0,
        RECENT_LOAD,
        RECENT_PATH,
    };

    enum slotIndex {
        SLOT_NAME=0,
        SLOT_LOAD,
        SLOT_SAVE,
        SLOT_EDIT,
        SLOT_REMOVE
    };

    enum memIndex {
        MEM_FLASH=0,
        MEM_RAM,
        MEM_MEM
    };

    // Save/Restore
    void saveToPath(const QString& path);
    bool restoreFromPath(const QString& path);

    // Actions
    bool runSetup();
    void screenshot();
    void screenshotSave(const QString& nameFilter, const QString& defaultSuffix, const QString& temppath);
#ifdef PNG_WRITE_APNG_SUPPORTED
    void recordAPNG();
    void saveAnimated(QString &filename);
#endif
    void setFrameskip(int value);
    void setOptimizeRecording(bool state);
    void changeFramerate();
    void checkForUpdates(bool forceInfoBox);
    void showAbout();
    void batteryIsCharging(bool checked);
    void batteryChangeStatus(int value);
    void setPortableConfig(bool state);
    void setAutoSaveState(bool state);
    void changeSnapshotPath();

    // Debugger
    void breakpointGUIAdd();
    void watchpointGUIAdd();
    void debuggerGUIPopulate();
    void debuggerGUISetState(bool state);

    void debuggerInstall();
    void debuggerRaise();
    void debuggerLeave();
    void debuggerUpdateChanges();
    void debuggerChangeState();
    void debuggerExecuteCommand(uint32_t debugAddress, uint8_t command);
    void debuggerProcessCommand(int reason, uint32_t input);
    void toggleADLDisasm(int state);
    void toggleADLStack(int state);
    void toggleADL(int state);

    void portRemoveSelected();
    void portRemoveRow(int row);

    void portUpdate(int currRow);
    void watchpointUpdate(int row);

    void portSetPreviousAddress(QTableWidgetItem* curr_item);
    void breakpointSetPreviousAddress(QTableWidgetItem* curr_item);
    void watchpointSetPreviousAddress(QTableWidgetItem* curr_item);

    void portDataChanged(QTableWidgetItem* item);
    void breakpointDataChanged(QTableWidgetItem* item);
    void watchpointDataChanged(QTableWidgetItem* item);

    void updateDisasm();
    void updateDisasmView(int sentBase, bool newPane);
    void drawNextDisassembleLine();
    void scrollDisasmView(int value);

    void stepInPressed();
    void stepOverPressed();
    void stepNextPressed();
    void stepOutPressed();

    void updateTIOSView();
    void updateStackView();

    void gotoPressed();
    void slotAddNew();
    void slotAdd(QString &name, QString &path);
    void slotRemove();
    void slotEdit();
    void slotSave();
    void slotLoad();
    void saveSlotInfo();
    void setSlotInfo();
    int slotGet(QObject *obj, int col);

    void disasmContextMenu(const QPoint &);
    void variablesContextMenu(const QPoint&);
    void vatContextMenu(const QPoint &);
    void opContextMenu(const QPoint &);
    void memContextMenu(const QPoint &);
    void memoryContextMenu(const QPoint &, uint32_t);
    void removeAllSentVars();
    void removeSentVars();
    void deselectAllVars();
    void selectAllVars();
    void resendContextMenu(const QPoint &);

    void setDebugIgnoreBreakpoints(bool state);
    void setDebugResetTrigger(bool state);
    void setDebugSoftCommands(bool state);
    void setFocusSetting(bool state);

    void breakpointRemoveAddress(uint32_t address);
    void watchpointRemoveAddress(uint32_t address);

    void debuggerZeroClockCounter();

    void setDataCol(bool state);
    void setMenuBarState(bool state);
    void setDockBoundaries(bool state);

    // For linking to the buttons
    void breakpointSlotAdd();
    void watchpointSlotAdd();
    void portSlotAdd();

    // Removal from widgets
    void breakpointRemoveSelected();
    void watchpointRemoveSelected();
    void breakpointRemoveRow(int row);
    void watchpointRemoveRow(int row);

    // Get labels
    QString watchpointNextLabel();
    QString breakpointNextLabel();

    // Adding watchpoints from disassembly
    void watchpointReadGUIAdd();
    void watchpointWriteGUIAdd();
    void watchpointReadWriteGUIAdd();

    // Debugging files
    void debuggerImportFile(const QString &filename);
    void debuggerExportFile(const QString &filename);
    QString debuggerGetFile(int mode);
    void debuggerImport();
    void debuggerExport();

    // Create memory views
    void createMemoryDock(QString title);

    // Creating bootable images
    bool checkForCEmuBootImage();
    void exportCEmuBootImage();
    bool loadCEmuBootImage(const QString& bootImagePath);
    void resetSettingsIfLoadedCEmuBootableImage();

    // MAIN IMPLEMENTATION ROUTINES
    bool portAdd(uint16_t port, unsigned int mask);
    bool breakpointAdd(const QString& label, uint32_t address, bool enabled);
    bool watchpointAdd(const QString& label, uint32_t address, uint8_t len, unsigned int mask);

    void screenContextMenu(const QPoint& posa);
    void updateLabels();
    void equatesAddDialog();
    void equatesAddFile(const QString& fileName);
    void equatesAddEquate(const QString &name, const QString &addrStr);
    void equatesClear();
    void equatesRefresh();
    void selectKeypadColor();
    void setKeypadColor(unsigned int color);
    void setCalcSkinTopFromType();

    // Speed
    void setEmuSpeed(int value);
    void setThrottle(int mode);
    void showEmuUpdates(int speed, int fps);

    // Console
    void showStatusMsg(const QString& str);
    void consoleOutputChanged();
    void consoleAppend(const QString& str, const QColor &color = Qt::black);

    // Settings
    void adjustScreen();
    void setDebugPath();
    void setSkinToggle(bool enable);
    void setLcdScale(int state);
    void setGuiSkip(int value);
    void setLcdSpi(bool state);
    void setLcdDma(bool state);
    void setAlwaysOnTop(int state);
    void setAutoCheckForUpdates(int state);
    void setSpaceDisasm(bool state);
    void setUIDocks();
    void setUIEditMode(bool mode);
    void toggleUIEditMode();
    void setSaveDebug(bool state);
    void saveMiscSettings();

    // Linking
    QStringList showVariableFileDialog(QFileDialog::AcceptMode mode, const QString &name_filter, const QString &defaultSuffix);
    void selectFiles();
    void changeVariableList();
    void variableDoubleClicked(QTableWidgetItem* item);
    void launchPrgm(const calc_var_t* prgm);
    void saveSelectedFile();
    void saveSelectedFiles();
    void resendFiles();
    void receiveChangeState();

    // Recent Files
    void setRecentInfo();
    void saveRecentInfo();
    void setRecentSave(bool state);

    // Autotester
    void dispAutotesterError(int errCode);
    int openJSONConfig(const QString& jsonPath);
    void prepareAndOpenJSONConfig();
    void reloadJSONConfig();
    void launchTest();
    void updateCRCParamsFromPreset(int comboBoxIdx);
    void refreshCRC();

    // Hex Editor
    void flashUpdate();
    void flashGotoPressed();
    void flashSyncPressed();
    void ramUpdate();
    void ramGotoPressed();
    void ramSyncPressed();
    void memEditUpdate(QHexEdit *edit, uint32_t addressBegin);
    void memUpdate(int index, uint32_t addressBegin);
    void memGoto(int index, const QString &addressStr);
    void memGotoPressed(int index);
    void memSearchPressed(int index);
    void memSyncPressed(int index);
    void memDocksUpdate();

    // Others
    void syncHexView(int posa, QHexEdit *hex_view);
    void searchEdit(int index);
    QString getAddressEquate(const std::string& in);

    // Keypad
    void keymapChanged();
    void setKeymap(const QString& value);

    // Font
    void setFont(int fontSize);

    // Reset
    void reloadROM();
    void resetCalculator();

    // Misc
    QString getAddressString(const QString& string, bool* ok);
    void optCheckSend(CEmuOpts& o);
    void optLoadFiles(CEmuOpts& o);
    void optAttemptLoad(CEmuOpts& o);
    void pauseEmu(Qt::ApplicationState state);
    void setMemoryDocks();

    // State items
    void reloadAll();
    void reloadGui();

    // Key History
    void toggleKeyHistory();

    // Clipboard
    void saveScreenToClipboard();

    // IPC
    bool ipcSetup();
    void ipcSpawnRandom();
    void ipcCloseOthers();
    void ipcReceived();
    void ipcChangeID();
    void ipcHandleCommandlineReceive(QDataStream& stream);

#ifdef _WIN32
    // Win32 Console Toggle
    void toggleConsole();
    void installToggleConsole();
#endif

    // Misc
    int pausedSpeed;

    // Members
    unsigned int watchpointGUIMask = DBG_NO_HANDLE;
    QString searchingString;

    Ui::MainWindow *ui = Q_NULLPTR;
    QLabel speedLabel;
    QLabel msgLabel;
    QSettings *settings = Q_NULLPTR;
    QTextCursor disasmOffset;
    bool disasmOffsetSet;
    bool fromPane;
    int32_t addressPane;
    int hexSearch = SEARCH_MODE_HEX;

    QDir currDir;
    QStringList currentEquateFiles;
    EmuThread emu;

    bool uiEditMode;
    bool portable = false;
    bool nativeConsole = false;
    bool closeAfterSave = false;
    bool canScroll = false;
    bool recordingAnimated = false;

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
    QShortcut *resendshortcut;

    QAction *toggleAction;
    QAction *addMemory;

    QIcon runIcon, stopIcon;
    QIcon saveIcon, loadIcon, editIcon, removeIcon;
    QIcon searchIcon, gotoIcon;
    QTextCharFormat consoleFormat;

    QString prevGotoAddress;
    QString prevFlashAddress;
    QString prevRAMAddress;
    QString prevMemAddress;

    QString pathSettings;
    QMenu *docksMenu;
    QMenu *debugMenu;

    KeyHistory *keyHistoryWindow = Q_NULLPTR;

    ipc *com;

    // for drag and drop of rom files
    bool isSendingROM = false;
    QString dragROM;

    bool needReload = false;
    bool guiAdd = false;
    bool initPassed = true;
    bool firstShow = false;
    bool useDataCol;
    bool pauseOnFocus;
    bool loadedCEmuBootImage = false;
    bool optimizeRecording;
    static const int WindowStateVersion = 0;

    // Settings definitions

    static const QString MSG_INFORMATION;
    static const QString MSG_WARNING;
    static const QString MSG_ERROR;

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
    static const QString SETTING_DEBUGGER_BREAK_IGNORE;
    static const QString SETTING_DEBUGGER_IGNORE_DMA;
    static const QString SETTING_SCREEN_FRAMESKIP;
    static const QString SETTING_SCREEN_SCALE;
    static const QString SETTING_SCREEN_SKIN;
    static const QString SETTING_SCREEN_SPI;
    static const QString SETTING_KEYPAD_KEYMAP;
    static const QString SETTING_KEYPAD_COLOR;
    static const QString SETTING_WINDOW_SIZE;
    static const QString SETTING_WINDOW_STATE;
    static const QString SETTING_WINDOW_GEOMETRY;
    static const QString SETTING_WINDOW_SEPARATOR;
    static const QString SETTING_WINDOW_MENUBAR;
    static const QString SETTING_WINDOW_MEMORY_DOCKS;
    static const QString SETTING_CAPTURE_FRAMESKIP;
    static const QString SETTING_CAPTURE_OPTIMIZE;
    static const QString SETTING_SLOT_NAMES;
    static const QString SETTING_SLOT_PATHS;
    static const QString SETTING_IMAGE_PATH;
    static const QString SETTING_ROM_PATH;
    static const QString SETTING_FIRST_RUN;
    static const QString SETTING_UI_EDIT_MODE;
    static const QString SETTING_PAUSE_FOCUS;
    static const QString SETTING_SAVE_ON_CLOSE;
    static const QString SETTING_RESTORE_ON_OPEN;
    static const QString SETTING_EMUSPEED;
    static const QString SETTING_AUTOUPDATE;
    static const QString SETTING_ALWAYS_ON_TOP;
    static const QString SETTING_CURRENT_DIR;
    static const QString SETTING_ENABLE_WIN_CONSOLE;
    static const QString SETTING_RECENT_SAVE;
    static const QString SETTING_RECENT_PATHS;
    static const QString SETTING_RECENT_SELECT;

    static const QString SETTING_KEYPAD_CEMU;
    static const QString SETTING_KEYPAD_TILEM;
    static const QString SETTING_KEYPAD_WABBITEMU;
    static const QString SETTING_KEYPAD_JSTIFIED;

    static const QString SETTING_DEFAULT_FILE;
    static const QString SETTING_DEFAULT_ROM_FILE;
    static const QString SETTING_DEFAULT_IMAGE_FILE;
    static const QString SETTING_DEFAULT_DEBUG_FILE;

    static const QString TITLE_MEM_DOCK;

    QMessageBox *infoBox = Q_NULLPTR;
    QMessageBox *warnBox = Q_NULLPTR;
    QProgressBar *progressBar;
    QVector<QHexEdit*> memory;
    int memoryDocks = 0;

    static const char *var_extension[];
};

#endif
