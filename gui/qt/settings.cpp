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

void MainWindow::setPortableConfig(bool state) {
    ui->checkPortable->setChecked(state);
    QString debugPath;
    QString imagePath;
    QString setPath;
    QString romPath;
    QDir dir = qApp->applicationDirPath();

    if (state) {
        setPath = qApp->applicationDirPath() + QStringLiteral("/cemu_config.ini");
        QFile::copy(pathSettings, setPath);
    } else {
        setPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/CEmu/cemu_config.ini");
        QFile(pathSettings).remove();
    }
    debugPath = QDir::cleanPath(QFileInfo(setPath).absoluteDir().absolutePath() + QStringLiteral("/cemu_debug.ini"));
    imagePath =  QDir::cleanPath(QFileInfo(setPath).absoluteDir().absolutePath() + QStringLiteral("/cemu_image.ce"));

    if(state) {
        debugPath = dir.relativeFilePath(debugPath);
        imagePath = dir.relativeFilePath(imagePath);
        romPath = dir.relativeFilePath(settings->value(QStringLiteral("romImage")).toString());
        settings->setValue(QStringLiteral("romImage"), romPath);
        ui->rompathView->setText(romPath);
        ui->settingsPath->setText(dir.relativeFilePath(setPath));
    } else {
        ui->settingsPath->setText(setPath);
    }

    delete settings;
    settings = new QSettings(setPath, QSettings::IniFormat);
    settings->setValue(QStringLiteral("savedDebugPath"), debugPath);
    settings->setValue(QStringLiteral("savedImagePath"), imagePath);

    ui->savedImagePath->setText(imagePath);
    ui->savedDebugPath->setText(debugPath);
    emu.image = imagePath;
    pathSettings = setPath;
    settings->sync();
    portable = state;
    ui->buttonChangeSavedDebugPath->setEnabled(!portable);
    ui->buttonChangeSavedImagePath->setEnabled(!portable);
}

void MainWindow::setAutoSaveState(bool state) {
    ui->checkSaveRestore->setChecked(state);
    settings->setValue(QStringLiteral("restoreOnOpen"), state);
    settings->setValue(QStringLiteral("saveOnClose"), state);
}

void MainWindow::setSaveDebug(bool state) {
    ui->checkSaveLoadDebug->setChecked(state);
    settings->setValue(QStringLiteral("saveDebugOnClose"), state);
    settings->setValue(QStringLiteral("loadDebugOnOpen"), state);
}

void MainWindow::setSpaceDisasm(bool state) {
    ui->checkAddSpace->setChecked(state);
    settings->setValue(QStringLiteral("addDisasmSpace"), state);
    disasm.spacing_string = state ? ", " : ",";
}

