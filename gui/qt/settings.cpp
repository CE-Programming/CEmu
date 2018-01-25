#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "sendinghandler.h"
#include "dockwidget.h"
#include "utils.h"

#include "../../core/schedule.h"
#include "../../core/cpu.h"
#include "../../core/emu.h"
#include "../../core/link.h"
#include "../../core/debug/debug.h"

#include <cmath>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkAccessManager>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QScrollBar>
#include <QtNetwork/QNetworkReply>

#ifdef _MSC_VER
    #include <direct.h>
    #define chdir _chdir
#else
    #include <unistd.h>
#endif

const QString MainWindow::SETTING_DEBUGGER_TEXT_SIZE        = QStringLiteral("Debugger/text_size");
const QString MainWindow::SETTING_DEBUGGER_RESTORE_ON_OPEN  = QStringLiteral("Debugger/restore_on_open");
const QString MainWindow::SETTING_DEBUGGER_SAVE_ON_CLOSE    = QStringLiteral("Debugger/save_on_close");
const QString MainWindow::SETTING_DEBUGGER_RESET_OPENS      = QStringLiteral("Debugger/open_on_reset");
const QString MainWindow::SETTING_DEBUGGER_ENABLE_SOFT      = QStringLiteral("Debugger/enable_soft_commands");
const QString MainWindow::SETTING_DEBUGGER_BOLD_SYMBOLS     = QStringLiteral("Debugger/disasm_bold_symbols");
const QString MainWindow::SETTING_DEBUGGER_DISASM_SPACE     = QStringLiteral("Debugger/disasm_add_space");
const QString MainWindow::SETTING_DEBUGGER_ADDR_COL         = QStringLiteral("Debugger/disasm_addr_column");
const QString MainWindow::SETTING_DEBUGGER_DATA_COL         = QStringLiteral("Debugger/disasm_data_column");
const QString MainWindow::SETTING_DEBUGGER_IMPLICT          = QStringLiteral("Debugger/disasm_implict");
const QString MainWindow::SETTING_DEBUGGER_UPPERCASE        = QStringLiteral("Debugger/disasm_uppercase");
const QString MainWindow::SETTING_DEBUGGER_IMAGE_PATH       = QStringLiteral("Debugger/image_path");
const QString MainWindow::SETTING_DEBUGGER_FLASH_BYTES      = QStringLiteral("Debugger/flash_bytes_per_line");
const QString MainWindow::SETTING_DEBUGGER_RAM_BYTES        = QStringLiteral("Debugger/ram_bytes_per_line");
const QString MainWindow::SETTING_DEBUGGER_FLASH_ASCII      = QStringLiteral("Debugger/flash_ascii");
const QString MainWindow::SETTING_DEBUGGER_RAM_ASCII        = QStringLiteral("Debugger/ram_ascii");
const QString MainWindow::SETTING_DEBUGGER_BREAK_IGNORE     = QStringLiteral("Debugger/ignore_breakpoints");
const QString MainWindow::SETTING_DEBUGGER_AUTO_EQUATES     = QStringLiteral("Debugger/auto_equates");
const QString MainWindow::SETTING_DEBUGGER_IGNORE_DMA       = QStringLiteral("Debugger/ignore_dma");
const QString MainWindow::SETTING_DEBUGGER_NORM_OS          = QStringLiteral("Debugger/norm_os");
const QString MainWindow::SETTING_DEBUGGER_PRE_I            = QStringLiteral("Debugger/pre_i");
const QString MainWindow::SETTING_SCREEN_FRAMESKIP          = QStringLiteral("Screen/frameskip");
const QString MainWindow::SETTING_SCREEN_SCALE              = QStringLiteral("Screen/scale");
const QString MainWindow::SETTING_SCREEN_SKIN               = QStringLiteral("Screen/skin");
const QString MainWindow::SETTING_SCREEN_SPI                = QStringLiteral("Screen/spi");
const QString MainWindow::SETTING_KEYPAD_KEYMAP             = QStringLiteral("Keypad/map");
const QString MainWindow::SETTING_KEYPAD_COLOR              = QStringLiteral("Keypad/color");
const QString MainWindow::SETTING_KEYPAD_CUSTOM_PATH        = QStringLiteral("Keypad/custom_path");
const QString MainWindow::SETTING_WINDOW_GROUP_DRAG         = QStringLiteral("Window/group_dock_drag");
const QString MainWindow::SETTING_WINDOW_FULLSCREEN         = QStringLiteral("Window/fullscreen");
const QString MainWindow::SETTING_WINDOW_STATE              = QStringLiteral("Window/state");
const QString MainWindow::SETTING_WINDOW_GEOMETRY           = QStringLiteral("Window/geometry");
const QString MainWindow::SETTING_WINDOW_SEPARATOR          = QStringLiteral("Window/boundaries");
const QString MainWindow::SETTING_WINDOW_MENUBAR            = QStringLiteral("Window/menubar");
const QString MainWindow::SETTING_WINDOW_STATUSBAR          = QStringLiteral("Window/statusbar");
const QString MainWindow::SETTING_WINDOW_POSITION           = QStringLiteral("Window/position");
const QString MainWindow::SETTING_WINDOW_MEMORY_DOCKS       = QStringLiteral("Window/memory_docks");
const QString MainWindow::SETTING_WINDOW_MEMORY_DOCK_BYTES  = QStringLiteral("Window/memory_docks_bytes");
const QString MainWindow::SETTING_WINDOW_MEMORY_DOCK_ASCII  = QStringLiteral("Window/memory_docks_ascii");
const QString MainWindow::SETTING_WINDOW_VISUALIZER_DOCKS   = QStringLiteral("Window/visualizer_docks");
const QString MainWindow::SETTING_WINDOW_VISUALIZER_CONFIG  = QStringLiteral("Window/visualizer_config");
const QString MainWindow::SETTING_WINDOW_KEYHISTORY_DOCKS   = QStringLiteral("Window/keyhistory_docks");
const QString MainWindow::SETTING_WINDOW_KEYHISTORY_CONFIG  = QStringLiteral("Window/keyhistory_config");
const QString MainWindow::SETTING_CAPTURE_FRAMESKIP         = QStringLiteral("Capture/frameskip");
const QString MainWindow::SETTING_CAPTURE_OPTIMIZE          = QStringLiteral("Capture/optimize");
const QString MainWindow::SETTING_SLOT_NAMES                = QStringLiteral("Slot/names");
const QString MainWindow::SETTING_SLOT_PATHS                = QStringLiteral("Slot/paths");
const QString MainWindow::SETTING_IMAGE_PATH                = QStringLiteral("image_path");
const QString MainWindow::SETTING_ROM_PATH                  = QStringLiteral("rom_path");
const QString MainWindow::SETTING_STATUS_INTERVAL           = QStringLiteral("status_interval");
const QString MainWindow::SETTING_FIRST_RUN                 = QStringLiteral("first_run");
const QString MainWindow::SETTING_UI_EDIT_MODE              = QStringLiteral("ui_edit_mode");
const QString MainWindow::SETTING_PAUSE_FOCUS               = QStringLiteral("pause_on_focus_change");
const QString MainWindow::SETTING_SAVE_ON_CLOSE             = QStringLiteral("save_on_close");
const QString MainWindow::SETTING_RESTORE_ON_OPEN           = QStringLiteral("restore_on_open");
const QString MainWindow::SETTING_EMUSPEED                  = QStringLiteral("emulated_speed");
const QString MainWindow::SETTING_AUTOUPDATE                = QStringLiteral("check_for_updates");
const QString MainWindow::SETTING_ALWAYS_ON_TOP             = QStringLiteral("always_on_top");
const QString MainWindow::SETTING_NATIVE_CONSOLE            = QStringLiteral("native_console");
const QString MainWindow::SETTING_CURRENT_DIR               = QStringLiteral("current_directory");
const QString MainWindow::SETTING_ENABLE_WIN_CONSOLE        = QStringLiteral("enable_windows_console");
const QString MainWindow::SETTING_RECENT_SAVE               = QStringLiteral("Recent/save_paths");
const QString MainWindow::SETTING_RECENT_PATHS              = QStringLiteral("Recent/paths");
const QString MainWindow::SETTING_RECENT_SELECT             = QStringLiteral("Recent/selected");

