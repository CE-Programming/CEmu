#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkAccessManager>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QScrollBar>
#include <QtNetwork/QNetworkReply>
#include <fstream>

#ifdef _MSC_VER
    #include <direct.h>
    #define chdir _chdir
#else
    #include <unistd.h>
#endif

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "dockwidget.h"
#include "utils.h"

#include "../../core/schedule.h"
#include "../../core/link.h"

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
const QString MainWindow::SETTING_DEBUGGER_MEM_BYTES        = QStringLiteral("Debugger/mem_bytes_per_line");
const QString MainWindow::SETTING_SCREEN_REFRESH_RATE       = QStringLiteral("Screen/refresh_rate");
const QString MainWindow::SETTING_SCREEN_SCALE              = QStringLiteral("Screen/scale");
const QString MainWindow::SETTING_SCREEN_SKIN               = QStringLiteral("Screen/skin");
const QString MainWindow::SETTING_KEYPAD_KEYMAP             = QStringLiteral("Keypad/map");
const QString MainWindow::SETTING_KEYPAD_COLOR              = QStringLiteral("Keypad/color");
const QString MainWindow::SETTING_WINDOW_SIZE               = QStringLiteral("Window/size");
const QString MainWindow::SETTING_WINDOW_STATE              = QStringLiteral("Window/state");
const QString MainWindow::SETTING_WINDOW_GEOMETRY           = QStringLiteral("Window/geometry");
const QString MainWindow::SETTING_CAPTURE_FRAMESKIP         = QStringLiteral("Capture/frameskip");
const QString MainWindow::SETTING_IMAGE_PATH                = QStringLiteral("image_path");
const QString MainWindow::SETTING_ROM_PATH                  = QStringLiteral("rom_path");
const QString MainWindow::SETTING_FIRST_RUN                 = QStringLiteral("first_run");
const QString MainWindow::SETTING_UI_EDIT_MODE              = QStringLiteral("ui_edit_mode");
const QString MainWindow::SETTING_SAVE_ON_CLOSE             = QStringLiteral("save_on_close");
const QString MainWindow::SETTING_RESTORE_ON_OPEN           = QStringLiteral("restore_on_open");
const QString MainWindow::SETTING_EMUSPEED                  = QStringLiteral("emulated_speed");
const QString MainWindow::SETTING_AUTOUPDATE                = QStringLiteral("check_for_updates");
const QString MainWindow::SETTING_DISABLE_MENUBAR           = QStringLiteral("disable_menubar");
const QString MainWindow::SETTING_ALWAYS_ON_TOP             = QStringLiteral("always_on_top");
const QString MainWindow::SETTING_CURRENT_DIR               = QStringLiteral("current_directory");
const QString MainWindow::SETTING_ENABLE_WIN_CONSOLE        = QStringLiteral("enable_windows_console");

const QString MainWindow::SETTING_KEYPAD_CEMU               = QStringLiteral("cemu");
const QString MainWindow::SETTING_KEYPAD_TILEM              = QStringLiteral("tilem");
const QString MainWindow::SETTING_KEYPAD_WABBITEMU          = QStringLiteral("wabbitemu");
const QString MainWindow::SETTING_KEYPAD_JSTIFIED           = QStringLiteral("jsTIfied");

const QString MainWindow::SETTING_DEFAULT_FILE              = QStringLiteral("/cemu_config.ini");
const QString MainWindow::SETTING_DEFAULT_ROM_FILE          = QStringLiteral("/cemu_rom.rom");
const QString MainWindow::SETTING_DEFAULT_DEBUG_FILE        = QStringLiteral("/cemu_debug.ini");
const QString MainWindow::SETTING_DEFAULT_IMAGE_FILE        = QStringLiteral("/cemu_image.ce");

const QString MainWindow::MSG_WARNING                       = tr("Warning");
const QString MainWindow::MSG_ERROR                         = tr("Error");

