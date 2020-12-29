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
#include "consolewidget.h"
#include "dockwidget.h"
#include "keyhistorywidget.h"
#include "romdialog.h"
#include "settings.h"
#include "settingsdialog.h"
#include "statewidget.h"
#include "util.h"

#include "developer/autotesterwidget.h"
#include "developer/breakpointswidget.h"
#include "developer/clockswidget.h"
#include "developer/controlwidget.h"
#include "developer/cpuwidget.h"
#include "developer/devmiscwidget.h"
#include "developer/disassemblywidget.h"
#include "developer/memorywidget.h"
#include "developer/osstackswidget.h"
#include "developer/osvarswidget.h"
#include "developer/portmonitorwidget.h"
#include "developer/watchpointswidget.h"
#include "developer/visualizerwidget.h"

#include "keypad/qtkeypadbridge.h"

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
    sCoreWindow = this;

    auto *menubar = menuBar();

    mCalcsMenu = new QMenu(tr("Calculator"), this);
    mCaptureMenu = new QMenu(tr("Capture"), this);
    mDocksMenu = new QMenu(tr("Docks"), this);
    mDevMenu = new QMenu(tr("Developer"), this);

    menubar->addMenu(mCalcsMenu);
    menubar->addMenu(mCaptureMenu);
    menubar->addMenu(mDocksMenu);
    menubar->addMenu(mDevMenu);

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
    connect(saveLayoutAction, &QAction::triggered, this, &CoreWindow::saveLayout);

    auto *restoreLayoutAction = mDocksMenu->addAction(tr("Restore Layout"));
    connect(restoreLayoutAction, &QAction::triggered, this, &CoreWindow::restoreLayout);

    setKeymap();
    restoreLayout();

    connect(this, &CoreWindow::romChanged, this, &CoreWindow::resetEmu);

    resetEmu();
}

CoreWindow::~CoreWindow()
{
    qDeleteAll(mDockWidgets);
    while (!mVisualizerWidgets.empty())
    {
        delete static_cast<VisualizerWidget *>(mVisualizerWidgets.prev())->parent();
    }

    sCoreWindow = nullptr;
}

KDDockWidgets::DockWidgetBase *CoreWindow::dockWidgetFactory(const QString &name)
{
    if (sCoreWindow)
    {
        if (name.startsWith("Visualizer #"))
        {
            return sCoreWindow->addVisualizerDock(name);
        }
    }
    return nullptr;
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

    createDeveloperWidgets();
}