const QString MainWindow::SETTING_KEYPAD_NATURAL            = QStringLiteral("natural");
const QString MainWindow::SETTING_KEYPAD_CEMU               = QStringLiteral("cemu");
const QString MainWindow::SETTING_KEYPAD_TILEM              = QStringLiteral("tilem");
const QString MainWindow::SETTING_KEYPAD_WABBITEMU          = QStringLiteral("wabbitemu");
const QString MainWindow::SETTING_KEYPAD_JSTIFIED           = QStringLiteral("jsTIfied");
const QString MainWindow::SETTING_KEYPAD_CUSTOM             = QStringLiteral("custom");

const QString MainWindow::SETTING_PREFERRED_LANG            = QStringLiteral("preferred_lang");
const QString MainWindow::SETTING_VERSION                   = QStringLiteral("version");

const QString MainWindow::SETTING_DEFAULT_CONFIG_FILE       = QStringLiteral("/cemu_config.ini");
const QString MainWindow::SETTING_DEFAULT_ROM_FILE          = QStringLiteral("/cemu_rom.rom");
const QString MainWindow::SETTING_DEFAULT_DEBUG_FILE        = QStringLiteral("/cemu_debug.ini");
const QString MainWindow::SETTING_DEFAULT_IMAGE_FILE        = QStringLiteral("/cemu_image.ce");
const QString MainWindow::TXT_YES                           = QStringLiteral("y");
const QString MainWindow::TXT_NO                            = QStringLiteral("n");
const QString MainWindow::TXT_NAN                           = QStringLiteral("NaN");

// In all cases, all paths in memory should be absolute paths.
// But in the UI or config file, they can be relative (to appDir()) if portable mode is enabled, or absolute otherwise.
void MainWindow::setPortable(bool state) {
    ui->checkPortable->blockSignals(true);
    ui->checkPortable->setChecked(state);
    ui->checkPortable->blockSignals(false);
    m_portable = state;

    delete m_config;
    m_config = Q_NULLPTR;

    const QDir dir = appDir();

    // Get all new paths, in absolute
    const QString newConfigPath = QFileInfo((state ? dir.path() : configPath) + SETTING_DEFAULT_CONFIG_FILE).absoluteFilePath();
    const QString newConfigDirPath = QDir::cleanPath(QFileInfo(newConfigPath).absolutePath());
    QString newDebugPath = newConfigDirPath + SETTING_DEFAULT_DEBUG_FILE;
    QString newImagePath = newConfigDirPath + SETTING_DEFAULT_IMAGE_FILE;
    QString newRomPath = m_pathRom; // No change here

    // Update the FS (still using absolute paths)
    QFile(m_pathConfig).copy(newConfigPath);
    QFile(m_pathImage).copy(newImagePath);

    // Remove old settings if previously portable
    if (!state) {
        QFile(m_pathConfig).remove();
        QFile(m_pathImage).remove();
    }

    // Update paths in memory (still using absolute paths)
    m_pathConfig = newConfigPath;
    m_pathImage = newImagePath;

    // Now changing the UI and qsettings content, with, if portable, paths relative to appDir()
    if (state) {
        newDebugPath = dir.relativeFilePath(newDebugPath);
        newImagePath = dir.relativeFilePath(newImagePath);
        newRomPath = dir.relativeFilePath(newRomPath);
    }

    // Update new QSettings (memory + FS)
    m_config = new QSettings(newConfigPath, QSettings::IniFormat); // Path is absolute
    m_config->setValue(SETTING_DEBUGGER_IMAGE_PATH, newDebugPath);
    m_config->setValue(SETTING_IMAGE_PATH, newImagePath);
    m_config->setValue(SETTING_ROM_PATH, newRomPath);
    m_config->sync();

    ui->pathDebug->setText(newDebugPath);
    ui->pathImage->setText(newImagePath);
    ui->pathRom->setText(newRomPath);
    ui->pathConfig->setText(newConfigPath);

    ui->buttonChangeSavedDebugPath->setEnabled(!state);
    ui->buttonChangeSavedImagePath->setEnabled(!state);
}

void MainWindow::setFrameskip(int value) {
    m_config->setValue(SETTING_CAPTURE_FRAMESKIP, value);
    ui->apngSkip->setValue(value);
    ui->apngSkipDisplay->setText(QString::number((ui->guiSkip->value() + 1) * (ui->apngSkip->value() + 1) - 1));
}

void MainWindow::setOptimizeRecord(bool state) {
    ui->checkOptimizeRecording->setChecked(state);
    m_config->setValue(SETTING_CAPTURE_OPTIMIZE, state);
    m_optimizeRecording = state;
}

void MainWindow::iconsLoad() {
    QString iconPath = QStringLiteral(":/icons/resources/icons/");
    m_iconStop.addPixmap(QPixmap(iconPath + QStringLiteral("stop.png")));
    m_iconRun.addPixmap(QPixmap(iconPath + QStringLiteral("run.png")));
    m_iconSave.addPixmap(QPixmap(iconPath + QStringLiteral("import.png")));
    m_iconLoad.addPixmap(QPixmap(iconPath + QStringLiteral("export.png")));
    m_iconEdit.addPixmap(QPixmap(iconPath + QStringLiteral("wizard.png")));
    m_iconRemove.addPixmap(QPixmap(iconPath + QStringLiteral("exit.png")));
    m_iconSearch.addPixmap(QPixmap(iconPath + QStringLiteral("search.png")));
    m_iconGoto.addPixmap(QPixmap(iconPath + QStringLiteral("goto.png")));
    m_iconSync.addPixmap(QPixmap(iconPath + QStringLiteral("refresh.png")));
    m_iconAddMem.addPixmap(QPixmap(iconPath + QStringLiteral("add_mem.png")));
    m_iconLcd.addPixmap(QPixmap(iconPath + QStringLiteral("lcd.png")));
    m_iconUiEdit.addPixmap(QPixmap(iconPath + QStringLiteral("ui_edit.png")));
    m_iconAscii.addPixmap(QPixmap(iconPath + QStringLiteral("characters.png")));
    m_iconCheck.addPixmap(QPixmap(iconPath + QStringLiteral("check.png")));
    m_iconCheckGray.addPixmap(QPixmap(iconPath + QStringLiteral("checkgray.png")));
    m_actionAddMemory->setIcon(m_iconAddMem);
    m_actionAddVisualizer->setIcon(m_iconLcd);
    m_actionToggleUI->setIcon(m_iconUiEdit);
}