void MainWindow::setFont(int fontSize) {
    ui->textSizeSlider->setValue(fontSize);
    settings->setValue(QStringLiteral("textSize"), ui->textSizeSlider->value());

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

void MainWindow::setKeypadColor(unsigned color) {
    ui->keypadWidget->setType(get_device_type(), color);
}

void MainWindow::setImagePath() {
    QString saveImagePath = QFileDialog::getSaveFileName(this, tr("Set saved image to restore from"),
                                                           currentDir.absolutePath(),
                                                           tr("CEmu images (*.ce);;All files (*.*)"));
    if (!saveImagePath.isEmpty()) {
        currentDir = QFileInfo(saveImagePath).absoluteDir();
        settings->setValue(QStringLiteral("savedImagePath"), QVariant(saveImagePath.toStdString().c_str()));
        ui->savedImagePath->setText(saveImagePath);
    }
}

void MainWindow::setDebugPath() {
    QString savePath = QFileDialog::getSaveFileName(this, tr("Set debugging information path"),
                                                           currentDir.absolutePath(),
                                                           tr("Debugging information (*.ini);;All files (*.*)"));
    if (!savePath.isEmpty()) {
        currentDir = QFileInfo(savePath).absoluteDir();
        settings->setValue(QStringLiteral("savedDebugPath"), QVariant(savePath.toStdString().c_str()));
        ui->savedDebugPath->setText(savePath);
    }
}

void MainWindow::setUIStyle(bool docks_enabled) {
    // Already in this mode?
    if (docks_enabled == ui->tabWidget->isHidden()) {
        return;
    }

    // Create "Docks" menu to make closing and opening docks more intuitive
    QMenu *docksMenu = new QMenu(tr("Docks"), this);
    ui->menubar->insertMenu(ui->menuAbout->menuAction(), docksMenu);

    //Convert the tabs into QDockWidgets
    DockWidget *last_dock = nullptr;
    while (ui->tabWidget->count()) {
        DockWidget *dw = new DockWidget(ui->tabWidget, this);

        // Fill "Docks" menu
        QAction *action = dw->toggleViewAction();
        action->setIcon(dw->windowIcon());
        docksMenu->addAction(action);

        QWidget *tab = dw->widget();
        if (tab == ui->tabDebugger)
            debuggerDock = dw;

        addDockWidget(Qt::RightDockWidgetArea, dw);
        if (last_dock != nullptr)
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
    settings->setValue(QStringLiteral("uiMode"), uiEditMode);
    toggleAction->setChecked(uiEditMode);
    for (const auto& dock : findChildren<DockWidget *>()) {
        dock->toggleState(uiEditMode);
    }
}

void MainWindow::setThrottleMode(int mode) {
    ui->checkThrottle->setChecked(mode == Qt::Checked);
    emit changedThrottleMode(mode == Qt::Checked);
}

void MainWindow::setAutoCheckForUpdates(int state) {
    settings->setValue(QStringLiteral("autoUpdate"), state);
    ui->checkUpdates->setChecked(state);

    if (state == Qt::Checked) {
        checkForUpdates(true);
    }
}

void MainWindow::checkForUpdates(bool forceInfoBox) {
    if (QStringLiteral(CEMU_VERSION).contains(QStringLiteral("dev"))) {
        if (forceInfoBox) {
            QMessageBox::warning(this, tr("Update check disabled"), tr("Checking updates is disabled for development builds"));
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
    float scale = ui->scaleSlider->value() / 100.0;
    bool skin = ui->checkSkin->isChecked();
    ui->calcSkinTop->setVisible(skin);
    float w, h;
    w = 320 * scale;
    h = 240 * scale;
    ui->lcdWidget->setFixedSize(w, h);
    ui->lcdWidget->move(skin ? 60 * scale : 0, skin ? 78 * scale : 0);
    if (skin) {
        w = 440 * scale;
        h = 351 * scale;
    }
    ui->calcSkinTop->resize(w, h);
    ui->screenWidgetContents->setFixedSize(w, h);
}

int MainWindow::setReprintScale(int scale) {
    int roundedScale = round(scale / 25.0) * 25;
    ui->scaleLabel->setText(QString::number(roundedScale) + "%");
    return roundedScale;
}

void MainWindow::setLCDScale(int scale) {
    int roundedScale = setReprintScale(scale);
    settings->setValue(QStringLiteral("scale"), roundedScale);
    ui->scaleSlider->setValue(roundedScale);
    adjustScreen();
}

void MainWindow::setSkinToggle(bool enable) {
    settings->setValue(QStringLiteral("skin"), enable);
    ui->checkSkin->setChecked(enable);
    adjustScreen();
}

void MainWindow::setLCDRefresh(int value) {
    settings->setValue(QStringLiteral("refreshRate"), value);
    ui->refreshLabel->setText(QString::number(value)+" FPS");
    ui->refreshSlider->setValue(value);
    ui->lcdWidget->refreshRate(value);
    changeFramerate();
}

void MainWindow::setEmulatedSpeed(int value) {
    int actualSpeed = value*10;
    settings->setValue(QStringLiteral("emuRate"), value);
    ui->emulationSpeedLabel->setText(QString::number(actualSpeed).rightJustified(3, '0')+QStringLiteral("%"));
    ui->emulationSpeed->setValue(value);
    emit setEmuSpeed(actualSpeed);
}

void MainWindow::selectKeypadColor() {
    QString sender_obj_name = sender()->objectName();
    unsigned keypad_color = KEYPAD_BLACK;

    if (sender_obj_name == "buttonWhite")     keypad_color = KEYPAD_WHITE;
    if (sender_obj_name == "buttonBlack")     keypad_color = KEYPAD_BLACK;
    if (sender_obj_name == "buttonGolden")    keypad_color = KEYPAD_GOLDEN;
    if (sender_obj_name == "buttonPlum")      keypad_color = KEYPAD_PLUM;
    if (sender_obj_name == "buttonPink")      keypad_color = KEYPAD_PINK;
    if (sender_obj_name == "buttonRed")       keypad_color = KEYPAD_RED;
    if (sender_obj_name == "buttonLightning") keypad_color = KEYPAD_LIGHTNING;
    if (sender_obj_name == "buttonTrueBlue")  keypad_color = KEYPAD_TRUE_BLUE;
    if (sender_obj_name == "buttonDenim")     keypad_color = KEYPAD_DENIM;
    if (sender_obj_name == "buttonSilver")    keypad_color = KEYPAD_SILVER;

    setKeypadColor(keypad_color);
}

void MainWindow::keymapChanged() {
    if (ui->radioCEmuKeys->isChecked()) {
        setKeymap(QStringLiteral("cemu"));
    } else if (ui->radioTilEmKeys->isChecked()) {
        setKeymap(QStringLiteral("tilem"));
    } else if (ui->radioWabbitemuKeys->isChecked()) {
        setKeymap(QStringLiteral("wabbitemu"));
    } else if (ui->radiojsTIfiedKeys->isChecked()) {
        setKeymap(QStringLiteral("jsTIfied"));
    }
}

void MainWindow::setKeymap(const QString & value) {
    settings->setValue(QStringLiteral("keyMap"), value);
    keypadBridge.setKeymap(value);
}

void MainWindow::setAlwaysOnTop(int state) {
    if (!state) {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    } else {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    }
    show();
    settings->setValue(QStringLiteral("onTop"), state);
    ui->checkAlwaysOnTop->setCheckState(Qt::CheckState(state));
}
