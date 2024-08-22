﻿#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ipc.h"
#include "searchwidget.h"
#include "cemuopts.h"
#include "lcdwidget.h"
#include "romselection.h"
#include "emuthread.h"
#include "keyhistorywidget.h"
#include "dockwidget.h"
#include "datawidget.h"
#include "vartablemodel.h"
#include "keypad/qtkeypadbridge.h"
#include "debugger/hexwidget.h"
#include "debugger/disasm.h"
#include "capture/animated-png.h"
#include "../../core/vat.h"
#include "../../core/debug/debug.h"

#include <QtWidgets/QProgressBar>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QFileDialog>
#include <QShortcut> /* Different module in Qt5 vs Qt6 */
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtCore/QPointer>
#include <QtGui/QTextCursor>
#include <QtGui/QFont>
#include <QtWidgets/QMessageBox>
#include <QtCore/QTranslator>
#include <QtCore/QStandardPaths>

#ifdef PNG_WRITE_APNG_SUPPORTED
class RecordingThread : public QThread {
    Q_OBJECT
protected:
    virtual void run() Q_DECL_OVERRIDE;
public:
    QString m_filename;
    bool m_optimize;
signals:
    void done();
};
#endif

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(CEmuOpts &opts, QWidget *p = Q_NULLPTR);
    ~MainWindow() Q_DECL_OVERRIDE;
    void setup();
    bool isInitialized();
    bool isReload();
    bool isResetAll();

signals:
    void setLcdFrameskip(int value);
    void setLcdResponseMode(bool state);

protected:
    virtual void changeEvent(QEvent* event) Q_DECL_OVERRIDE;
    virtual void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
    virtual bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;
    virtual void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
    virtual void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;

private:
    typedef struct {
        int line;
        int offset;
        int len;
    } token_highlight_t;

    typedef enum {
        DBG_BASIC_SUCCESS,
        DBG_BASIC_NO_EXECUTING_PRGM,
        DBG_BASIC_NEED_REFRESH,
        DBG_BASIC_NO_REFRESH,
    } debug_basic_status_t;

    enum {
        CONSOLE_ESC,
        CONSOLE_BRACKET,
        CONSOLE_PARSE,
        CONSOLE_BGCOLOR,
        CONSOLE_FGCOLOR,
        CONSOLE_EQUALS,
        CONSOLE_ENDVAL
    };

    enum {
        BREAK_REMOVE_COL,
        BREAK_ENABLE_COL,
        BREAK_ADDR_COL,
        BREAK_NAME_COL
    };

    enum {
        WATCH_REMOVE_COL,
        WATCH_READ_COL,
        WATCH_WRITE_COL,
        WATCH_LOW_COL,
        WATCH_HIGH_COL,
        WATCH_NAME_COL,
    };

    enum {
        PORT_REMOVE_COL,
        PORT_READ_COL,
        PORT_WRITE_COL,
        PORT_FREEZE_COL,
        PORT_ADDR_COL,
        PORT_VALUE_COL,
    };

    enum {
        OP_ADDR_COL,
        OP_NUMBER_COL,
        OP_DATA_COL,
        OP_STRING_COL,
        OP_VALUE_COL
    };

    enum {
        FP_ADDR_COL,
        FP_DATA_COL,
        FP_STRING_COL,
        FP_VALUE_COL
    };

    enum {
        OPS_ADDR_COL,
        OPS_DATA_COL,
        OPS_STRING_COL
    };

    enum {
        VAT_ADDR_COL,
        VAT_VAT_ADDR_COL,
        VAT_SIZE_COL,
        VAT_NAME_COL,
        VAT_TYPE_COL
    };

    enum {
        RECENT_REMOVE_COL,
        RECENT_RESEND_COL,
        RECENT_SELECT_COL,
        RECENT_PATH_COL
    };

    enum {
        SLOT_REMOVE_COL,
        SLOT_LOAD_COL,
        SLOT_SAVE_COL,
        SLOT_EDIT_COL,
        SLOT_NAME_COL
    };

    enum {
        TRANSLATE_INIT,
        TRANSLATE_UPDATE,
        TRANSLATE_ONLY
    };

    enum {
        FULLSCREEN_NONE,
        FULLSCREEN_ALL,
        FULLSCREEN_LCD
    };

    // emu keypresses
    void sendEmuKey(uint16_t key);
    void sendEmuLetterKey(char letter);

    // console
    void console(const QString &str, int type = EmuThread::ConsoleNorm);
    void console(int type, const char *str, int size = -1);
    void consoleStr();
    void consoleClear();
    void consoleModified();

    // Other
    void romExport();
    void ramExport();
    void guiExport();
    void ramImport();
    void guiImport();

    // emu state
    void emuSaved(bool success);
    void emuBlocked(int req);

    // debug
    void debugDisable();
    void debugEnable();

    // translations
    void translateExtras(int init);
    void translateSwitch(const QString &lang);

    // dark mode
    void darkModeSwitch(bool darkMode);

    // state slots
    void stateAdd(QString &name, QString &path);
    void stateAddNew();
    void stateRemove();
    void stateEdit();
    void stateSave();
    void stateLoad();
    void stateSaveInfo();
    void stateLoadInfo();
    int stateGet(QObject *obj, int col);

    // save / restore states
    void stateToFile();
    void stateToPath(const QString &path);
    void stateFromFile();
    void stateFromPath(const QString &path);

    // others
    bool runSetup();
    void showAbout();
    void checkUpdate(bool info);

    // screenshots
    void screenshot();
    void screenshotSave(const QString &filter, const QString &suffix, const QString &temp);
    void setFrameskip(int value);
    void setOptimizeRecord(bool state);
    void setSnapshotPath();