bool MainWindow::bootImageCheck() {
    QDir dir = appDir();
    QDirIterator dirIt(dir, QDirIterator::NoIteratorFlags);
    while (dirIt.hasNext()) {
        dirIt.next();
        QString dirItFile = dirIt.filePath();
        if (QFileInfo(dirItFile).isFile()) {
            if (QFileInfo(dirItFile).suffix() == QStringLiteral("cemu")) {
                if (!m_loadedBootImage) {
                    m_loadedBootImage = bootImageImport(dirItFile);
                }
                QFile(dirItFile).remove();
            }
        }
    }
    return m_loadedBootImage;
}

bool MainWindow::bootImageImport(const QString &bootImagePath) {
    QString newConfigPath = configPath + SETTING_DEFAULT_CONFIG_FILE;
    QString romPath = configPath + SETTING_DEFAULT_ROM_FILE;
    QFile bootFile(bootImagePath);
    QFile romFile(romPath);
    romFile.remove();
    if (!romFile.open(QIODevice::WriteOnly)) { return false; }
    QFile settingsFile(newConfigPath);
    settingsFile.remove();
    if (!settingsFile.open(QIODevice::WriteOnly)) { return false; }
    bootFile.open(QIODevice::ReadOnly);
    QByteArray romData = bootFile.read(0x400000);
    QByteArray settingsData = bootFile.readAll();
    romFile.write(romData);
    settingsFile.write(settingsData);
    romFile.close();
    settingsFile.close();
    bootFile.close();
    m_pathConfig = newConfigPath;
    return true;
}

void MainWindow::bootImageExport() {
    QMessageBox::information(this, MSG_INFORMATION, tr("A bootable image can be used to start CEmu with predefined configurations, without the need for any extra setup."
                                                       "\n\nThe bootable image should be placed in the same directory as the CEmu executable. When CEmu is then started, "
                                                       "the boot image will be loaded automatically and then removed for convience."));

    QString path = QFileDialog::getSaveFileName(this, tr("Save bootable CEmu image"),
                                                m_dir.absolutePath(),
                                                tr("Bootable CEmu images (*.cemu);"));

    if (!path.isEmpty()) {
        m_dir = QFileInfo(path).absoluteDir();
        QFile romFile(m_pathRom);
        if (!romFile.open(QIODevice::ReadOnly)) {
            return;
        }
        QByteArray romData = romFile.readAll();

        QFile settingsFile(m_pathConfig);
        if (!settingsFile.open(QIODevice::ReadOnly)) {
            return;
        }
        QByteArray settingsData = settingsFile.readAll();

        QFile writter(path);
        writter.open(QIODevice::WriteOnly);
        writter.write(romData);
        writter.write(settingsData);
        romFile.close();
        settingsFile.close();
        writter.close();
    }
}

void MainWindow::setDebugSoftCommands(bool state) {
    ui->checkDisableSoftCommands->blockSignals(true);
    ui->checkDisableSoftCommands->setChecked(state);
    ui->checkDisableSoftCommands->blockSignals(false);
    m_config->setValue(SETTING_DEBUGGER_ENABLE_SOFT, state);
    debug_flag(DBG_SOFT_COMMANDS, state);
}

void MainWindow::setDebugDisasmBoldSymbols(bool state) {
    ui->checkDisasmBoldSymbols->setChecked(state);
    m_config->setValue(SETTING_DEBUGGER_BOLD_SYMBOLS, state);
    disasm.bold_sym = state;
    if (guiDebug) {
        disasmUpdate();
    }
}

void MainWindow::setDockGroupDrag(bool state) {
    ui->checkAllowGroupDrag->setChecked(state);
    m_config->setValue(SETTING_WINDOW_GROUP_DRAG, state);
    setUIDockEditMode(m_uiEditMode);
}

void MainWindow::setDebugDisasmDataCol(bool state) {
    ui->checkDisasmDataCol->setChecked(state);
    m_config->setValue(SETTING_DEBUGGER_DATA_COL, state);
    disasm.bytes = state;
    if (guiDebug) {
        disasmUpdate();
    }
}

void MainWindow::setDebugDisasmAddrCol(bool state) {
    ui->checkDisasmAddr->setChecked(state);
    m_config->setValue(SETTING_DEBUGGER_ADDR_COL, state);
    disasm.addr = state;
    if (guiDebug) {
        if (!disasm.addr) {
            disasmUpdate();
        } else {
            disasmUpdateAddr(cpu.registers.PC, true);
        }
    }
}

void MainWindow::setDebugDisasmImplict(bool state) {
    ui->checkDisasmImplict->setChecked(state);
    m_config->setValue(SETTING_DEBUGGER_IMPLICT, state);
    disasm.implicit = state;
    if (guiDebug) {
        disasmUpdate();
    }
}

void MainWindow::setDebugDisasmUppercase(bool state) {
    ui->checkDisasmUppercase->setChecked(state);
    m_config->setValue(SETTING_DEBUGGER_UPPERCASE, state);
    disasm.uppercase = state;
    if (guiDebug) {
        disasmUpdate();
    }
}

void MainWindow::setDebugDisasmSpace(bool state) {
    ui->checkAddSpace->setChecked(state);
    m_config->setValue(SETTING_DEBUGGER_DISASM_SPACE, state);
    disasm.comma = state ? ", " : ",";
    if (guiDebug) {
        disasmUpdate();
    }
}

void MainWindow::setLcdSpi(bool state) {
    ui->checkSpi->setChecked(state);
    m_config->setValue(SETTING_SCREEN_SPI, state);
    emu_set_lcd_spi(state == false ? 0 : 1);
    emit setLcdMode(state);
}

void MainWindow::setLcdDma(bool state) {
    ui->checkDma->setChecked(state);
    m_config->setValue(SETTING_DEBUGGER_IGNORE_DMA, !state);
    bool ok;
    int64_t cycleCounter = ui->cycleView->text().toLongLong(&ok);
    if (!ok)
        cycleCounter = debug.totalCycles - (!state ? debug.dmaCycles : 0);
    else if (!state)
        cycleCounter -= debug.dmaCycles;
    else
        cycleCounter += debug.dmaCycles;
    ui->cycleView->setText(QString::number(cycleCounter));
    m_ignoreDmaCycles = !state;
}

void MainWindow::setFocusSetting(bool state) {
    ui->checkFocus->setChecked(state);
    m_config->setValue(SETTING_PAUSE_FOCUS, state);
    m_pauseOnFocus = state;
}

void MainWindow::memLoadState() {
    ui->ramBytes->setValue(m_config->value(SETTING_DEBUGGER_RAM_BYTES, 8).toInt());
    ui->flashBytes->setValue(m_config->value(SETTING_DEBUGGER_FLASH_BYTES, 8).toInt());
    ui->ramAscii->setChecked(m_config->value(SETTING_DEBUGGER_RAM_ASCII, true).toBool());
    ui->flashAscii->setChecked(m_config->value(SETTING_DEBUGGER_FLASH_ASCII, true).toBool());
    ui->ramEdit->setAsciiArea(ui->ramAscii->isChecked());
    ui->flashEdit->setAsciiArea(ui->flashAscii->isChecked());
}

void MainWindow::setMenuBarState(bool state) {
    ui->menubar->setHidden(state);
    ui->actionHideMenuBar->setChecked(state);
    m_config->setValue(SETTING_WINDOW_MENUBAR, state);
}

