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
const QString MainWindow::SETTING_DEBUGGER_ADD_DISASM_SPACE = QStringLiteral("Debugger/add_disassembly_space");
const QString MainWindow::SETTING_DEBUGGER_RESTORE_ON_OPEN  = QStringLiteral("Debugger/restore_on_open");
const QString MainWindow::SETTING_DEBUGGER_SAVE_ON_CLOSE    = QStringLiteral("Debugger/save_on_close");
const QString MainWindow::SETTING_DEBUGGER_RESET_OPENS      = QStringLiteral("Debugger/open_on_reset");
const QString MainWindow::SETTING_DEBUGGER_ENABLE_SOFT      = QStringLiteral("Debugger/enable_soft_commands");
const QString MainWindow::SETTING_DEBUGGER_DATA_COL         = QStringLiteral("Debugger/show_data_column");
const QString MainWindow::SETTING_DEBUGGER_IMAGE_PATH       = QStringLiteral("Debugger/image_path");
const QString MainWindow::SETTING_DEBUGGER_FLASH_BYTES      = QStringLiteral("Debugger/flash_bytes_per_line");
const QString MainWindow::SETTING_DEBUGGER_RAM_BYTES        = QStringLiteral("Debugger/ram_bytes_per_line");
const QString MainWindow::SETTING_DEBUGGER_FLASH_ASCII      = QStringLiteral("Debugger/flash_ascii");
const QString MainWindow::SETTING_DEBUGGER_RAM_ASCII        = QStringLiteral("Debugger/ram_ascii");
const QString MainWindow::SETTING_DEBUGGER_BREAK_IGNORE     = QStringLiteral("Debugger/ignore_breakpoints");
const QString MainWindow::SETTING_DEBUGGER_AUTO_EQUATES     = QStringLiteral("Debugger/auto_equates");
const QString MainWindow::SETTING_DEBUGGER_IGNORE_DMA       = QStringLiteral("Debugger/ignore_dma");
const QString MainWindow::SETTING_DEBUGGER_PRE_I            = QStringLiteral("Debugger/pre_i");
const QString MainWindow::SETTING_SCREEN_FRAMESKIP          = QStringLiteral("Screen/frameskip");
const QString MainWindow::SETTING_SCREEN_SCALE              = QStringLiteral("Screen/scale");
const QString MainWindow::SETTING_SCREEN_SKIN               = QStringLiteral("Screen/skin");
const QString MainWindow::SETTING_SCREEN_SPI                = QStringLiteral("Screen/spi");
const QString MainWindow::SETTING_KEYPAD_KEYMAP             = QStringLiteral("Keypad/map");
const QString MainWindow::SETTING_KEYPAD_COLOR              = QStringLiteral("Keypad/color");
const QString MainWindow::SETTING_WINDOW_STATE              = QStringLiteral("Window/state");
const QString MainWindow::SETTING_WINDOW_GEOMETRY           = QStringLiteral("Window/geometry");
const QString MainWindow::SETTING_WINDOW_SEPARATOR          = QStringLiteral("Window/boundaries");
const QString MainWindow::SETTING_WINDOW_MENUBAR            = QStringLiteral("Window/menubar");
const QString MainWindow::SETTING_WINDOW_MEMORY_DOCKS       = QStringLiteral("Window/memory_docks");
const QString MainWindow::SETTING_WINDOW_MEMORY_DOCK_BYTES  = QStringLiteral("Window/memory_docks_bytes");
const QString MainWindow::SETTING_WINDOW_MEMORY_DOCK_ASCII  = QStringLiteral("Window/memory_docks_ascii");
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
const QString MainWindow::SETTING_CURRENT_DIR               = QStringLiteral("current_directory");
const QString MainWindow::SETTING_ENABLE_WIN_CONSOLE        = QStringLiteral("enable_windows_console");
const QString MainWindow::SETTING_RECENT_SAVE               = QStringLiteral("Recent/save_paths");
const QString MainWindow::SETTING_RECENT_PATHS              = QStringLiteral("Recent/paths");
const QString MainWindow::SETTING_RECENT_SELECT             = QStringLiteral("Recent/selected");