void MainWindow::setPortableConfig(bool state) {
    ui->checkPortable->setChecked(state);
    QString debugPath;
    QString imagePath;
    QString setPath;
    QString romPath;
    QDir dir = qApp->applicationDirPath();

    if (state) {
        setPath = qApp->applicationDirPath() + SETTING_DEFAULT_FILE;
        QFile::copy(pathSettings, setPath);
    } else {
        setPath = configPath + SETTING_DEFAULT_FILE;
        QFile(pathSettings).remove();
    }
    debugPath = QDir::cleanPath(QFileInfo(setPath).absoluteDir().absolutePath() + SETTING_DEFAULT_DEBUG_FILE);
    imagePath =  QDir::cleanPath(QFileInfo(setPath).absoluteDir().absolutePath() + SETTING_DEFAULT_IMAGE_FILE);

    if(state) {
        debugPath = dir.relativeFilePath(debugPath);
        imagePath = dir.relativeFilePath(imagePath);
        romPath = dir.relativeFilePath(settings->value(SETTING_ROM_PATH).toString());
        settings->setValue(SETTING_ROM_PATH, romPath);
        ui->rompathView->setText(romPath);
        ui->settingsPath->setText(dir.relativeFilePath(setPath));
    } else {
        ui->settingsPath->setText(setPath);
    }

    delete settings;
    settings = new QSettings(setPath, QSettings::IniFormat);
    settings->setValue(SETTING_DEBUGGER_IMAGE_PATH, debugPath);
    settings->setValue(SETTING_IMAGE_PATH, imagePath);

    ui->savedImagePath->setText(imagePath);
    ui->savedDebugPath->setText(debugPath);
    emu.image = imagePath;
    pathSettings = setPath;
    settings->sync();
    portable = state;
    ui->buttonChangeSavedDebugPath->setEnabled(!portable);
    ui->buttonChangeSavedImagePath->setEnabled(!portable);
}

bool MainWindow::checkForCEmuBootImage() {
    QDirIterator dirIt(qApp->applicationDirPath(), QDirIterator::NoIteratorFlags);
    while (dirIt.hasNext()) {
        dirIt.next();
        QString dirItFile = dirIt.filePath();
        if (QFileInfo(dirItFile).isFile()) {
            if (QFileInfo(dirItFile).suffix() == QStringLiteral("cemu")) {
                if (!loadedCEmuBootImage) {
                    loadedCEmuBootImage = loadCEmuBootImage(dirItFile);
                }
                QFile(dirItFile).remove();
            }
        }
    }
    return loadedCEmuBootImage;
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
    pathSettings = newSettingsPath;
    return true;
}

void MainWindow::exportCEmuBootImage() {
    QMessageBox::information(this, tr("Information"), tr("A bootable image can be used to start CEmu with predefined configurations, without the need for any extra setup."
                                                         "\n\nOnce done, a bootable image should be placed in the same directory as the CEmu executable. When CEmu is then started, "
                                                         "the boot image will be loaded automatically and then removed for convience."));

    QString saveImage = QFileDialog::getSaveFileName(this, tr("Save bootable CEmu image"),
                                                      currDir.absolutePath(),
                                                      tr("Bootable CEmu images (*.cemu);"));
    saveMiscSettings();
    settings->sync();

    if (!saveImage.isEmpty()) {
        currDir = QFileInfo(saveImage).absoluteDir();
        QFile romFile(emu.rom);
        if (!romFile.open(QIODevice::ReadOnly)) return;
        QByteArray romData = romFile.readAll();

        QFile settingsFile(pathSettings);
        if (!settingsFile.open(QIODevice::ReadOnly)) return;
        QByteArray settingsData = settingsFile.readAll();

        QFile writter(saveImage);
        writter.open(QIODevice::WriteOnly);
        writter.write(romData);
        writter.write(settingsData);
        romFile.close();
        settingsFile.close();
        writter.close();
    }
}

void MainWindow::setEnableSoftCommands(bool state) {
    ui->checkDisableSoftCommands->blockSignals(true);
    ui->checkDisableSoftCommands->setChecked(state);
    ui->checkDisableSoftCommands->blockSignals(false);
    settings->setValue(SETTING_DEBUGGER_ENABLE_SOFT, state);
    enabledSoftCommands = state;
}

void MainWindow::setDataCol(bool state) {
    ui->checkDataCol->setChecked(state);
    settings->setValue(SETTING_DEBUGGER_DATA_COL, state);
    useDataCol = state;
}

void MainWindow::saveMiscSettings() {
    settings->setValue(SETTING_WINDOW_STATE,                saveState(WindowStateVersion));
    settings->setValue(SETTING_WINDOW_GEOMETRY,             saveGeometry());
    settings->setValue(SETTING_WINDOW_SIZE,                 size());
    settings->setValue(SETTING_CURRENT_DIR,                 currDir.absolutePath());
    settings->setValue(SETTING_DEBUGGER_FLASH_BYTES,        ui->flashBytes->value());
    settings->setValue(SETTING_DEBUGGER_RAM_BYTES,          ui->ramBytes->value());
    settings->setValue(SETTING_DEBUGGER_MEM_BYTES,          ui->memBytes->value());
}

