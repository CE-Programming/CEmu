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

#include <kddockwidgets/LayoutSaver.h>

#include <QMenu>
#include <QMenuBar>
#include <QEvent>
#include <QDebug>
#include <QString>
#include <QTextEdit>
#include <QApplication>

CoreWindow::CoreWindow(const QString &uniqueName,
                       KDDockWidgets::MainWindowOptions options,
                       QWidget *parent)
    : KDDockWidgets::MainWindow(uniqueName, options, parent)
{
    auto menubar = menuBar();
    auto fileMenu = new QMenu(tr("File"));
    menubar->addMenu(fileMenu);

    auto saveLayoutAction = fileMenu->addAction(tr("Save Layout"));
    connect(saveLayoutAction, &QAction::triggered, this, [] {
        KDDockWidgets::LayoutSaver saver;
        const bool result = saver.saveToFile(QStringLiteral("mylayout.json"));
        qDebug() << "Saving layout to disk. Result=" << result;
    });

    auto restoreLayoutAction = fileMenu->addAction(tr("Restore Layout"));
    connect(restoreLayoutAction, &QAction::triggered, this, [] {
        KDDockWidgets::RestoreOptions options = KDDockWidgets::RestoreOption_None;
        KDDockWidgets::LayoutSaver saver(options);
        saver.restoreFromFile(QStringLiteral("mylayout.json"));
    });

    auto quitAction = fileMenu->addAction(tr("Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    createDockWidgets();
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

    auto *keyHistoryDock = new KDDockWidgets::DockWidget(tr("Key History"));
    auto *keyHistory = new KeyHistoryWidget();

    calc->setConfig(ti_device_t::TI84PCE, KeypadWidget::KeypadColor::COLOR_DENIM);

    calcDock->setWidget(calc);
    keyHistoryDock->setWidget(keyHistory);

    connect(calc, &CalculatorWidget::keyPressed, keyHistory, &KeyHistoryWidget::add);
    connect(&mKeypadBridge, &QtKeypadBridge::keyStateChanged, calc, &CalculatorWidget::changeKeyState);
    calc->installEventFilter(&mKeypadBridge);

    mDockWidgets << calcDock;
    mDockWidgets << keyHistoryDock;

    addDockWidget(calcDock, KDDockWidgets::Location_OnTop);
    addDockWidget(keyHistoryDock, KDDockWidgets::Location_OnRight);
}
