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

#include "corewindow.h"
#include "calculatorwidget.h"
#include "keyhistorywidget.h"
#include "consolewidget.h"
#include "statewidget.h"

#include <kddockwidgets/LayoutSaver.h>

#include <QMenu>
#include <QMenuBar>
#include <QEvent>
#include <QDebug>
#include <QString>
#include <QTextEdit>
#include <QApplication>
#include <QMessageBox>

CoreWindow::CoreWindow(const QString &uniqueName,
                       KDDockWidgets::MainWindowOptions options,
                       QWidget *parent)
    : KDDockWidgets::MainWindow(uniqueName, options, parent)
{
    auto menubar = menuBar();

    mCalcsMenu = new QMenu(tr("Calculator"), this);
    mCaptureMenu = new QMenu(tr("Capture"), this);
    mDocksMenu = new QMenu(tr("Docks"), this);
    mDebugMenu = new QMenu(tr("Developer"), this);

    menubar->addMenu(mCalcsMenu);
    menubar->addMenu(mCaptureMenu);
    menubar->addMenu(mDocksMenu);
    menubar->addMenu(mDebugMenu);

    auto resetAction = mCalcsMenu->addAction(tr("Reset"));
    connect(resetAction, &QAction::triggered, qApp, &QApplication::quit);

    auto romAction = mCalcsMenu->addAction(tr("Load ROM..."));
    connect(romAction, &QAction::triggered, qApp, &QApplication::quit);

    mCalcsMenu->addSeparator();

    auto prefAction = mCalcsMenu->addAction(tr("Preferences"));
    connect(prefAction, &QAction::triggered, qApp, &QApplication::quit);

    auto quitAction = mCalcsMenu->addAction(tr("Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    auto screenshotAction = mCaptureMenu->addAction(tr("Capture Screenshot"));
    connect(screenshotAction, &QAction::triggered, qApp, &QApplication::quit);

    auto animatedAction = mCaptureMenu->addAction(tr("Record Animated Screen"));
    connect(animatedAction, &QAction::triggered, qApp, &QApplication::quit);

    auto copyScreenAction = mCaptureMenu->addAction(tr("Copy Screen to Clipboard"));
    connect(copyScreenAction, &QAction::triggered, qApp, &QApplication::quit);

    createDockWidgets();

    auto saveLayoutAction = mDocksMenu->addAction(tr("Save Layout"));
    connect(saveLayoutAction, &QAction::triggered, this, [] {
        KDDockWidgets::LayoutSaver saver;
        const bool result = saver.saveToFile(QStringLiteral("mylayout.json"));
        qDebug() << "Saving layout to disk. Result=" << result;
    });

    auto restoreLayoutAction = mDocksMenu->addAction(tr("Restore Layout"));
    connect(restoreLayoutAction, &QAction::triggered, this, [] {
        KDDockWidgets::RestoreOptions options = KDDockWidgets::RestoreOption_None;
        KDDockWidgets::LayoutSaver saver(options);
        saver.restoreFromFile(QStringLiteral("mylayout.json"));
    });

}

CoreWindow::~CoreWindow()
{
    qDeleteAll(mDockWidgets);
}

void CoreWindow::createDockWidgets()
{
    Q_ASSERT(mDockWidgets.isEmpty());

    mKeypadBridge.setKeymap(QtKeypadBridge::KEYMAP_CEMU);

    auto *calcDock = new KDDockWidgets::DockWidget(tr("Calculator"));
    auto *calc = new CalculatorWidget();

    auto *calcOverlay = new CalculatorOverlay(calc);

    auto *consoleDock = new KDDockWidgets::DockWidget(tr("Console"));
    auto *console = new ConsoleWidget();

    auto *keyHistoryDock = new KDDockWidgets::DockWidget(tr("Key History"));
    auto *keyHistory = new KeyHistoryWidget();

    auto *stateDock = new KDDockWidgets::DockWidget(tr("States"));
    auto *state = new StateWidget();

    calc->setConfig(ti_device_t::TI84PCE, KeypadWidget::KeypadColor::COLOR_DENIM);

    calcDock->setWidget(calc);
    keyHistoryDock->setWidget(keyHistory);
    consoleDock->setWidget(console);
    stateDock->setWidget(state);

    connect(calc, &CalculatorWidget::keyPressed, keyHistory, &KeyHistoryWidget::add);
    connect(&mKeypadBridge, &QtKeypadBridge::keyStateChanged, calc, &CalculatorWidget::changeKeyState);
    calc->installEventFilter(&mKeypadBridge);

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