void MainWindow::setMenuBarState(bool state) {
    ui->menubar->setHidden(state);
    ui->actionDisableMenuBar->setChecked(state);
    settings->setValue(SETTING_DISABLE_MENUBAR, state);
}

void MainWindow::resetSettingsIfLoadedCEmuBootableImage() {
    if (loadedCEmuBootImage) {
        settings->setValue(SETTING_FIRST_RUN, false);
    }
}

void MainWindow::setDebugResetTrigger(bool state) {
    ui->checkDebugResetTrigger->setChecked(state);
    settings->setValue(SETTING_DEBUGGER_RESET_OPENS, state);
    debugger.resetOpensDebugger = state;
}

void MainWindow::setAutoSaveState(bool state) {
    ui->checkSaveRestore->setChecked(state);
    settings->setValue(SETTING_RESTORE_ON_OPEN, state);
    settings->setValue(SETTING_SAVE_ON_CLOSE, state);
}

void MainWindow::setSaveDebug(bool state) {
    ui->checkSaveLoadDebug->setChecked(state);
    settings->setValue(SETTING_DEBUGGER_SAVE_ON_CLOSE, state);
    settings->setValue(SETTING_DEBUGGER_RESTORE_ON_OPEN, state);
}

void MainWindow::setSpaceDisasm(bool state) {
    ui->checkAddSpace->setChecked(state);
    settings->setValue(SETTING_DEBUGGER_ADD_DISASM_SPACE, state);
    disasm.space = state ? ", " : ",";
}

void MainWindow::setFont(int fontSize) {
    ui->textSize->setValue(fontSize);
    settings->setValue(SETTING_DEBUGGER_TEXT_SIZE, ui->textSize->value());

    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    monospace.setPointSize(fontSize);
    ui->console->setFont(monospace);
    ui->opView->setFont(monospace);
    ui->vatView->setFont(monospace);
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
    settings->setValue(SETTING_KEYPAD_COLOR, color);
}

void MainWindow::setImagePath() {
    QString saveImagePath = QFileDialog::getSaveFileName(this, tr("Set saved image to restore from"),
                                                           currDir.absolutePath(),
                                                           tr("CEmu images (*.ce);;All files (*.*)"));
    if (!saveImagePath.isEmpty()) {
        currDir = QFileInfo(saveImagePath).absoluteDir();
        settings->setValue(SETTING_IMAGE_PATH, saveImagePath);
        ui->savedImagePath->setText(saveImagePath);
    }
}

void MainWindow::setDebugPath() {
    QString savePath = QFileDialog::getSaveFileName(this, tr("Set debugging information path"),
                                                           currDir.absolutePath(),
                                                           tr("Debugging information (*.ini);;All files (*.*)"));
    if (!savePath.isEmpty()) {
        currDir = QFileInfo(savePath).absoluteDir();
        settings->setValue(SETTING_DEBUGGER_IMAGE_PATH, savePath);
        ui->savedDebugPath->setText(savePath);
    }
}

void MainWindow::setUIStyle(bool docks_enabled) {
    // Already in this mode?
    if (docks_enabled == ui->tabWidget->isHidden()) {
        return;
    }

    // Create "Docks" menu to make closing and opening docks more intuitive
    docksMenu = new QMenu(tr("Docks"), this);
    ui->menubar->insertMenu(ui->menuAbout->menuAction(), docksMenu);

    //Convert the tabs into QDockWidgets
    DockWidget *last_dock = nullptr;
    while (ui->tabWidget->count()) {
        DockWidget *dw = new DockWidget(ui->tabWidget, this);

        // Fill "Docks" menu
        QAction *action = dw->toggleViewAction();
        action->setIcon(dw->windowIcon());
        docksMenu->addAction(action);

        addDockWidget(Qt::RightDockWidgetArea, dw);
        if (last_dock != Q_NULLPTR)
            tabifyDockWidget(last_dock, dw);

        last_dock = dw;
    }

    docksMenu->addSeparator();
    toggleAction = new QAction(tr("Enable UI edit mode"), this);
    toggleAction->setCheckable(true);
    docksMenu->addAction(toggleAction);
    connect(toggleAction, &QAction::triggered, this, &MainWindow::toggleUIEditMode);

    ui->tabWidget->setHidden(true);
}

void MainWindow::toggleUIEditMode(void) {
    setUIEditMode(!uiEditMode);
}