const QString MainWindow::SETTING_KEYPAD_CEMU               = QStringLiteral("cemu");
const QString MainWindow::SETTING_KEYPAD_TILEM              = QStringLiteral("tilem");
const QString MainWindow::SETTING_KEYPAD_WABBITEMU          = QStringLiteral("wabbitemu");
const QString MainWindow::SETTING_KEYPAD_JSTIFIED           = QStringLiteral("jsTIfied");

const QString MainWindow::SETTING_PREFERRED_LANG            = QStringLiteral("preferred_lang");
const QString MainWindow::SETTING_VERSION                   = QStringLiteral("version");

const QString MainWindow::SETTING_DEFAULT_FILE              = QStringLiteral("/cemu_config.ini");
const QString MainWindow::SETTING_DEFAULT_ROM_FILE          = QStringLiteral("/cemu_rom.rom");
const QString MainWindow::SETTING_DEFAULT_DEBUG_FILE        = QStringLiteral("/cemu_debug.ini");
const QString MainWindow::SETTING_DEFAULT_IMAGE_FILE        = QStringLiteral("/cemu_image.ce");
const QString MainWindow::TXT_YES                           = QStringLiteral("y");
const QString MainWindow::TXT_NO                            = QStringLiteral("n");

void MainWindow::setPortableConfig(bool state) {
    ui->checkPortable->setChecked(state);
    QString pathSet;
    QString romPathSet;
    QString debugPathSet;
    QString imagePathSet;
    QDir appDir = qApp->applicationDirPath();

    if (state) {
        pathSet = qApp->applicationDirPath() + SETTING_DEFAULT_FILE;
        QFile::copy(m_settingsPath, pathSet);
    } else {
        pathSet = configPath + SETTING_DEFAULT_FILE;
        QFile(m_settingsPath).remove();
    }
    debugPathSet = QDir::cleanPath(QFileInfo(pathSet).absoluteDir().absolutePath() + SETTING_DEFAULT_DEBUG_FILE);
    imagePathSet =  QDir::cleanPath(QFileInfo(pathSet).absoluteDir().absolutePath() + SETTING_DEFAULT_IMAGE_FILE);

    if(state) {
        debugPathSet = appDir.relativeFilePath(debugPathSet);
        imagePathSet = appDir.relativeFilePath(imagePathSet);
        romPathSet = appDir.relativeFilePath(m_settings->value(SETTING_ROM_PATH).toString());
        m_pathRom = romPathSet;
        m_settings->setValue(SETTING_ROM_PATH, romPathSet);
        ui->rompathView->setText(romPathSet);
        ui->settingsPath->setText(appDir.relativeFilePath(pathSet));
    } else {
        ui->settingsPath->setText(pathSet);
    }

    delete m_settings;
    m_settings = new QSettings(pathSet, QSettings::IniFormat);
    m_settings->setValue(SETTING_DEBUGGER_IMAGE_PATH, debugPathSet);
    m_settings->setValue(SETTING_IMAGE_PATH, imagePathSet);

    ui->savedImagePath->setText(imagePathSet);
    ui->savedDebugPath->setText(debugPathSet);
    m_pathImage = imagePathSet;
    m_settingsPath = pathSet;
    saveSettings();
    m_portable = state;
    ui->buttonChangeSavedDebugPath->setEnabled(!m_portable);
    ui->buttonChangeSavedImagePath->setEnabled(!m_portable);
}

void MainWindow::setFrameskip(int value) {
    m_settings->setValue(SETTING_CAPTURE_FRAMESKIP, value);
    ui->apngSkip->setValue(value);
    ui->apngSkipDisplay->setText(QString::number((ui->guiSkip->value() + 1) * (ui->apngSkip->value() + 1) - 1));
}

void MainWindow::setOptimizeRecording(bool state) {
    ui->checkOptimizeRecording->setChecked(state);
    m_settings->setValue(SETTING_CAPTURE_OPTIMIZE, state);
    m_optimizeRecording = state;
}

bool MainWindow::checkForCEmuBootImage() {
    QDirIterator dirIt(qApp->applicationDirPath(), QDirIterator::NoIteratorFlags);
    while (dirIt.hasNext()) {
        dirIt.next();
        QString dirItFile = dirIt.filePath();
        if (QFileInfo(dirItFile).isFile()) {
            if (QFileInfo(dirItFile).suffix() == QStringLiteral("cemu")) {
                if (!m_loadedBootImage) {
                    m_loadedBootImage = loadCEmuBootImage(dirItFile);
                }
                QFile(dirItFile).remove();
            }
        }
    }
    return m_loadedBootImage;
}