#ifdef PNG_WRITE_APNG_SUPPORTED
    void recordAnimated();
    void recordSave(const QString &file);
    void recordControlUpdate();
#endif

    // debugger
    void debugPopulate();
    void debugGuiState(bool state);
    void debugForce();
    void debugInit();
    void debugRaise();
    void debugSync();
    void debugToggle();
    void debugExecute(uint32_t addr, uint8_t cmd);
    void debugCommand(int reason, uint32_t data);
    void debugZeroCycles();

    // ti-basic debugging
    void debugBasic(bool enable);
    void debugBasicInit();
    debug_basic_status_t debugBasicRaise();
    void debugBasicToggle();
    void debugBasicLeave(bool allowRefresh);
    debug_basic_status_t debugBasicEnable();
    void debugBasicDisable();
    void debugBasicStep();
    void debugBasicStepNext();
    void debugBasicClearCache();
    QString debugBasicGetPrgmName();
    debug_basic_status_t  debugBasicUpdate(bool force);
    debug_basic_status_t debugBasicPgrmLookup(bool allowSwitch, int *idx);
    void debugBasicCreateTokenMap(int idx, const QByteArray &data);
    debug_basic_status_t debugBasicGuiState(bool state);
    void debugBasicToggleHighlight(bool enabled);
    void debugBasicToggleShowFetch(bool enabled);
    void debugBasicToggleShowTempParse(bool enabled);
    void debugBasicToggleLiveExecution(bool enabled);

    // battery
    void batterySetCharging(bool state);
    void batterySet(int value);

    // disassembly
    void disasmUpdateAddr(int base, bool pane);
    void disasmUpdate();
    void disasmLine();
    void disasmScroll(int value);

    // stack
    void stackUpdate();
    void stackLine();
    void stackScroll(int value);

    // cpu state
    bool adlState(int state);

    // stepping
    void debugStep(int mode);
    void stepIn();
    void stepOver();
    void stepNext();
    void stepOut();

    // os view
    void osUpdate();
    void opModified(QTableWidgetItem *item);
    void fpModified(QTableWidgetItem *item);

    // goto
    void gotoPressed();
    void gotoDisasmAddr(uint32_t addr);
    QAction *gotoDisasmAction(QMenu *menu);
    void gotoMemAddr(uint32_t addr);
    QAction *gotoMemAction(QMenu *menu, bool vat = false);

    void handleCtrlClickText(QPlainTextEdit *edit);
    void handleCtrlClickLine(QLineEdit *edit);

    // breakpoints
    void breakModified(QTableWidgetItem *item);
    void breakSetPrev(QTableWidgetItem *current, QTableWidgetItem *previous);
    int breakGetMask(int row);

    // breakpoint additions
    bool breakAdd(const QString &label, uint32_t address, bool enabled, bool toggle, bool unset);
    void breakAddGui();
    void breakAddSlot();

    // breakpoint removal
    void breakRemove(uint32_t addr);
    void breakRemoveRow(int row);
    void breakRemoveSelected();

    // watchpoints
    void watchModified(QTableWidgetItem *item);
    void watchSetPrev(QTableWidgetItem *current, QTableWidgetItem *previous);
    void watchUpdate();
    void watchUpdateRow(int row);
    int watchGetMask(int row);

    // watchpoint additions
    bool watchAdd(const QString &label, uint32_t low, uint32_t high, int mask, bool toggle, bool unset);
    void watchAddGui();
    void watchAddGuiR();
    void watchAddGuiW();
    void watchAddGuiRW();
    void watchAddSlot();

    // watchpoint removal
    void watchRemove(uint32_t addr);
    void watchRemoveRow(int row);
    void watchRemoveSelected();

    // ports
    void portModified(QTableWidgetItem *item);
    void portSetPrev(QTableWidgetItem *current, QTableWidgetItem *previous);
    void portPopulate(int row);
    int portGetMask(int row);

    // port additions
    bool portAdd(uint16_t port, int mask, bool unset);
    void portAddSlot();

    // port removal
    void portRemoveSelected();
    void portRemoveRow(int row);

    // labels
    QString watchNextLabel();
    QString breakNextLabel();
    void updateLabels();

    // debug files
    void debugImportFile(const QString &file);
    void debugExportFile(const QString &file);
    QString debugGetFile(bool save);

    // bootable images
    bool bootImageImport(const QString &path);
    bool bootImageCheck();
    void bootImageExport();

    // equates
    void setDebugAutoEquates(bool enable);
    void equatesAddDialog();
    void equatesAddFile(const QString &file);
    void equatesAddEquate(const QString &name, uint32_t addr);
    bool equatesAddEquateInternal(const QString &name, uint32_t addr);
    void equatesClear();
    void equatesRefresh();
    QString getAddressString(const QString &string, bool *ok);

    // keypad
    void keymapLoad();
    void keymapChanged();
    void keymapCustomSelected();
    void keypadChanged();
    void setKeymap(const QString &value);
    void setKeypadColor(unsigned int color);
    void setKeypadHolding(bool enabled);
    void setCalcSkinTopFromType(bool python);

    // settings
    void setRom(const QString &path);
    void setImagePath();
    void setCalcId();
    void setFocusSetting(bool state);
    void setStatusInterval(int value);
    void setSkinToggle(bool state);
    void setGuiSkip(int value);
    void setLcdScale(int value);
    void setLcdUpscale(int value);
    void setLcdFullscreen(int value);
    void setLcdDma(bool state);
    void setLcdGamma(bool state);
    void setLcdResponse(bool state);
    void setDebugLcdDma(bool state);
    void setTop(bool state);
    void setDockGroupDrag(bool state);
    void setMenuBarState(bool state);
    void setStatusBarState(bool state);
    void setUIBoundaries(bool state);
    void setAsicValidRevisions();
    void setAsicRevision(int index);
    void setAllowAnyRev(bool state);
    void setPythonEdition(int state);
    void setNormalOs(bool state);
    void setRecentSave(bool state);
    void setPortable(bool state);
    void setAutoSave(bool state);
    void setAutoUpdates(int value);
    void saveSettings();
    void saveDebug();
    void setFont(int size);
    void setUIDocks();
    void setUIDockEditMode(bool mode);
    void setUIEditMode(bool mode);
    void setFullscreen(int value);
    void iconsLoad();
    void showAsicRevInfo(const QList<int>& supportedRevs, int loadedRev, int defaultRev, bool python);

    // speed settings
    void setEmuSpeed(int value);
    void setThrottle(int mode);
    void showEmuSpeed(double emuTime);
    void timeoutEmuSpeed();
    void showFpsSpeed(double emuFps, double guiFps);
    void timeoutFpsSpeed();
    void showStatusMsg(const QString &str);

    // debug settings
    void setDebugPath();
    void setDebugAutoSave(bool state);
    void setDebugDisasmDataCol(bool state);
    void setDebugDisasmAddrCol(bool state);
    void setDebugDisasmSpace(bool state);
    void setDebugDisasmTab(bool state);
    void setDebugDisasmImplict(bool state);
    void setDebugDisasmUppercase(bool state);
    void setDebugDisasmBoldSymbols(bool state);
    void setDebugIgnoreBreakpoints(bool state);
    void setDebugResetTrigger(bool state);
    void setDebugSoftCommands(bool state);

    // linking
    QStringList varDialog(QFileDialog::AcceptMode mode, const QString &filter, const QString &suffix);
    void varReceive(std::function<void(bool)> recvAction);
    void varUpdate();
    void varShow();
    void varPressed(const QModelIndex &index);
    void varLaunch(const calc_var_t *prgm);
    void varSelect();
    void varSaveSelected();
    void varSaveSelectedFiles();
    void varResend();
    void varToggle();

    // recent
    void recentLoadInfo();
    void recentSaveInfo();

    // autotester
    int autotesterOpen(const QString &jsonPath);
    void autotesterUpdatePresets(int comboBoxIdx);
    void autotesterErr(int errCode);
    void autotesterTested(int status);
    void autotesterLoad();
    void autotesterReload();
    void autotesterLaunch();
    void autotesterRefreshCRC();

    // keybindings
    void keymapExport();

    // memory
    void flashUpdate();
    void flashGotoPressed();
    void flashSyncPressed();
    void ramUpdate();
    void ramGotoPressed();
    void ramSyncPressed();
    void memUpdate();
    void setMemDocks();
    void setVisualizerDocks();
    void setKeyHistoryDocks();
    void memLoadState();
    void memSync(HexWidget *edit);
    void memUpdateEdit(HexWidget *edit, bool force = false);
    void memGotoEdit(HexWidget *edit);
    void memGoto(HexWidget *edit, uint32_t addr);
    void memSearchEdit(HexWidget *edit);
    void memSyncEdit(HexWidget *edit);
    void memAsciiToggle(HexWidget *edit);
    void memDocksUpdate();
    HexWidget *firstMemWidget();
    void addMemDock(const QString &magic, int bytes, bool ascii);
    void addVisualizerDock(const QString &magic, const QString &config);
    void addKeyHistoryDock(const QString &magic, int size);

    // versions
    void setVersion();
    void checkVersion();
    bool isFirstRun();

    // options
    void optSend(CEmuOpts &o);
    void optLoadFiles(CEmuOpts &o);
    void optAttemptLoad(CEmuOpts &o);

    // lcd
    void lcdUpdate(double emuFps);
    void lcdAdjust();
    void lcdCopy();

    // resets and loads
    void emuLoad(emu_data_t type);
    void emuCheck(emu_state_t state, emu_data_t type);
    void resetEmu();
    void resetCEmu();
    void resetGui();
    void pauseEmu(Qt::ApplicationState state);

    // process communication
    bool ipcSetup();
    void ipcSpawn();
    void ipcCloseConnected();
    void ipcReceived();
    void ipcSetId();
    void ipcCli(QDataStream &stream);

    // context menus
    void contextLcd(const QPoint &posa);
    void contextConsole(const QPoint &posa);
    void contextDisasm(const QPoint &posa);
    void contextVars(const QPoint &posa);
    void contextVat(const QPoint &posa);
    void contextOp(const QPoint &posa);
    void contextMem(const QPoint &posa);
    void contextMemWidget(const QPoint &posa, uint32_t addr);