void MainWindow::setUIEditMode(bool mode) {
    uiEditMode = mode;
    settings->setValue(SETTING_UI_EDIT_MODE, uiEditMode);
    toggleAction->setChecked(uiEditMode);
    for (const auto& dock : findChildren<DockWidget *>()) {
        dock->toggleState(uiEditMode);
    }
}

void MainWindow::setThrottle(int mode) {
    ui->checkThrottle->setChecked(mode == Qt::Checked);
    emit changedThrottleMode(mode == Qt::Checked);
}

void MainWindow::setAutoCheckForUpdates(int state) {
    settings->setValue(SETTING_AUTOUPDATE, state);
    ui->checkUpdates->setChecked(state);

    if (state == Qt::Checked) {
        checkForUpdates(true);
    }
}

void MainWindow::checkForUpdates(bool forceInfoBox) {
    if (QStringLiteral(CEMU_VERSION).contains(QStringLiteral("dev"))) {
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
                    QMessageBox::information(this, tr("No update available"), tr("You already have the latest CEmu version (" CEMU_VERSION ")"));
                }
            } else {
                QMessageBox updateInfoBox(this);
                updateInfoBox.addButton(QMessageBox::Ok);
                updateInfoBox.setIconPixmap(QPixmap(":/icons/resources/icons/icon.png"));
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
    ui->lcdWidget->setFixedSize(w, h);
    ui->lcdWidget->move(skin ? 60 * scale : 0, skin ? 78 * scale : 0);
    if (skin) {
        w = 440 * scale;
        h = 351 * scale;
    }
    ui->calcSkinTop->resize(w, h);
    ui->screenWidgetContents->setFixedSize(w, h);
}

void MainWindow::setLCDScale(int scale) {
    int roundedScale = round(scale / 25.0) * 25;
    settings->setValue(SETTING_SCREEN_SCALE, roundedScale);
    ui->scaleLCD->setValue(roundedScale);
    adjustScreen();
}

void MainWindow::setSkinToggle(bool enable) {
    settings->setValue(SETTING_SCREEN_SKIN, enable);
    ui->checkSkin->setChecked(enable);
    adjustScreen();
}

void MainWindow::setLCDRefresh(int value) {
    settings->setValue(SETTING_SCREEN_REFRESH_RATE, value);
    ui->refreshLCD->setValue(value);
    ui->lcdWidget->refreshRate(value);
    changeFramerate();
}

void MainWindow::setEmuSpeed(int value) {
    int actualSpeed = value*10;
    settings->setValue(SETTING_EMUSPEED, value);
    ui->emulationSpeedLabel->setText(QString::number(actualSpeed).rightJustified(3, '0')+QStringLiteral("%"));
    ui->emulationSpeed->setValue(value);
    emit changedEmuSpeed(actualSpeed);
}

void MainWindow::selectKeypadColor() {
    QString sender_obj_name = sender()->objectName();
    unsigned int color = KEYPAD_BLACK;

    if (sender_obj_name == "buttonWhite")     color = KEYPAD_WHITE;
    if (sender_obj_name == "buttonBlack")     color = KEYPAD_BLACK;
    if (sender_obj_name == "buttonGolden")    color = KEYPAD_GOLDEN;
    if (sender_obj_name == "buttonPlum")      color = KEYPAD_PLUM;
    if (sender_obj_name == "buttonPink")      color = KEYPAD_PINK;
    if (sender_obj_name == "buttonRed")       color = KEYPAD_RED;
    if (sender_obj_name == "buttonLightning") color = KEYPAD_LIGHTNING;
    if (sender_obj_name == "buttonTrueBlue")  color = KEYPAD_TRUE_BLUE;
    if (sender_obj_name == "buttonDenim")     color = KEYPAD_DENIM;
    if (sender_obj_name == "buttonSilver")    color = KEYPAD_SILVER;
    if (sender_obj_name == "buttonSpaceGrey") color = KEYPAD_SPACEGREY;
    if (sender_obj_name == "buttonCoral")     color = KEYPAD_CORAL;
    if (sender_obj_name == "buttonMint")      color = KEYPAD_MINT;

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
    settings->setValue(SETTING_KEYPAD_KEYMAP, value);
    keypadBridge->setKeymap(value);
}

void MainWindow::setAlwaysOnTop(int state) {
    if (!state) {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    } else {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    }
    show();
    settings->setValue(SETTING_ALWAYS_ON_TOP, state);
    ui->checkAlwaysOnTop->setCheckState(Qt::CheckState(state));
}