void MainWindow::setStatusBarState(bool state) {
    ui->statusBar->setHidden(state);
    ui->actionHideStatusBar->setChecked(state);
    m_config->setValue(SETTING_WINDOW_STATUSBAR, state);
}

void MainWindow::setDebugIgnoreBreakpoints(bool state) {
    ui->buttonToggleBreakpoints->setChecked(state);
    m_config->setValue(SETTING_DEBUGGER_BREAK_IGNORE, state);
    debug_flag(DBG_IGNORE, state);
}

void MainWindow::setDebugResetTrigger(bool state) {
    ui->checkDebugResetTrigger->setChecked(state);
    m_config->setValue(SETTING_DEBUGGER_RESET_OPENS, state);
    debug_flag(DBG_OPEN_ON_RESET, state);
}

void MainWindow::setAutoSave(bool state) {
    ui->checkSaveRestore->setChecked(state);
    m_config->setValue(SETTING_RESTORE_ON_OPEN, state);
    m_config->setValue(SETTING_SAVE_ON_CLOSE, state);
}

void MainWindow::setDebugAutoSave(bool state) {
    ui->checkSaveLoadDebug->setChecked(state);
    m_config->setValue(SETTING_DEBUGGER_SAVE_ON_CLOSE, state);
    m_config->setValue(SETTING_DEBUGGER_RESTORE_ON_OPEN, state);
}

void MainWindow::setFont(int fontSize) {
    ui->textSize->setValue(fontSize);
    m_config->setValue(SETTING_DEBUGGER_TEXT_SIZE, ui->textSize->value());

    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    monospace.setPointSize(fontSize);
    ui->console->setFont(monospace);
    m_disasm->setFont(monospace);
    ui->sources->setSourceFont(monospace);
    ui->stackView->setFont(monospace);

    ui->afregView->setFont(monospace);
    ui->hlregView->setFont(monospace);
    ui->deregView->setFont(monospace);
    ui->bcregView->setFont(monospace);
    ui->ixregView->setFont(monospace);
    ui->iyregView->setFont(monospace);
    ui->af_regView->setFont(monospace);
    ui->hl_regView->setFont(monospace);
    ui->de_regView->setFont(monospace);
    ui->bc_regView->setFont(monospace);
    ui->splregView->setFont(monospace);
    ui->spsregView->setFont(monospace);
    ui->mbregView->setFont(monospace);
    ui->iregView->setFont(monospace);
    ui->rregView->setFont(monospace);
    ui->imregView->setFont(monospace);
    ui->freqView->setFont(monospace);
    ui->pcregView->setFont(monospace);
    ui->lcdbaseView->setFont(monospace);
    ui->lcdcurrView->setFont(monospace);
    ui->cycleView->setFont(monospace);
}

void MainWindow::setKeypadColor(unsigned int color) {
    ui->keypadWidget->setType(get_device_type(), color);
    m_config->setValue(SETTING_KEYPAD_COLOR, color);
}

void MainWindow::setCalcSkinTopFromType() {
    bool is83 = get_device_type() == TI83PCE;
    ui->calcSkinTop->setStyleSheet(is83 ? QStringLiteral(".QFrame { border-image: url(:/skin/resources/skin/ti83pce.png) 0 0 0 0 stretch stretch; }")
                                        : QStringLiteral(".QFrame { border-image: url(:/skin/resources/skin/ti84pce.png) 0 0 0 0 stretch stretch; }"));
}

void MainWindow::setImagePath() {
    QString saveImagePath = QFileDialog::getSaveFileName(this, tr("Set saved image to restore from"),
                                                           m_dir.absolutePath(),
                                                           tr("CEmu images (*.ce);;All files (*.*)"));
    if (!saveImagePath.isEmpty()) {
        m_dir = QFileInfo(saveImagePath).absoluteDir();
        m_config->setValue(SETTING_IMAGE_PATH, saveImagePath);
        ui->pathImage->setText(saveImagePath);
    }
}

void MainWindow::setDebugPath() {
    QString savePath = QFileDialog::getSaveFileName(this, tr("Set debugging information path"),
                                                           m_dir.absolutePath(),
                                                           tr("Debugging information (*.ini);;All files (*.*)"));
    if (!savePath.isEmpty()) {
        m_dir = QFileInfo(savePath).absoluteDir();
        m_config->setValue(SETTING_DEBUGGER_IMAGE_PATH, savePath);
        ui->pathDebug->setText(savePath);
    }
}

void MainWindow::setUIDocks() {

    // Create "Docks" menu to make closing and opening docks more intuitive
    m_menuDocks = new QMenu(TITLE_DOCKS, this);
    ui->menubar->insertMenu(ui->menuExtras->menuAction(), m_menuDocks);

    // Convert any current docks
    for (const auto &dock : findChildren<DockWidget*>()) {
        dock->setState(m_uiEditMode);
    }

    // Add the screen action
    QAction *action = ui->screenDock->toggleViewAction();
    action->setIcon(m_iconLcd);

    // Convert the tabs into DockWidgets
    while (ui->tabWidget->count()) {
        DockWidget *dw = new DockWidget(ui->tabWidget, this);

        // Fill "Docks" menu
        QAction *action = dw->toggleViewAction();
        action->setIcon(dw->windowIcon());
        m_menuDocks->addAction(action);

        dw->setState(m_uiEditMode);
        addDockWidget(Qt::RightDockWidgetArea, dw);
        if (!m_dockPtrs.isEmpty()) {
            tabifyDockWidget(m_dockPtrs.back(), dw);
        }

        m_dockPtrs.append(dw);
    }

    m_menuDocks->addAction(action);
    m_menuDocks->addSeparator();

    m_menuDebug = new QMenu(TITLE_DEBUG, this);
    ui->menubar->insertMenu(ui->menuExtras->menuAction(), m_menuDebug);

    // Convert the tabs into QDockWidgets
    while (ui->tabDebug->count()) {
        DockWidget *dw = new DockWidget(ui->tabDebug, this);

        // Fill "Docks" menu
        QAction *action = dw->toggleViewAction();
        action->setIcon(dw->windowIcon());
        m_menuDebug->addAction(action);

        dw->setState(m_uiEditMode);
        addDockWidget(Qt::RightDockWidgetArea, dw);
        if (isFirstRun() || !opts.useSettings) {
            dw->hide();
            dw->close();
        }
    }

    // Load removable docks
    setMemDocks();
    setVisualizerDocks();
    setKeyHistoryDocks();

    m_menuDebug->addSeparator();
    m_menuDebug->addAction(m_actionAddMemory);
    m_menuDebug->addAction(m_actionAddVisualizer);

    m_menuDocks->addSeparator();
    m_menuDocks->addAction(m_actionToggleUI);

    ui->tabWidget->close();
    ui->tabDebug->close();

    m_dockPtrs.append(ui->screenDock);

    // hide all the docks
    for (const auto &dock : findChildren<DockWidget*>()) {
        dock->setVisible(false);
    }
}

void MainWindow::setUIDockEditMode(bool mode) {
    if (mode) {
        if (m_config->value(SETTING_WINDOW_GROUP_DRAG).toBool()) {
            setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
                | QMainWindow::GroupedDragging
#endif
            );
        } else {
            setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);
        }
    } else {
        setDockOptions(0);
    }
    for (const auto &dock : findChildren<DockWidget*>()) {
        dock->setState(mode);
    }
}