bool MainWindow::loadCEmuBootImage(const QString &bootImagePath) {
    QString newSettingsPath = configPath + SETTING_DEFAULT_FILE;
    QString romPath = configPath + SETTING_DEFAULT_ROM_FILE;
    QFile bootFile(bootImagePath);
    QFile romFile(romPath);
    romFile.remove();
    if (!romFile.open(QIODevice::WriteOnly)) { return false; }
    QFile settingsFile(newSettingsPath);
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
    m_settingsPath = newSettingsPath;
    return true;
}

void MainWindow::exportCEmuBootImage() {
    QMessageBox::information(this, MSG_INFORMATION, tr("A bootable image can be used to start CEmu with predefined configurations, without the need for any extra setup."
                                                       "\n\nThe bootable image should be placed in the same directory as the CEmu executable. When CEmu is then started, "
                                                       "the boot image will be loaded automatically and then removed for convience."));

    QString path = QFileDialog::getSaveFileName(this, tr("Save bootable CEmu image"),
                                                m_dir.absolutePath(),
                                                tr("Bootable CEmu images (*.cemu);"));
    saveSettings();

    if (!path.isEmpty()) {
        m_dir = QFileInfo(path).absoluteDir();
        QFile romFile(m_pathRom);
        if (!romFile.open(QIODevice::ReadOnly)) return;
        QByteArray romData = romFile.readAll();

        QFile settingsFile(m_settingsPath);
        if (!settingsFile.open(QIODevice::ReadOnly)) return;
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
    m_settings->setValue(SETTING_DEBUGGER_ENABLE_SOFT, state);
    debugger.commands = state;
}

void MainWindow::setDataCol(bool state) {
    ui->checkDataCol->setChecked(state);
    m_settings->setValue(SETTING_DEBUGGER_DATA_COL, state);
    m_useDataCol = state;
}

void MainWindow::setLcdSpi(bool state) {
    ui->checkSpi->setChecked(state);
    m_settings->setValue(SETTING_SCREEN_SPI, state);
    lcd.spi = state;
    emit updateMode(state);
}

void MainWindow::setLcdDma(bool state) {
    ui->checkDma->setChecked(state);
    m_settings->setValue(SETTING_DEBUGGER_IGNORE_DMA, !state);
    ui->cycleView->setText(QString::number(!state ? debugger.totalCycles : debugger.totalCycles + debugger.dmaCycles));
    m_ignoreDmaCycles = !state;
}

void MainWindow::setFocusSetting(bool state) {
    ui->checkFocus->setChecked(state);
    m_settings->setValue(SETTING_PAUSE_FOCUS, state);
    m_pauseOnFocus = state;
}

void MainWindow::saveMiscSettings() {
    m_settings->setValue(SETTING_CURRENT_DIR, m_dir.absolutePath());
    m_settings->setValue(SETTING_DEBUGGER_FLASH_BYTES, ui->flashBytes->value());
    m_settings->setValue(SETTING_DEBUGGER_RAM_BYTES, ui->ramBytes->value());
    m_settings->setValue(SETTING_DEBUGGER_FLASH_ASCII, ui->flashAscii->isChecked());
    m_settings->setValue(SETTING_DEBUGGER_RAM_ASCII, ui->ramAscii->isChecked());
    saveSlotInfo();
    saveRecentInfo();

    if (!m_needReload) {
        m_settings->setValue(SETTING_WINDOW_STATE, saveState());
        m_settings->setValue(SETTING_WINDOW_GEOMETRY, saveGeometry());
        m_settings->setValue(SETTING_WINDOW_MEMORY_DOCKS, m_docksMemory);
        m_settings->setValue(SETTING_WINDOW_MEMORY_DOCK_BYTES, QVariant::fromValue(m_docksMemoryBytes));
        m_settings->setValue(SETTING_WINDOW_MEMORY_DOCK_ASCII, QVariant::fromValue(m_docksMemoryAscii));
    } else {
        if (!m_windowLoading && !m_keepSetup) {
            m_settings->setValue(SETTING_FIRST_RUN, false);
        }
    }
}

void MainWindow::setMemoryState() {
    ui->ramBytes->setValue(m_settings->value(SETTING_DEBUGGER_RAM_BYTES, 8).toInt());
    ui->flashBytes->setValue(m_settings->value(SETTING_DEBUGGER_FLASH_BYTES, 8).toInt());
    ui->ramAscii->setChecked(m_settings->value(SETTING_DEBUGGER_RAM_ASCII, true).toBool());
    ui->flashAscii->setChecked(m_settings->value(SETTING_DEBUGGER_FLASH_ASCII, true).toBool());
    ui->ramEdit->setAsciiArea(ui->ramAscii->isChecked());
    ui->flashEdit->setAsciiArea(ui->flashAscii->isChecked());
}

void MainWindow::setMenuBarState(bool state) {
    ui->menubar->setHidden(state);
    ui->actionDisableMenuBar->setChecked(state);
    m_settings->setValue(SETTING_WINDOW_MENUBAR, state);
}

void MainWindow::resetSettingsIfLoadedCEmuBootableImage() {
    if (m_loadedBootImage) {
        m_settings->setValue(SETTING_FIRST_RUN, false);
    }
}

void MainWindow::setDebugIgnoreBreakpoints(bool state) {
    ui->buttonToggleBreakpoints->setChecked(state);
    m_settings->setValue(SETTING_DEBUGGER_BREAK_IGNORE, state);
    debugger.ignore = state;
}

void MainWindow::setDebugResetTrigger(bool state) {
    ui->checkDebugResetTrigger->setChecked(state);
    m_settings->setValue(SETTING_DEBUGGER_RESET_OPENS, state);
    debugger.resetOpensDebugger = state;
}

void MainWindow::setAutoSaveState(bool state) {
    ui->checkSaveRestore->setChecked(state);
    m_settings->setValue(SETTING_RESTORE_ON_OPEN, state);
    m_settings->setValue(SETTING_SAVE_ON_CLOSE, state);
}

void MainWindow::setSaveDebug(bool state) {
    ui->checkSaveLoadDebug->setChecked(state);
    m_settings->setValue(SETTING_DEBUGGER_SAVE_ON_CLOSE, state);
    m_settings->setValue(SETTING_DEBUGGER_RESTORE_ON_OPEN, state);
}

void MainWindow::setSpaceDisasm(bool state) {
    ui->checkAddSpace->setChecked(state);
    m_settings->setValue(SETTING_DEBUGGER_ADD_DISASM_SPACE, state);
    disasm.space = state ? ", " : ",";
}

void MainWindow::setFont(int fontSize) {
    ui->textSize->setValue(fontSize);
    m_settings->setValue(SETTING_DEBUGGER_TEXT_SIZE, ui->textSize->value());

    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    monospace.setPointSize(fontSize);
    ui->console->setFont(monospace);
    ui->disassemblyView->setFont(monospace);

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
    m_settings->setValue(SETTING_KEYPAD_COLOR, color);
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
        m_settings->setValue(SETTING_IMAGE_PATH, saveImagePath);
        ui->savedImagePath->setText(saveImagePath);
    }
}