#ifdef _WIN32
    // Win32 Console Toggle
    void toggleConsole();
    void installToggleConsole();
#endif

    // Redistribute Docks
    DockWidget *redistributeFindDock(const QPoint &pos);
    bool redistributeDocks(const QPoint &pos, const QPoint &offset,
                           Qt::CursorShape cursorShape,
                           int (QSize::*dimension)() const,
                           Qt::Orientation orientation);
    void raiseContainingDock(QWidget *widget);

    // Members
    Ui::MainWindow *ui = Q_NULLPTR;
    EmuThread emu;
    CEmuOpts opts;
    InterCom com;

    bool m_isInDarkMode = false;

    int m_watchGUIMask = DBG_MASK_NONE;

    QString m_basicStringReference;

    QTranslator m_appTranslator;
    QLabel m_speedLabel;
    QLabel m_fpsLabel;
    QLabel m_msgLabel;
    QTextCursor m_disasmOffset;
    bool m_disasmOffsetSet;
    bool m_disasmPane;
    int32_t m_disasmAddr;
    uint32_t m_stackAddr;

    QString m_searchStr;
    int m_searchMode = SearchWidget::Hex;

    QDir m_dir;
    QStringList m_equateFiles;
    VarTableModel *m_varTableModel;
    std::function<void(bool)> m_recvAction;

    bool m_uiEditMode = false;
    bool m_portable = false;
    bool m_nativeConsole = false;
    bool m_shutdown = false;
    bool m_recording = false;

    bool m_basicTempOpen = false;
    QString m_basicVariableName;
    const QString *m_basicOriginalCode;
    const QString *m_basicFormattedCode;
    QString m_basicOriginalCodeTemp;
    QString m_basicFormattedCodeTemp;
    bool m_basicShowHighlighted = true;
    bool m_basicShowFetches = false;
    bool m_basicShowTempParser = false;
    bool m_basicShowLiveExecution = true;
    bool m_basicClearCache = false;
    QList<QList<token_highlight_t>> m_basicPrgmsTokensMap;
    QMap<QString, int> m_basicPrgmsMap;
    QStringList m_basicPrgmsOriginalCode;
    QStringList m_basicPrgmsFormattedCode;

    QTextEdit::ExtraSelection m_basicCurrToken;
    QTextEdit::ExtraSelection m_basicCurrLine;

    uint32_t m_prevDisasmAddr = 0;
    QPalette m_cBack, m_cNone;

    QString m_prevWatchLow;
    QString m_prevWatchHigh;
    QString m_prevBreakAddr;
    QString m_prevPortAddr;

    QShortcut *m_shortcutStepIn;
    QShortcut *m_shortcutStepOver;
    QShortcut *m_shortcutStepNext;
    QShortcut *m_shortcutStepOut;
    QShortcut *m_shortcutDebug;
    QShortcut *m_shortcutFullscreen;
    QShortcut *m_shortcutAsm;
    QShortcut *m_shortcutResend;

    QAction *m_actionToggleUI;
    QAction *m_actionAddMemory;
    QAction *m_actionAddVisualizer;

    QIcon m_iconRun, m_iconStop;
    QIcon m_iconSave, m_iconLoad;
    QIcon m_iconEdit, m_iconRemove;
    QIcon m_iconSearch, m_iconGoto;
    QIcon m_iconSync, m_iconAddMem, m_iconLcd;
    QIcon m_iconAscii, m_iconUiEdit;
    QIcon m_iconCheck, m_iconCheckGray;

    QString m_gotoAddr;
    QString m_flashGotoAddr;
    QString m_RamGotoAddr;
    QString m_memGotoAddr;

    QString m_pathConfig;
    QMenu *m_menuDocks;
    QMenu *m_menuDebug;

    KeyHistoryWidget *m_windowKeys = Q_NULLPTR;

    bool m_isSendingRom = false;
    QString m_dragRom;

    bool m_needReload = false;
    bool m_needFullReset = false;
    bool m_keepSetup = false;
    bool m_guiAdd = false;
    bool m_initPassed = true;
    bool m_useSoftCom = false;
    bool m_pauseOnFocus;
    bool m_loadedBootImage = false;
    bool m_optimizeRecording;
    bool m_portableActivated = false;
    bool m_ignoreDmaCycles;
    QList<int> m_supportedRevs;
    bool m_allowAnyRev;
    bool m_normalOs;
    bool m_setup = false;
    int m_fullscreen = FULLSCREEN_NONE;
    uint32_t m_runUntilAddr;

    QPushButton *m_btnCancelTranser;
    QProgressBar *m_progressBar;
    QStringList m_docksMemory;
    QList<int> m_docksMemoryBytes;
    QList<bool> m_docksMemoryAscii;
    QStringList m_docksKeyHistory;
    QList<int> m_docksKeyHistorySize;
    QStringList m_docksVisualizer;
    QStringList m_docksVisualizerConfig;
    QList<DockWidget*> m_dockPtrs;
    QSettings *m_config = Q_NULLPTR;
    QPointer<HexWidget> m_memWidget = Q_NULLPTR;

    QString m_pathRom;
    QString m_pathRam;
    QString m_pathImage;
    QTimer m_timerEmu;
    QTimer m_timerFps;
    bool m_timerEmuTriggered = false;
    bool m_timerFpsTriggered = false;

    QString m_styleForMode[2];

    static const char *m_varExtensions[];

    // Settings definitions
    static const QString SETTING_DEBUGGER_TEXT_SIZE;
    static const QString SETTING_DEBUGGER_DISASM_SPACE;
    static const QString SETTING_DEBUGGER_DISASM_TAB;
    static const QString SETTING_DEBUGGER_RESTORE_ON_OPEN;
    static const QString SETTING_DEBUGGER_SAVE_ON_CLOSE;
    static const QString SETTING_DEBUGGER_RESET_OPENS;
    static const QString SETTING_DEBUGGER_ENABLE_SOFT;
    static const QString SETTING_DEBUGGER_BOLD_SYMBOLS;
    static const QString SETTING_DEBUGGER_DATA_COL;
    static const QString SETTING_DEBUGGER_ADDR_COL;
    static const QString SETTING_DEBUGGER_IMPLICT;
    static const QString SETTING_DEBUGGER_UPPERCASE;
    static const QString SETTING_DEBUGGER_IMAGE_PATH;
    static const QString SETTING_DEBUGGER_FLASH_BYTES;
    static const QString SETTING_DEBUGGER_RAM_BYTES;
    static const QString SETTING_DEBUGGER_FLASH_ASCII;
    static const QString SETTING_DEBUGGER_RAM_ASCII;
    static const QString SETTING_DEBUGGER_BREAK_IGNORE;
    static const QString SETTING_DEBUGGER_IGNORE_DMA;
    static const QString SETTING_DEBUGGER_AUTO_EQUATES;
    static const QString SETTING_DEBUGGER_ALLOW_ANY_REV;
    static const QString SETTING_DEBUGGER_NORM_OS;
    static const QString SETTING_PYTHON_EDITION;
    static const QString SETTING_SCREEN_FRAMESKIP;
    static const QString SETTING_SCREEN_SCALE;
    static const QString SETTING_SCREEN_UPSCALE;
    static const QString SETTING_SCREEN_FULLSCREEN;
    static const QString SETTING_SCREEN_SKIN;
    static const QString SETTING_SCREEN_DMA;
    static const QString SETTING_SCREEN_GAMMA;
    static const QString SETTING_SCREEN_RESPONSE;
    static const QString SETTING_KEYPAD_KEYMAP;
    static const QString SETTING_KEYPAD_COLOR;
    static const QString SETTING_KEYPAD_HOLDING;
    static const QString SETTING_WINDOW_FULLSCREEN;
    static const QString SETTING_WINDOW_GROUP_DRAG;
    static const QString SETTING_WINDOW_STATE;
    static const QString SETTING_WINDOW_GEOMETRY;
    static const QString SETTING_WINDOW_SEPARATOR;
    static const QString SETTING_WINDOW_MENUBAR;
    static const QString SETTING_WINDOW_STATUSBAR;
    static const QString SETTING_WINDOW_POSITION;
    static const QString SETTING_WINDOW_MEMORY_DOCKS;
    static const QString SETTING_WINDOW_MEMORY_DOCK_BYTES;
    static const QString SETTING_WINDOW_MEMORY_DOCK_ASCII;
    static const QString SETTING_WINDOW_VISUALIZER_DOCKS;
    static const QString SETTING_WINDOW_VISUALIZER_CONFIG;
    static const QString SETTING_WINDOW_KEYHISTORY_DOCKS;
    static const QString SETTING_WINDOW_KEYHISTORY_CONFIG;
    static const QString SETTING_CAPTURE_FRAMESKIP;
    static const QString SETTING_CAPTURE_OPTIMIZE;
    static const QString SETTING_SLOT_NAMES;
    static const QString SETTING_SLOT_PATHS;
    static const QString SETTING_IMAGE_PATH;
    static const QString SETTING_ROM_PATH;
    static const QString SETTING_STATUS_INTERVAL;
    static const QString SETTING_FIRST_RUN;
    static const QString SETTING_UI_EDIT_MODE;
    static const QString SETTING_PAUSE_FOCUS;
    static const QString SETTING_SAVE_ON_CLOSE;
    static const QString SETTING_RESTORE_ON_OPEN;
    static const QString SETTING_EMUSPEED;
    static const QString SETTING_AUTOUPDATE;
    static const QString SETTING_ALWAYS_ON_TOP;
    static const QString SETTING_NATIVE_CONSOLE;
    static const QString SETTING_CURRENT_DIR;
    static const QString SETTING_ENABLE_WIN_CONSOLE;
    static const QString SETTING_RECENT_SAVE;
    static const QString SETTING_RECENT_PATHS;
    static const QString SETTING_RECENT_SELECT;

    static const QString SETTING_KEYPAD_NATURAL;
    static const QString SETTING_KEYPAD_CEMU;
    static const QString SETTING_KEYPAD_TILEM;
    static const QString SETTING_KEYPAD_WABBITEMU;
    static const QString SETTING_KEYPAD_JSTIFIED;
    static const QString SETTING_KEYPAD_SMARTPAD;
    static const QString SETTING_KEYPAD_CUSTOM;
    static const QString SETTING_KEYPAD_CUSTOM_PATH;

    static const QString SETTING_PREFERRED_LANG;
    static const QString SETTING_VERSION;

    static const QString SETTING_DEFAULT_CONFIG_FILE;
    static const QString SETTING_DEFAULT_ROM_FILE;
    static const QString SETTING_DEFAULT_IMAGE_FILE;
    static const QString SETTING_DEFAULT_DEBUG_FILE;

    static const QString TXT_YES;
    static const QString TXT_NO;
    static const QString TXT_NAN;
    static const QString DEBUG_UNSET_ADDR;
    static const QString DEBUG_UNSET_PORT;

    QString TITLE_DEBUG;
    QString TITLE_DOCKS;

    QString TXT_MEM_DOCK;
    QString TXT_VISUALIZER_DOCK;
    QString TXT_KEYHISTORY_DOCK;

    QString TXT_CLEAR_HISTORY;
    QString TXT_SIZE;

    QString TXT_CONSOLE;
    QString TXT_SETTINGS;
    QString TXT_VARIABLES;
    QString TXT_CAPTURE;
    QString TXT_STATE;
    QString TXT_KEYPAD;

    QString TXT_TI_BASIC_DEBUG;
    QString TXT_DEBUG_CONTROL;
    QString TXT_CPU_STATUS;
    QString TXT_DISASSEMBLY;
    QString TXT_MEMORY;
    QString TXT_TIMERS;
    QString TXT_BREAKPOINTS;
    QString TXT_WATCHPOINTS;
    QString TXT_PORTMON;
    QString TXT_OS_VIEW;
    QString TXT_OS_STACKS;
    QString TXT_MISC;
    QString TXT_AUTOTESTER;

    QString MSG_INFORMATION;
    QString MSG_WARNING;
    QString MSG_ERROR;
    QString MSG_ADD_MEMORY;
    QString MSG_ADD_VISUALIZER;
    QString MSG_EDIT_UI;

    QString ACTION_TOGGLE_BREAK;
    QString ACTION_TOGGLE_READ;
    QString ACTION_TOGGLE_WRITE;
    QString ACTION_TOGGLE_RW;
    QString ACTION_GOTO_MEMORY_VIEW;
    QString ACTION_GOTO_VAT_MEMORY_VIEW;
    QString ACTION_GOTO_DISASM_VIEW;
    QString ACTION_COPY_ADDR;
    QString ACTION_COPY_DATA;

    QTableWidget *m_breakpoints = Q_NULLPTR;
    QTableWidget *m_watchpoints = Q_NULLPTR;
    QTableWidget *m_ports = Q_NULLPTR;
    DataWidget *m_disasm = Q_NULLPTR;

#ifdef _WIN32
    QAction *actionToggleConsole;
    QString TXT_TOGGLE_CONSOLE;
#endif
};

#endif