void MainWindow::setUIEditMode(bool mode) {
    m_uiEditMode = mode;
    m_config->setValue(SETTING_UI_EDIT_MODE, mode);
    m_actionToggleUI->setChecked(mode);
    m_actionAddMemory->setEnabled(mode);
    m_actionAddVisualizer->setEnabled(mode);
    setUIBoundaries(mode);
    setUIDockEditMode(mode);
}

void MainWindow::setThrottle(int mode) {
    ui->checkThrottle->setChecked(mode == Qt::Checked);
    emu.setThrottle(mode == Qt::Checked);
}

void MainWindow::setAutoUpdates(int state) {
    m_config->setValue(SETTING_AUTOUPDATE, state);
    ui->checkUpdates->setChecked(state);

    if (state == Qt::Checked) {
        checkUpdate(false);
    }
}

void MainWindow::checkUpdate(bool forceInfoBox) {
    if (!CEMU_RELEASE) {
        if (forceInfoBox) {
            QMessageBox::warning(this, MSG_WARNING, tr("Checking updates is disabled for development builds"));
        }
        return;
    }

    static const QString currentVersionReleaseURL = QStringLiteral("https://github.com/CE-Programming/CEmu/releases/tag/" CEMU_VERSION);
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply) {
        QString newVersionURL = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
        if (!newVersionURL.isEmpty()) {
            if (newVersionURL.compare(currentVersionReleaseURL) == 0) {
                if (forceInfoBox) {
                    QMessageBox::information(this, tr("No update available"), tr("You already have the latest CEmu version") + QStringLiteral(" (" CEMU_VERSION ")"));
                }
            } else {
                QMessageBox updateInfoBox(this);
                updateInfoBox.addButton(QMessageBox::Ok);
                updateInfoBox.setIconPixmap(QPixmap(QStringLiteral(":/icons/resources/icons/icon.png")));
                updateInfoBox.setWindowTitle(tr("CEmu update"));
                updateInfoBox.setText(tr("<b>A new version of CEmu is available!</b>"
                                         "<br/>"
                                         "You can <a href='%1'>download it here</a>.")
                                     .arg(newVersionURL));
                updateInfoBox.setTextFormat(Qt::RichText);
                updateInfoBox.show();
                updateInfoBox.exec();
            }
        } else { // No internet connection? GitHub doesn't provide this redirection anymore?
            if (forceInfoBox) {
                QMessageBox updateInfoBox(this);
                updateInfoBox.addButton(QMessageBox::Ok);
                updateInfoBox.setIcon(QMessageBox::Warning);
                updateInfoBox.setWindowTitle(tr("Update check failed"));
                updateInfoBox.setText(tr("<b>An error occurred while checking for CEmu updates.</b>"
                                         "<br/>"
                                         "You can however <a href='https://github.com/CE-Programming/CEmu/releases/latest'>go here</a> to check yourself."));
                updateInfoBox.setTextFormat(Qt::RichText);
                updateInfoBox.show();
                updateInfoBox.exec();
            }
        }
    });

    manager->get(QNetworkRequest(QUrl(QStringLiteral("https://github.com/CE-Programming/CEmu/releases/latest"))));
}

void MainWindow::lcdAdjust() {
    float scale = ui->scaleLCD->value() / 100.0;
    bool skin = ui->checkSkin->isChecked();
    ui->calcSkinTop->setVisible(skin);
    float w, h;
    w = LCD_WIDTH * scale;
    h = LCD_HEIGHT * scale;
    ui->lcd->setFixedSize(w, h);
    ui->lcd->move(skin ? 60 * scale : 0, skin ? 78 * scale : 0);
    if (skin) {
        w = 440 * scale;
        h = 351 * scale;
    }
    ui->calcSkinTop->resize(w, h);
    ui->screenWidget->setFixedSize(w, h);
}

void MainWindow::setLcdScale(int scale) {
    int roundedScale = round(scale / 10.0) * 10;
    m_config->setValue(SETTING_SCREEN_SCALE, roundedScale);
    ui->scaleLCD->setValue(roundedScale);
    lcdAdjust();
}

void MainWindow::setSkinToggle(bool enable) {
    m_config->setValue(SETTING_SCREEN_SKIN, enable);
    ui->checkSkin->setChecked(enable);
    lcdAdjust();
}

void MainWindow::setDebugAutoEquates(bool enable) {
    m_config->setValue(SETTING_DEBUGGER_AUTO_EQUATES, enable);
    ui->checkAutoEquates->setChecked(enable);
    sendingHandler->setLoadEquates(enable);
}

void MainWindow::setGuiSkip(int value) {
    m_config->setValue(SETTING_SCREEN_FRAMESKIP, value);
    ui->guiSkip->blockSignals(true);
    ui->guiSkip->setValue(value);
    ui->guiSkip->blockSignals(false);
    ui->apngSkipDisplay->setText(QString::number((ui->guiSkip->value() + 1) * (ui->apngSkip->value() + 1) - 1));
    emit setLcdFrameskip(value);
}

void MainWindow::setStatusInterval(int value) {
    m_config->setValue(SETTING_STATUS_INTERVAL, value);
    ui->statusInterval->setValue(value);
    m_timerFps.stop();
    m_timerEmu.stop();
    m_timerFpsTriggered = true;
    m_timerEmuTriggered = true;
    m_timerEmuTriggerable = true;
    m_timerFpsTriggerable = true;
    if (!value) {
        m_timerEmuTriggerable = false;
        m_timerFpsTriggerable = false;
    } else {
        m_timerFps.start(value * 1000);
        m_timerEmu.start(value * 1000);
    }
}

void MainWindow::setEmuSpeed(int value) {
    m_config->setValue(SETTING_EMUSPEED, value);
    ui->emulationSpeedSpin->blockSignals(true);
    ui->emulationSpeedSpin->setValue(value);
    ui->emulationSpeedSpin->blockSignals(false);
    ui->emulationSpeed->setValue(value);
    emu.setSpeed(value);
    if (value == 0) {
      m_timerEmuTriggered = true;
      showEmuSpeed(0);
    }
}

void MainWindow::keypadChanged() {
    QString name = sender()->objectName();
    unsigned int color = KEYPAD_BLACK;

    if      (name == QStringLiteral("buttonWhite"))        color = KEYPAD_WHITE;
    else if (name == QStringLiteral("buttonBlack"))        color = KEYPAD_BLACK;
    else if (name == QStringLiteral("buttonGolden"))       color = KEYPAD_GOLDEN;
    else if (name == QStringLiteral("buttonPlum"))         color = KEYPAD_PLUM;
    else if (name == QStringLiteral("buttonPink"))         color = KEYPAD_PINK;
    else if (name == QStringLiteral("buttonRed"))          color = KEYPAD_RED;
    else if (name == QStringLiteral("buttonLightning"))    color = KEYPAD_LIGHTNING;
    else if (name == QStringLiteral("buttonTrueBlue"))     color = KEYPAD_TRUE_BLUE;
    else if (name == QStringLiteral("buttonDenim"))        color = KEYPAD_DENIM;
    else if (name == QStringLiteral("buttonSilver"))       color = KEYPAD_SILVER;
    else if (name == QStringLiteral("buttonSpaceGrey"))    color = KEYPAD_SPACEGREY;
    else if (name == QStringLiteral("buttonCoral"))        color = KEYPAD_CORAL;
    else if (name == QStringLiteral("buttonMint"))         color = KEYPAD_MINT;
    else if (name == QStringLiteral("buttonRoseGold"))     color = KEYPAD_ROSEGOLD;
    else if (name == QStringLiteral("buttonCrystalClear")) color = KEYPAD_CRYSTALCLEAR;

    setKeypadColor(color);
}