void MainWindow::setDebugPath() {
    QString savePath = QFileDialog::getSaveFileName(this, tr("Set debugging information path"),
                                                           m_dir.absolutePath(),
                                                           tr("Debugging information (*.ini);;All files (*.*)"));
    if (!savePath.isEmpty()) {
        m_dir = QFileInfo(savePath).absoluteDir();
        m_settings->setValue(SETTING_DEBUGGER_IMAGE_PATH, savePath);
        ui->savedDebugPath->setText(savePath);
    }
}

void MainWindow::setUIDocks(bool firstRun) {
    // Already in this mode?
    if (ui->tabWidget->isHidden()) {
        return;
    }

    // Create "Docks" menu to make closing and opening docks more intuitive
    m_menuDocks = new QMenu(TITLE_DOCKS, this);
    ui->menubar->insertMenu(ui->menuAbout->menuAction(), m_menuDocks);

    // Convert the tabs into QDockWidgets
    DockWidget *last_dock = Q_NULLPTR;
    while (ui->tabWidget->count()) {
        DockWidget *dw = new DockWidget(ui->tabWidget, this);

        // Fill "Docks" menu
        QAction *action = dw->toggleViewAction();
        action->setIcon(dw->windowIcon());
        m_menuDocks->addAction(action);

        dw->setAllowedAreas(Qt::AllDockWidgetAreas);
        addDockWidget(Qt::RightDockWidgetArea, dw);
        if (last_dock) {
            tabifyDockWidget(last_dock, dw);
        }

        last_dock = dw;
    }

    m_menuDocks->addSeparator();

    m_menuDebug = new QMenu(TITLE_DEBUG, this);
    ui->menubar->insertMenu(ui->menuAbout->menuAction(), m_menuDebug);

    // Convert the tabs into QDockWidgets
    while (ui->tabDebug->count()) {
        DockWidget *dw = new DockWidget(ui->tabDebug, this);

        // Fill "Docks" menu
        QAction *action = dw->toggleViewAction();
        action->setIcon(dw->windowIcon());
        m_menuDebug->addAction(action);

        dw->setAllowedAreas(Qt::AllDockWidgetAreas);
        addDockWidget(Qt::RightDockWidgetArea, dw);
        if (firstRun) {
            dw->setFloating(true);
            dw->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, dw->minimumSize(), qApp->desktop()->availableGeometry()));
            dw->close();
            dw->hide();
        }
    }

    m_menuDebug->addSeparator();
    m_menuDebug->addAction(m_actionAddMemory);

    m_menuDocks->addSeparator();
    m_menuDocks->addAction(m_actionToggleUI);

    ui->tabWidget->close();
    ui->tabDebug->close();
}

