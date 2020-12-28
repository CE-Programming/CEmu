/*
 * Copyright (c) 2015-2020 CE Programming.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "calculatorwidget.h"
#include "consolewidget.h"
#include "corewindow.h"
#include "keyhistorywidget.h"
#include "keypad/qtkeypadbridge.h"
#include "romdialog.h"
#include "settings.h"
#include "settingsdialog.h"
#include "statewidget.h"

#include <kddockwidgets/LayoutSaver.h>

#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QString>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QTextEdit>

const QString CoreWindow::sErrorStr        = CoreWindow::tr("Error");
const QString CoreWindow::sWarningStr      = CoreWindow::tr("Warning");
const QString CoreWindow::sInformationStr  = CoreWindow::tr("Information");

CoreWindow::CoreWindow(const QString &uniqueName,
                       KDDockWidgets::MainWindowOptions options,
                       QWidget *parent)
    : KDDockWidgets::MainWindow(uniqueName, options, parent),
      mKeypadBridge{new QtKeypadBridge{this}},
      mCalcOverlay{nullptr},
      mCalcType{ti_device_t::TI84PCE}
{
    auto *menubar = menuBar();

    mCalcsMenu = new QMenu(tr("Calculator"), this);
    mCaptureMenu = new QMenu(tr("Capture"), this);
    mDocksMenu = new QMenu(tr("Docks"), this);
    mDebugMenu = new QMenu(tr("Developer"), this);

    menubar->addMenu(mCalcsMenu);
    menubar->addMenu(mCaptureMenu);
    menubar->addMenu(mDocksMenu);
    menubar->addMenu(mDebugMenu);

    auto *resetAction = mCalcsMenu->addAction(tr("Reset"));
    connect(resetAction, &QAction::triggered, this, &CoreWindow::resetEmu);

    auto *romAction = mCalcsMenu->addAction(tr("Load ROM..."));
    connect(romAction, &QAction::triggered, this, &CoreWindow::loadRom);

    mCalcsMenu->addSeparator();

    auto *prefAction = mCalcsMenu->addAction(tr("Preferences"));
    connect(prefAction, &QAction::triggered, this, &CoreWindow::showPreferences);

    auto *quitAction = mCalcsMenu->addAction(tr("Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    auto *screenshotAction = mCaptureMenu->addAction(tr("Capture Screenshot"));
    connect(screenshotAction, &QAction::triggered, qApp, &QApplication::quit);

    auto *animatedAction = mCaptureMenu->addAction(tr("Record Animated Screen"));
    connect(animatedAction, &QAction::triggered, qApp, &QApplication::quit);

    auto *copyScreenAction = mCaptureMenu->addAction(tr("Copy Screen to Clipboard"));
    connect(copyScreenAction, &QAction::triggered, qApp, &QApplication::quit);

    createDockWidgets();

    auto *saveLayoutAction = mDocksMenu->addAction(tr("Save Layout"));
    connect(saveLayoutAction, &QAction::triggered, this, []
    {
        KDDockWidgets::LayoutSaver saver;
        const bool result = saver.saveToFile(Settings::textOption(Settings::LayoutFile));
        if (!result)
        {
            QMessageBox::critical(nullptr, sErrorStr, tr("Unable to save layout. Ensure that the preferences directory is writable and has the required permissions."));
        }
    });

    auto *restoreLayoutAction = mDocksMenu->addAction(tr("Restore Layout"));
    connect(restoreLayoutAction, &QAction::triggered, this, []
    {
        KDDockWidgets::RestoreOptions options = KDDockWidgets::RestoreOption_None;
        KDDockWidgets::LayoutSaver saver(options);
        const bool result = saver.restoreFromFile(Settings::textOption(Settings::LayoutFile));
        if (!result)
        {
            QMessageBox::critical(nullptr, sErrorStr, tr("Unable to load layout. Ensure that the preferences directory is readable and has the required permissions."));
        }
    });

    connect(this, &CoreWindow::romChanged, this, &CoreWindow::resetEmu);

    setKeymap();

    resetEmu();
}

CoreWindow::~CoreWindow()
{
    qDeleteAll(mDockWidgets);
}

void CoreWindow::createDockWidgets()
{
    Q_ASSERT(mDockWidgets.isEmpty());

    auto *calcDock = new KDDockWidgets::DockWidget(tr("Calculator"));
    mCalc = new CalculatorWidget();

    auto *consoleDock = new KDDockWidgets::DockWidget(tr("Console"));
    auto *console = new ConsoleWidget();

    auto *keyHistoryDock = new KDDockWidgets::DockWidget(tr("Key History"));
    auto *keyHistory = new KeyHistoryWidget();

    auto *stateDock = new KDDockWidgets::DockWidget(tr("States"));
    auto *state = new StateWidget();

    mCalcOverlay = new CalculatorOverlay(mCalc);
    mCalcOverlay->setVisible(false);

    connect(mCalcOverlay, &CalculatorOverlay::createRom, this, &CoreWindow::createRom);
    connect(mCalcOverlay, &CalculatorOverlay::loadRom, this, &CoreWindow::loadRom);

    calcDock->setWidget(mCalc);
    keyHistoryDock->setWidget(keyHistory);
    consoleDock->setWidget(console);
    stateDock->setWidget(state);

    connect(mCalc, &CalculatorWidget::keyPressed, keyHistory, &KeyHistoryWidget::add);
    connect(mKeypadBridge, &QtKeypadBridge::keyStateChanged, mCalc, &CalculatorWidget::changeKeyState);
    mCalc->installEventFilter(mKeypadBridge);

    mDockWidgets << calcDock;
    mDockWidgets << keyHistoryDock;
    mDockWidgets << consoleDock;
    mDockWidgets << stateDock;

    mDocksMenu->addAction(calcDock->toggleAction());
    mDocksMenu->addAction(keyHistoryDock->toggleAction());
    mDocksMenu->addAction(stateDock->toggleAction());
    mDocksMenu->addAction(consoleDock->toggleAction());

    addDockWidget(calcDock, KDDockWidgets::Location_OnTop);
}

void CoreWindow::setKeymap()
{
    Keymap keymap = static_cast<Keymap>(Settings::intOption(Settings::KeyMap));
    mKeypadBridge->setKeymap(keymap);
}

void CoreWindow::createRom()
{
    RomDialog romdialog;
    if (romdialog.exec())
    {
        QString romFile = romdialog.romFile();

        Settings::setTextOption(Settings::RomFile, romFile);
        emit romChanged();
    }
}

void CoreWindow::loadRom()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setWindowTitle(tr("Select ROM file"));
    dialog.setNameFilter(tr("ROM Image (*.rom *.Rom *.ROM);;All Files (*.*)"));
    if (dialog.exec())
    {
        QString romFile = dialog.selectedFiles().first();

        Settings::setTextOption(Settings::RomFile, romFile);
        emit romChanged();
    }
}

void CoreWindow::resetEmu()
{
    int keycolor = Settings::intOption(Settings::KeypadColor);

    // holds the path to the rom file to load into the emulator
    //Settings::textOption(Settings::RomFile);
    static bool test = false;

    if (test)
    {

        mCalc->setConfig(mCalcType, keycolor);
        mCalcOverlay->setVisible(false);
    }
    else
    {
        mCalc->setConfig(mCalcType, keycolor);
        mCalcOverlay->setVisible(true);
    }

    test = true;
}

void CoreWindow::showPreferences()
{
    SettingsDialog dialog;
    connect(&dialog, &SettingsDialog::changedKeypadColor, [this](int color)
    {
        mCalc->setConfig(mCalcType, color);
    });
    dialog.exec();
}