void CoreWindow::createDeveloperWidgets()
{
    auto *autotesterDock = new KDDockWidgets::DockWidget(tr("Autotester"));
    auto *autotester = new AutotesterWidget();

    auto *breakpointsDock = new KDDockWidgets::DockWidget(tr("Breakpoints"));
    auto *breakpoints = new BreakpointsWidget();

    auto *clocksDock = new KDDockWidgets::DockWidget(tr("Clocks"));
    auto *clocks = new ClocksWidget();

    auto *controlDock = new KDDockWidgets::DockWidget(tr("Control"));
    auto *control = new ControlWidget();

    auto *cpuDock = new KDDockWidgets::DockWidget(tr("CPU"));
    auto *cpu = new CpuWidget();

    auto *devMiscDock = new KDDockWidgets::DockWidget(tr("Miscellaneous"));
    auto *devMisc = new DevMiscWidget();

    auto *dissassmeblyDock = new KDDockWidgets::DockWidget(tr("Disassembly"));
    auto *dissassmebly = new DisassemblyWidget();

    auto *memoryDock = new KDDockWidgets::DockWidget(tr("Memory"));
    auto *memory = new MemoryWidget();

    auto *osStacksDock = new KDDockWidgets::DockWidget(tr("OS Stacks"));
    auto *osStacks = new OsStacksWidget();

    auto *osVarsDock = new KDDockWidgets::DockWidget(tr("OS Variables"));
    auto *osVars = new OsVarsWidget();

    auto *portMonitorDock = new KDDockWidgets::DockWidget(tr("Port Monitor"));
    auto *portMonitor = new PortMonitorWidget();

    auto *watchpointsDock = new KDDockWidgets::DockWidget(tr("Watchpoints"));
    auto *watchpoints = new WatchpointsWidget();

    autotesterDock->setWidget(autotester);
    autotesterDock->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    breakpointsDock->setWidget(breakpoints);
    breakpointsDock->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    clocksDock->setWidget(clocks);
    clocksDock->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    controlDock->setWidget(control);
    controlDock->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    cpuDock->setWidget(cpu);
    cpuDock->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    devMiscDock->setWidget(devMisc);
    devMiscDock->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dissassmeblyDock->setWidget(dissassmebly);
    dissassmeblyDock->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    memoryDock->setWidget(memory);
    memoryDock->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    osStacksDock->setWidget(osStacks);
    osStacksDock->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    osVarsDock->setWidget(osVars);
    osVarsDock->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    portMonitorDock->setWidget(portMonitor);
    portMonitorDock->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    watchpointsDock->setWidget(watchpoints);
    watchpointsDock->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    mDockWidgets << autotesterDock;
    mDockWidgets << breakpointsDock;
    mDockWidgets << clocksDock;
    mDockWidgets << controlDock;
    mDockWidgets << cpuDock;
    mDockWidgets << devMiscDock;
    mDockWidgets << dissassmeblyDock;
    mDockWidgets << memoryDock;
    mDockWidgets << osStacksDock;
    mDockWidgets << osVarsDock;
    mDockWidgets << portMonitorDock;
    mDockWidgets << watchpointsDock;

    mDevMenu->addAction(controlDock->toggleAction());
    mDevMenu->addAction(cpuDock->toggleAction());
    mDevMenu->addAction(dissassmeblyDock->toggleAction());
    mDevMenu->addAction(memoryDock->toggleAction());
    mDevMenu->addAction(clocksDock->toggleAction());
    mDevMenu->addAction(breakpointsDock->toggleAction());
    mDevMenu->addAction(watchpointsDock->toggleAction());
    mDevMenu->addAction(portMonitorDock->toggleAction());
    mDevMenu->addAction(osVarsDock->toggleAction());
    mDevMenu->addAction(osStacksDock->toggleAction());
    mDevMenu->addAction(devMiscDock->toggleAction());
    mDevMenu->addAction(autotesterDock->toggleAction());

    mDevMenu->addSeparator();

    auto *memoryAction = mDevMenu->addAction(tr("Add Memory View"));
    connect(memoryAction, &QAction::triggered, this, &CoreWindow::restoreLayout);

    auto *visualizerAction = mDevMenu->addAction(tr("Add LCD Visualizer"));
    connect(visualizerAction, &QAction::triggered, [this]
    {
        addVisualizerDock();
    });
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

void CoreWindow::saveLayout()
{
    KDDockWidgets::LayoutSaver saver;
    const bool result = saver.saveToFile(Settings::textOption(Settings::LayoutFile));
    if (!result)
    {
        QMessageBox::critical(nullptr, sErrorStr, tr("Unable to save layout. Ensure that the preferences directory is writable and has the required permissions."));
    }
}

void CoreWindow::restoreLayout()
{
    KDDockWidgets::RestoreOptions options = KDDockWidgets::RestoreOption_None;
    KDDockWidgets::LayoutSaver saver(options);
    saver.restoreFromFile(Settings::textOption(Settings::LayoutFile));
}

KDDockWidgets::DockWidget *CoreWindow::addVisualizerDock(QString name, const QString &config)
{
    if (name.isNull())
    {
        do
        {
            name = QStringLiteral("Visualizer #") + Util::randomString(10);
        }
        while (KDDockWidgets::DockWidgetBase::byName(name));
    }

    auto *visualizerDock = new KDDockWidgets::DockWidget(name);
    auto *visualizer = new VisualizerWidget(&mVisualizerWidgets, config, visualizerDock);

    visualizerDock->setTitle(tr("Visualizer"));

    visualizerDock->setWidget(visualizer);
    visualizerDock->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    visualizerDock->show();

    return visualizerDock;
}

CoreWindow *CoreWindow::sCoreWindow;