void MainWindow::toggleUIEditMode() {
    setUIEditMode(!m_uiEditMode);
}

void MainWindow::updateDocks() {
    for (const auto &dock : findChildren<DockWidget *>()) {
        dock->toggleState(m_uiEditMode);
        if (dock->isFloating() && !dock->isHidden() && !m_uiEditMode) {
            removeDockWidget(dock);
            dock->show();
            dock->activateWindow();
            dock->raise();
        }
    }
}

void MainWindow::setUIEditMode(bool mode) {
    m_uiEditMode = mode;
    m_settings->setValue(SETTING_UI_EDIT_MODE, m_uiEditMode);
    m_actionToggleUI->setChecked(m_uiEditMode);
    m_actionAddMemory->setEnabled(m_uiEditMode);
    updateDocks();
    if (m_uiEditMode) {
        setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        | QMainWindow::GroupedDragging
#endif
        );
    } else {
        setDockOptions(0);
    }
    setDockBoundaries(!m_uiEditMode);
}

void MainWindow::setThrottle(int mode) {
    ui->checkThrottle->setChecked(mode == Qt::Checked);
    connect(&emu, &EmuThread::actualSpeedChanged, this, &MainWindow::showEmuSpeed, Qt::QueuedConnection);
    emu.setThrottleMode(mode == Qt::Checked);
}

void MainWindow::setAutoCheckForUpdates(int state) {
    m_settings->setValue(SETTING_AUTOUPDATE, state);
    ui->checkUpdates->setChecked(state);

    if (state == Qt::Checked) {
        checkForUpdates(false);
    }
}