void MainWindow::keymapCustomSelected() {
    QFileDialog dialog(this);

    m_config->remove(SETTING_KEYPAD_CUSTOM_PATH);

    dialog.setDirectory(m_dir);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("Keymap Config (*.ini);;All files (*.*)"));

    if (!dialog.exec()) {
        ui->radioCEmuKeys->setChecked(true);
        return;
    }

    m_dir = dialog.directory().absolutePath();
    const auto& selectedFiles = dialog.selectedFiles();
    if (selectedFiles.empty()) {
        ui->radioCEmuKeys->setChecked(true);
        return;
    }

    m_config->setValue(SETTING_KEYPAD_CUSTOM_PATH, selectedFiles.first());
    setKeymap(SETTING_KEYPAD_CUSTOM);
}

void MainWindow::keymapChanged() {
    if (ui->radioNaturalKeys->isChecked()) {
        setKeymap(SETTING_KEYPAD_NATURAL);
    } else if (ui->radioCEmuKeys->isChecked()) {
        setKeymap(SETTING_KEYPAD_CEMU);
    } else if (ui->radioTilEmKeys->isChecked()) {
        setKeymap(SETTING_KEYPAD_TILEM);
    } else if (ui->radioWabbitemuKeys->isChecked()) {
        setKeymap(SETTING_KEYPAD_WABBITEMU);
    } else if (ui->radiojsTIfiedKeys->isChecked()) {
        setKeymap(SETTING_KEYPAD_JSTIFIED);
    } else if (ui->radioCustomKeys->isChecked()) {
        setKeymap(SETTING_KEYPAD_CUSTOM);
    }
}

void MainWindow::setKeymap(const QString &value) {
    QtKeypadBridge::KeymapMode mode;
    QString map = value;
    QString customPath = m_config->value(SETTING_KEYPAD_CUSTOM_PATH, QString()).toString();
    if (!SETTING_KEYPAD_CUSTOM.compare(map, Qt::CaseInsensitive)) {
        if (customPath.isEmpty() || !fileExists(customPath)) {
            map = SETTING_KEYPAD_CEMU;
        } else if (!keypadBridge->keymapImport(customPath)) {
            QMessageBox::warning(this, MSG_WARNING, tr("Unable to set custom keymap."));
            map = SETTING_KEYPAD_CEMU;
        }
    }
    if (!SETTING_KEYPAD_NATURAL.compare(map, Qt::CaseInsensitive)) {
        mode = QtKeypadBridge::KEYMAP_NATURAL;
    } else if (!SETTING_KEYPAD_CEMU.compare(map, Qt::CaseInsensitive)) {
        mode = QtKeypadBridge::KEYMAP_CEMU;
    } else if (!SETTING_KEYPAD_TILEM.compare(map, Qt::CaseInsensitive)) {
        mode = QtKeypadBridge::KEYMAP_TILEM;
    } else if (!SETTING_KEYPAD_WABBITEMU.compare(map, Qt::CaseInsensitive)) {
        mode = QtKeypadBridge::KEYMAP_WABBITEMU;
    } else if (!SETTING_KEYPAD_JSTIFIED.compare(map, Qt::CaseInsensitive)) {
        mode = QtKeypadBridge::KEYMAP_JSTIFIED;
    } else {
        mode = QtKeypadBridge::KEYMAP_CUSTOM;
    }
    m_config->setValue(SETTING_KEYPAD_KEYMAP, map);
    keypadBridge->setKeymap(mode);
    if (map == SETTING_KEYPAD_CEMU) {
        ui->radioCEmuKeys->setChecked(true);
    }
}

void MainWindow::keymapLoad() {
    QString currKeyMap = m_config->value(SETTING_KEYPAD_KEYMAP, SETTING_KEYPAD_CEMU).toString();
    if (!SETTING_KEYPAD_NATURAL.compare(currKeyMap, Qt::CaseInsensitive)) {
        ui->radioNaturalKeys->setChecked(true);
    } else if (!SETTING_KEYPAD_CEMU.compare(currKeyMap, Qt::CaseInsensitive)) {
        ui->radioCEmuKeys->setChecked(true);
    } else if (!SETTING_KEYPAD_TILEM.compare(currKeyMap, Qt::CaseInsensitive)) {
        ui->radioTilEmKeys->setChecked(true);
    } else if (!SETTING_KEYPAD_WABBITEMU.compare(currKeyMap, Qt::CaseInsensitive)) {
        ui->radioWabbitemuKeys->setChecked(true);
    } else if (!SETTING_KEYPAD_JSTIFIED.compare(currKeyMap, Qt::CaseInsensitive)) {
        ui->radiojsTIfiedKeys->setChecked(true);
    } else if (!SETTING_KEYPAD_CUSTOM.compare(currKeyMap, Qt::CaseInsensitive)) {
        ui->radioCustomKeys->setChecked(true);
    }
    setKeymap(currKeyMap);
}

void MainWindow::setFullscreen(int value) {
    static QWidget *parentPtr = Q_NULLPTR;
    if (parentPtr == Q_NULLPTR) {
        parentPtr = ui->lcd->parentWidget();
    }
    switch (value) {
        default:
        case FULLSCREEN_NONE:
            showNormal();
            ui->lcd->setParent(parentPtr);
            ui->lcd->showNormal();
            lcdAdjust();
            m_fullscreen = FULLSCREEN_NONE;
            break;
        case FULLSCREEN_ALL:
            showFullScreen();
            m_fullscreen = FULLSCREEN_ALL;
            break;
        case FULLSCREEN_LCD:
            ui->lcd->setParent(this, Qt::Tool | Qt::FramelessWindowHint | Qt::CustomizeWindowHint);
            ui->lcd->setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            ui->lcd->showFullScreen();
            ui->lcd->installEventFilter(keypadBridge);
            ui->lcd->setFocus();
            m_fullscreen = FULLSCREEN_LCD;
            break;
    }
}

void MainWindow::setTop(bool state) {
    if (state) {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    } else {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    }
    if (m_setup) {
        show();
    }
    m_config->setValue(SETTING_ALWAYS_ON_TOP, state);
    ui->checkAlwaysOnTop->setCheckState(state ? Qt::Checked : Qt::Unchecked);
}

void MainWindow::stateSaveInfo() {
    QStringList slotNames;
    QStringList slotPaths;

    for (int i = 0; i < ui->slotView->rowCount(); i++) {
        slotNames.append(ui->slotView->item(i, SLOT_NAME_COL)->text());
        slotPaths.append(ui->slotView->item(i, SLOT_EDIT_COL)->data(Qt::UserRole).toString());
    }

    m_config->setValue(SETTING_SLOT_NAMES, slotNames);
    m_config->setValue(SETTING_SLOT_PATHS, slotPaths);
}

void MainWindow::stateLoadInfo() {
    QStringList slotNames = m_config->value(SETTING_SLOT_NAMES).toStringList();
    QStringList slotPaths = m_config->value(SETTING_SLOT_PATHS).toStringList();

    for (int i = 0; i < slotNames.size(); i++) {
        QString name = slotNames.at(i);
        QString path = slotPaths.at(i);
        stateAdd(name, path);
    }
}

void MainWindow::setRecentSave(bool state) {
    ui->checkSaveRecent->setChecked(state);
    m_config->setValue(SETTING_RECENT_SAVE, state);
}

void MainWindow::setPreRevisionI(bool state) {
    ui->checkPreI->setChecked(state);
    m_config->setValue(SETTING_DEBUGGER_PRE_I, state);
    cpu.preI = state;
}

void MainWindow::setNormalOs(bool state) {
    ui->checkNormOs->setChecked(state);
    m_config->setValue(SETTING_DEBUGGER_NORM_OS, state);
    m_normalOs = state;
    if (guiDebug) {
        ui->opView->setEnabled(state);
        ui->vatView->setEnabled(state);
        ui->opStack->setEnabled(state);
        ui->fpStack->setEnabled(state);
        if (state) {
            osUpdate();
        }
    }
}

void MainWindow::setUIBoundaries(bool state) {
    m_config->setValue(SETTING_WINDOW_SEPARATOR, state);
    if (state) {
        setStyleSheet(QStringLiteral("QMainWindow::separator{ width: 4px; height: 4px; }"));
    } else {
        setStyleSheet(QStringLiteral("QMainWindow::separator{ width: 0px; height: 0px; }"));
    }
}

void MainWindow::recentSaveInfo() {
    QStringList paths;
    QStringList selects;

    if (m_config->value(SETTING_RECENT_SAVE).toBool()) {
        for (int i = 0; i < ui->varLoadedView->rowCount(); i++) {
            paths.append(ui->varLoadedView->item(i, RECENT_PATH_COL)->text());
            selects.append(static_cast<QAbstractButton *>(ui->varLoadedView->cellWidget(i, RECENT_SELECT_COL))->isChecked() ? TXT_YES : TXT_NO);
        }
    }

    m_config->setValue(SETTING_RECENT_PATHS, paths);
    m_config->setValue(SETTING_RECENT_SELECT, selects);
}

void MainWindow::recentLoadInfo() {
    QStringList paths = m_config->value(SETTING_RECENT_PATHS).toStringList();
    QStringList selects = m_config->value(SETTING_RECENT_SELECT).toStringList();

    if (m_config->value(SETTING_RECENT_SAVE).toBool()) {
        for (int i = 0; i < paths.size(); i++) {
            const QString path = paths.at(i);
            bool select = selects.at(i) == TXT_YES;
            sendingHandler->addFile(path, select);
        }
    }
}

void MainWindow::setMemDocks() {
    QStringList names = m_config->value(SETTING_WINDOW_MEMORY_DOCKS).toStringList();
    QList<int> bytes = m_config->value(SETTING_WINDOW_MEMORY_DOCK_BYTES).value<QList<int>>();
    QList<bool> ascii = m_config->value(SETTING_WINDOW_MEMORY_DOCK_ASCII).value<QList<bool>>();

    if (names.length() != bytes.length()) {
        return;
    }

    for (int i = 0; i < names.length(); i++) {
        addMemDock(names.at(i), bytes.at(i), ascii.at(i));
    }
}

void MainWindow::setVisualizerDocks() {
    QStringList names = m_config->value(SETTING_WINDOW_VISUALIZER_DOCKS).toStringList();
    QStringList configs = m_config->value(SETTING_WINDOW_VISUALIZER_CONFIG).toStringList();

    if (names.length() != configs.length()) {
        return;
    }

    for (int i = 0; i < names.length(); i++) {
        addVisualizerDock(names.at(i), configs.at(i));
    }
}

void MainWindow::setKeyHistoryDocks() {
    QStringList names = m_config->value(SETTING_WINDOW_KEYHISTORY_DOCKS).toStringList();
    QList<int> sizes = m_config->value(SETTING_WINDOW_KEYHISTORY_CONFIG).value<QList<int>>();

    if (names.length() != sizes.length()) {
        return;
    }

    for (int i = 0; i < names.length(); i++) {
        addKeyHistoryDock(names.at(i), sizes.at(i));
    }
}


void MainWindow::addKeyHistoryDock(const QString &magic, int size) {
    if (m_docksKeyHistory.contains(magic)) {
        return;
    }

    m_docksKeyHistory.append(magic);
    m_docksKeyHistorySize.append(size);

    DockWidget *dw = new DockWidget(TXT_KEYHISTORY_DOCK, this);
    KeyHistoryWidget *widget = new KeyHistoryWidget(this, size);

    dw->makeCloseableFloat(true);

    if (m_setup) {
        dw->setFloating(true);
        dw->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, dw->minimumSize(), qApp->desktop()->availableGeometry()));
    }

    connect(ui->keypadWidget, &KeypadWidget::keyPressed, widget, &KeyHistoryWidget::add);
    connect(widget, &KeyHistoryWidget::fontSizeChanged, [this, widget, magic]{
        int index;
        if ((index = m_docksKeyHistory.indexOf(magic)) != -1) {
            m_docksKeyHistorySize[index] = widget->getFontSize();
        }
    });
    connect(dw, &DockWidget::closed, [this, magic]{
        int index;
        if ((index = m_docksKeyHistory.indexOf(magic)) != -1) {
            m_docksKeyHistory.removeAt(index);
            m_docksKeyHistorySize.removeAt(index);
        }
    });

    dw->setState(m_uiEditMode);
    addDockWidget(Qt::RightDockWidgetArea, dw);
    dw->setObjectName(magic);
    dw->setWidget(widget);

    if (m_setup) {
        dw->show();
        dw->activateWindow();
        dw->raise();
    }
}

void MainWindow::setVersion() {
    m_config->setValue(SETTING_VERSION, QStringLiteral(CEMU_VERSION));
}

void MainWindow::checkVersion() {
    bool ask = false;

    if (isFirstRun()) {
        setVersion();
        return;
    }

    if (m_config->contains(SETTING_VERSION)) {
        if (m_config->value(SETTING_VERSION, QStringLiteral(CEMU_VERSION)).toString().compare(QStringLiteral(CEMU_VERSION))) {
            ask = true;
        }
    } else {
        ask = true;
    }

    if (ask) {
        if (CEMU_RELEASE) {
            setAutoUpdates(true);
        }

        m_keepSetup = true;
        QCheckBox *cb = new QCheckBox(tr("Keep migratable settings"));
        cb->setChecked(true);
        QMessageBox msgbox;
        msgbox.setText(tr("This version of CEmu is not compatible with your settings, probably made by an older version. "
                          "Would you like to erase them to prevent any unexpected behavior?"));
        msgbox.setIcon(QMessageBox::Icon::Question);
        msgbox.addButton(QMessageBox::Yes);
        msgbox.addButton(QMessageBox::No);
        msgbox.setDefaultButton(QMessageBox::Yes);
        msgbox.setCheckBox(cb);

        connect(cb, &QCheckBox::stateChanged, [this](int state) {
            m_keepSetup = static_cast<Qt::CheckState>(state) == Qt::CheckState::Checked;
        });

        if (msgbox.exec() == QMessageBox::Yes) {
            resetCEmu();
        }
        setVersion();
    }
}

void MainWindow::saveDebug() {
    if (m_config->value(SETTING_DEBUGGER_SAVE_ON_CLOSE, false).toBool()) {
        debugExportFile(m_config->value(SETTING_DEBUGGER_IMAGE_PATH).toString());
    }
}