void MainWindow::checkForUpdates(bool forceInfoBox) {
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

void MainWindow::adjustScreen() {
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
    m_settings->setValue(SETTING_SCREEN_SCALE, roundedScale);
    ui->scaleLCD->setValue(roundedScale);
    adjustScreen();
}

void MainWindow::setSkinToggle(bool enable) {
    m_settings->setValue(SETTING_SCREEN_SKIN, enable);
    ui->checkSkin->setChecked(enable);
    adjustScreen();
}

void MainWindow::setAutoEquates(bool enable) {
    m_settings->setValue(SETTING_DEBUGGER_AUTO_EQUATES, enable);
    ui->checkAutoEquates->setChecked(enable);
    sendingHandler->setLoadEquates(enable);
}

void MainWindow::setGuiSkip(int value) {
    m_settings->setValue(SETTING_SCREEN_FRAMESKIP, value);
    ui->guiSkip->blockSignals(true);
    ui->guiSkip->setValue(value);
    ui->guiSkip->blockSignals(false);
    ui->apngSkipDisplay->setText(QString::number((ui->guiSkip->value() + 1) * (ui->apngSkip->value() + 1) - 1));
    emit updateFrameskip(value);
}

void MainWindow::setStatusInterval(int value) {
    m_settings->setValue(SETTING_STATUS_INTERVAL, value);
    ui->statusInterval->setValue(value);
    m_timerFps.stop();
    m_timerEmu.stop();
    connect(&m_timerEmu, SIGNAL(timeout()), this, SLOT(emuTimerSlot()));
    connect(&m_timerFps, SIGNAL(timeout()), this, SLOT(fpsTimerSlot()));
    m_timerEmuTriggerable = true;
    m_timerFpsTriggerable = true;
    if (!value) {
        disconnect(&m_timerEmu, SIGNAL(timeout()), this, SLOT(emuTimerSlot()));
        disconnect(&m_timerFps, SIGNAL(timeout()), this, SLOT(fpsTimerSlot()));
        connect(&emu, &EmuThread::actualSpeedChanged, this, &MainWindow::showEmuSpeed, Qt::QueuedConnection);
        m_timerFpsTriggered = true;
        m_timerEmuTriggerable = false;
        m_timerFpsTriggerable = false;
    } else {
        m_timerFps.start(value * 1000);
        m_timerEmu.start(value * 1000);
    }
}

void MainWindow::setEmuSpeed(int value) {
    m_settings->setValue(SETTING_EMUSPEED, value);
    ui->emulationSpeedSpin->blockSignals(true);
    ui->emulationSpeedSpin->setValue(value);
    ui->emulationSpeedSpin->blockSignals(false);
    ui->emulationSpeed->setValue(value);
    emu.setEmuSpeed(value);
}

void MainWindow::selectKeypadColor() {
    QString name = sender()->objectName();
    unsigned int color = KEYPAD_BLACK;

    if (name == QStringLiteral("buttonWhite"))     color = KEYPAD_WHITE;
    if (name == QStringLiteral("buttonBlack"))     color = KEYPAD_BLACK;
    if (name == QStringLiteral("buttonGolden"))    color = KEYPAD_GOLDEN;
    if (name == QStringLiteral("buttonPlum"))      color = KEYPAD_PLUM;
    if (name == QStringLiteral("buttonPink"))      color = KEYPAD_PINK;
    if (name == QStringLiteral("buttonRed"))       color = KEYPAD_RED;
    if (name == QStringLiteral("buttonLightning")) color = KEYPAD_LIGHTNING;
    if (name == QStringLiteral("buttonTrueBlue"))  color = KEYPAD_TRUE_BLUE;
    if (name == QStringLiteral("buttonDenim"))     color = KEYPAD_DENIM;
    if (name == QStringLiteral("buttonSilver"))    color = KEYPAD_SILVER;
    if (name == QStringLiteral("buttonSpaceGrey")) color = KEYPAD_SPACEGREY;
    if (name == QStringLiteral("buttonCoral"))     color = KEYPAD_CORAL;
    if (name == QStringLiteral("buttonMint"))      color = KEYPAD_MINT;

    setKeypadColor(color);
}

void MainWindow::keymapChanged() {
    if (ui->radioCEmuKeys->isChecked()) {
        setKeymap(SETTING_KEYPAD_CEMU);
    } else if (ui->radioTilEmKeys->isChecked()) {
        setKeymap(SETTING_KEYPAD_TILEM);
    } else if (ui->radioWabbitemuKeys->isChecked()) {
        setKeymap(SETTING_KEYPAD_WABBITEMU);
    } else if (ui->radiojsTIfiedKeys->isChecked()) {
        setKeymap(SETTING_KEYPAD_JSTIFIED);
    }
}

void MainWindow::setKeymap(const QString & value) {
    m_settings->setValue(SETTING_KEYPAD_KEYMAP, value);
    keypadBridge->setKeymap(value);
}

void MainWindow::toggleFullscreen() {
    static QWidget *parent_ptr = Q_NULLPTR;
    switch (m_fullscreen) {
        default:
        case FULLSCREEN_NONE:
            showFullScreen();
            m_fullscreen = FULLSCREEN_ALL;
            break;
        case FULLSCREEN_ALL:
            parent_ptr = ui->lcd->parentWidget();
            ui->lcd->setParent(this, Qt::Tool | Qt::FramelessWindowHint | Qt::CustomizeWindowHint);
            ui->lcd->setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            ui->lcd->showFullScreen();
            ui->lcd->installEventFilter(keypadBridge);
            ui->lcd->setFocus();
            m_fullscreen = FULLSCREEN_LCD;
            break;
        case FULLSCREEN_LCD:
            showNormal();
            ui->lcd->setParent(parent_ptr);
            ui->lcd->showNormal();
            adjustScreen();
            m_fullscreen = FULLSCREEN_NONE;
            break;
    }
}

void MainWindow::setAlwaysOnTop(int state) {
    if (!state) {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    } else {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    }
    show();
    m_settings->setValue(SETTING_ALWAYS_ON_TOP, state);
    ui->checkAlwaysOnTop->setCheckState(state ? Qt::Checked : Qt::Unchecked);
}

void MainWindow::saveSlotInfo() {
    QStringList slotNames;
    QStringList slotPaths;

    for (int i = 0; i < ui->slotView->rowCount(); i++) {
        slotNames.append(ui->slotView->item(i, SLOT_NAME)->text());
        slotPaths.append(ui->slotView->item(i, SLOT_EDIT)->data(Qt::UserRole).toString());
    }

    m_settings->setValue(SETTING_SLOT_NAMES, slotNames);
    m_settings->setValue(SETTING_SLOT_PATHS, slotPaths);
}

void MainWindow::setSlotInfo() {
    QStringList slotNames = m_settings->value(SETTING_SLOT_NAMES).toStringList();
    QStringList slotPaths = m_settings->value(SETTING_SLOT_PATHS).toStringList();

    for (int i = 0; i < slotNames.size(); i++) {
        QString name = slotNames.at(i);
        QString path = slotPaths.at(i);
        slotAdd(name, path);
    }
}

void MainWindow::setRecentSave(bool state) {
    ui->checkSaveRecent->setChecked(state);
    m_settings->setValue(SETTING_RECENT_SAVE, state);
}

void MainWindow::setPreRevisionI(bool state) {
    ui->checkPreI->setChecked(state);
    m_settings->setValue(SETTING_DEBUGGER_PRE_I, state);
    cpu.preI = state;
}

void MainWindow::setDockBoundaries(bool state) {
    m_settings->setValue(SETTING_WINDOW_SEPARATOR, state);
    if (state) {
        setStyleSheet(QStringLiteral("QMainWindow::separator{ width: 0px; height: 0px; }"));
    } else {
        setStyleSheet(QStringLiteral("QMainWindow::separator{ width: 4px; height: 4px; }"));
    }
}

void MainWindow::saveRecentInfo() {
    QStringList paths;
    QStringList selects;

    if (m_settings->value(SETTING_RECENT_SAVE).toBool()) {
        for (int i = 0; i < ui->varLoadedView->rowCount(); i++) {
            paths.append(ui->varLoadedView->item(i, RECENT_PATH)->text());
            selects.append(ui->varLoadedView->item(i, RECENT_SELECT)->checkState() == Qt::Checked ? TXT_YES : TXT_NO);
        }
    }

    m_settings->setValue(SETTING_RECENT_PATHS, paths);
    m_settings->setValue(SETTING_RECENT_SELECT, selects);
}

void MainWindow::setRecentInfo() {
    QStringList paths = m_settings->value(SETTING_RECENT_PATHS).toStringList();
    QStringList selects = m_settings->value(SETTING_RECENT_SELECT).toStringList();

    if (m_settings->value(SETTING_RECENT_SAVE).toBool()) {
        for (int i = 0; i < paths.size(); i++) {
            QString path = paths.at(i);
            bool select = selects.at(i) == TXT_YES;
            sendingHandler->addFile(path, select);
        }
    }
}

void MainWindow::setMemoryDocks() {
    QStringList names = m_settings->value(SETTING_WINDOW_MEMORY_DOCKS).toStringList();
    QList<int> bytes = m_settings->value(SETTING_WINDOW_MEMORY_DOCK_BYTES).value<QList<int>>();
    QList<bool> ascii = m_settings->value(SETTING_WINDOW_MEMORY_DOCK_ASCII).value<QList<bool>>();

    if (names.length() != bytes.length()) {
        return;
    }

    for (int i = 0; i < names.length(); i++) {
        addMemoryDock(names.at(i), bytes.at(i), ascii.at(i));
    }
}

void MainWindow::setVersion() {
    m_settings->setValue(SETTING_VERSION, QStringLiteral(CEMU_VERSION));
}

void MainWindow::checkVersion() {
    bool ask = false;

    if (isFirstRun()) {
        setVersion();
        return;
    }

    if (m_settings->contains(SETTING_VERSION)) {
        if (m_settings->value(SETTING_VERSION, QStringLiteral(CEMU_VERSION)).toString().compare(QStringLiteral(CEMU_VERSION))) {
            ask = true;
        }
    } else {
        ask = true;
    }

    if (ask) {
        if (CEMU_RELEASE) {
            setAutoCheckForUpdates(true);
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

void MainWindow::saveSettings() {
    saveMiscSettings();
    if (opts.useSettings) {
        m_settings->sync();
    } else {
        QFile settingsFile(m_settings->fileName());
        settingsFile.remove();
    }
}

void MainWindow::exportWindowConfig() {
    QString filter = tr("Window Config (*.ini)");
    QString path = QFileDialog::getSaveFileName(this, tr("Save window configuration"),
                                                m_dir.absolutePath(), filter, &filter);
    if (path.isEmpty()) {
        return;
    }

    QSettings window(path, QSettings::IniFormat);
    window.setValue(SETTING_SCREEN_SKIN, m_settings->value(SETTING_SCREEN_SKIN));
    window.setValue(SETTING_WINDOW_MENUBAR, m_settings->value(SETTING_WINDOW_MENUBAR));
    window.setValue(SETTING_WINDOW_SEPARATOR, m_settings->value(SETTING_WINDOW_SEPARATOR));
    window.setValue(SETTING_WINDOW_STATE, saveState());
    window.setValue(SETTING_WINDOW_GEOMETRY, saveGeometry());
    window.setValue(SETTING_WINDOW_MEMORY_DOCKS, m_docksMemory);
    window.setValue(SETTING_WINDOW_MEMORY_DOCK_BYTES, QVariant::fromValue(m_docksMemoryBytes));
    window.setValue(SETTING_WINDOW_MEMORY_DOCK_ASCII, QVariant::fromValue(m_docksMemoryAscii));
    window.setValue(SETTING_UI_EDIT_MODE, m_uiEditMode);
    window.setValue(SETTING_STATUS_INTERVAL, ui->statusInterval->value());
    window.sync();
}

void MainWindow::importWindowConfig() {
    QString path = QFileDialog::getOpenFileName(this, tr("Select saved image to restore from"),
                                                m_dir.absolutePath(),
                                                tr("Window Config (*.ini);;All files (*.*)"));
    if (path.isEmpty()) {
        return;
    }

    QSettings window(path, QSettings::IniFormat);
    ipcCloseOthers();
    m_settings->setValue(SETTING_SCREEN_SKIN, window.value(SETTING_SCREEN_SKIN));
    m_settings->setValue(SETTING_WINDOW_GEOMETRY, window.value(SETTING_WINDOW_GEOMETRY));
    m_settings->setValue(SETTING_WINDOW_STATE, window.value(SETTING_WINDOW_STATE));
    m_settings->setValue(SETTING_WINDOW_MEMORY_DOCKS, window.value(SETTING_WINDOW_MEMORY_DOCKS));
    m_settings->setValue(SETTING_WINDOW_MEMORY_DOCK_BYTES, window.value(SETTING_WINDOW_MEMORY_DOCK_BYTES));
    m_settings->setValue(SETTING_WINDOW_MEMORY_DOCK_ASCII, window.value(SETTING_WINDOW_MEMORY_DOCK_ASCII));
    m_settings->setValue(SETTING_UI_EDIT_MODE, window.value(SETTING_UI_EDIT_MODE));
    m_settings->setValue(SETTING_WINDOW_MENUBAR, window.value(SETTING_WINDOW_MENUBAR));
    m_settings->setValue(SETTING_WINDOW_SEPARATOR, window.value(SETTING_WINDOW_SEPARATOR));
    m_settings->setValue(SETTING_STATUS_INTERVAL, window.value(SETTING_STATUS_INTERVAL));
    m_needReload = true;
    m_windowLoading = true;
    close();
}

bool MainWindow::isFirstRun() {
    return !m_settings->value(SETTING_FIRST_RUN, false).toBool();
}