void MainWindow::saveSettings() {
    if (opts.useSettings) {
        m_config->setValue(SETTING_WINDOW_POSITION, pos());
        m_config->setValue(SETTING_WINDOW_GEOMETRY, saveGeometry());
        m_config->setValue(SETTING_WINDOW_STATE, saveState());
        m_config->setValue(SETTING_CURRENT_DIR, m_dir.absolutePath());
        m_config->setValue(SETTING_DEBUGGER_FLASH_BYTES, ui->flashBytes->value());
        m_config->setValue(SETTING_DEBUGGER_RAM_BYTES, ui->ramBytes->value());
        m_config->setValue(SETTING_DEBUGGER_FLASH_ASCII, ui->flashAscii->isChecked());
        m_config->setValue(SETTING_DEBUGGER_RAM_ASCII, ui->ramAscii->isChecked());
        m_config->setValue(SETTING_WINDOW_FULLSCREEN, m_fullscreen);
        m_config->setValue(SETTING_WINDOW_MEMORY_DOCKS, m_docksMemory);
        m_config->setValue(SETTING_WINDOW_MEMORY_DOCK_BYTES, QVariant::fromValue(m_docksMemoryBytes));
        m_config->setValue(SETTING_WINDOW_MEMORY_DOCK_ASCII, QVariant::fromValue(m_docksMemoryAscii));
        m_config->setValue(SETTING_WINDOW_VISUALIZER_DOCKS, m_docksVisualizer);
        m_config->setValue(SETTING_WINDOW_VISUALIZER_CONFIG, m_docksVisualizerConfig);
        m_config->setValue(SETTING_WINDOW_KEYHISTORY_DOCKS, m_docksKeyHistory);
        m_config->setValue(SETTING_WINDOW_KEYHISTORY_CONFIG, QVariant::fromValue(m_docksKeyHistorySize));

        saveDebug();
        stateSaveInfo();
        recentSaveInfo();

        m_config->sync();
    }
}

void MainWindow::guiExport() {
    QString filter = tr("Window Config (*.ini)");
    QString path = QFileDialog::getSaveFileName(this, tr("Save window configuration"),
                                                m_dir.absolutePath(), filter, &filter);
    if (path.isEmpty()) {
        return;
    }

    QSettings window(path, QSettings::IniFormat);
    window.setValue(SETTING_SCREEN_SKIN, m_config->value(SETTING_SCREEN_SKIN));
    window.setValue(SETTING_WINDOW_FULLSCREEN, m_fullscreen);
    window.setValue(SETTING_WINDOW_MENUBAR, m_config->value(SETTING_WINDOW_MENUBAR));
    window.setValue(SETTING_WINDOW_STATUSBAR, m_config->value(SETTING_WINDOW_STATUSBAR));
    window.setValue(SETTING_WINDOW_SEPARATOR, m_config->value(SETTING_WINDOW_SEPARATOR));
    window.setValue(SETTING_WINDOW_STATE, saveState());
    window.setValue(SETTING_WINDOW_GEOMETRY, saveGeometry());
    window.setValue(SETTING_WINDOW_POSITION, pos());
    window.setValue(SETTING_WINDOW_MEMORY_DOCKS, m_docksMemory);
    window.setValue(SETTING_WINDOW_MEMORY_DOCK_BYTES, QVariant::fromValue(m_docksMemoryBytes));
    window.setValue(SETTING_WINDOW_MEMORY_DOCK_ASCII, QVariant::fromValue(m_docksMemoryAscii));
    window.setValue(SETTING_WINDOW_VISUALIZER_DOCKS, m_docksVisualizer);
    window.setValue(SETTING_WINDOW_VISUALIZER_CONFIG, m_docksVisualizerConfig);
    window.setValue(SETTING_WINDOW_KEYHISTORY_DOCKS, m_docksKeyHistory);
    window.setValue(SETTING_WINDOW_KEYHISTORY_CONFIG, QVariant::fromValue(m_docksKeyHistorySize));
    window.setValue(SETTING_UI_EDIT_MODE, m_uiEditMode);
    window.setValue(SETTING_STATUS_INTERVAL, ui->statusInterval->value());
    window.sync();
}

void MainWindow::guiImport() {
    QString path = QFileDialog::getOpenFileName(this, tr("Select saved image to restore from"),
                                                m_dir.absolutePath(),
                                                tr("Window Config (*.ini);;All files (*.*)"));
    if (path.isEmpty()) {
        return;
    }

    QSettings window(path, QSettings::IniFormat);
    ipcCloseConnected();
    m_config->setValue(SETTING_SCREEN_SKIN, window.value(SETTING_SCREEN_SKIN));
    m_config->setValue(SETTING_WINDOW_FULLSCREEN, window.value(SETTING_WINDOW_FULLSCREEN));
    m_config->setValue(SETTING_WINDOW_GEOMETRY, window.value(SETTING_WINDOW_GEOMETRY));
    m_config->setValue(SETTING_WINDOW_STATE, window.value(SETTING_WINDOW_STATE));
    m_config->setValue(SETTING_WINDOW_POSITION, window.value(SETTING_WINDOW_POSITION));
    m_config->setValue(SETTING_WINDOW_MEMORY_DOCKS, window.value(SETTING_WINDOW_MEMORY_DOCKS));
    m_config->setValue(SETTING_WINDOW_MEMORY_DOCK_BYTES, window.value(SETTING_WINDOW_MEMORY_DOCK_BYTES));
    m_config->setValue(SETTING_WINDOW_MEMORY_DOCK_ASCII, window.value(SETTING_WINDOW_MEMORY_DOCK_ASCII));
    m_config->setValue(SETTING_WINDOW_VISUALIZER_DOCKS, window.value(SETTING_WINDOW_VISUALIZER_DOCKS));
    m_config->setValue(SETTING_WINDOW_VISUALIZER_CONFIG, window.value(SETTING_WINDOW_VISUALIZER_CONFIG));
    m_config->setValue(SETTING_WINDOW_KEYHISTORY_DOCKS, window.value(SETTING_WINDOW_KEYHISTORY_DOCKS));
    m_config->setValue(SETTING_WINDOW_KEYHISTORY_CONFIG, window.value(SETTING_WINDOW_KEYHISTORY_CONFIG));
    m_config->setValue(SETTING_UI_EDIT_MODE, window.value(SETTING_UI_EDIT_MODE));
    m_config->setValue(SETTING_WINDOW_MENUBAR, window.value(SETTING_WINDOW_MENUBAR));
    m_config->setValue(SETTING_WINDOW_STATUSBAR, window.value(SETTING_WINDOW_STATUSBAR));
    m_config->setValue(SETTING_WINDOW_SEPARATOR, window.value(SETTING_WINDOW_SEPARATOR));
    m_config->setValue(SETTING_STATUS_INTERVAL, window.value(SETTING_STATUS_INTERVAL));
    m_needReload = true;
    close();
}

void MainWindow::keymapExport() {
    QString filter = tr("Keymap Config (*.ini)");
    QString path = QFileDialog::getSaveFileName(this, tr("Save keymap configuration"),
                                                m_dir.absolutePath(), filter, &filter);
    if (path.isEmpty()) {
        return;
    }


    keypadBridge->keymapExport(path);
}

bool MainWindow::isFirstRun() {
    return !m_config->value(SETTING_FIRST_RUN, false).toBool();
}

