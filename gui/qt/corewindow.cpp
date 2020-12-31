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
#include "capturewidget.h"
#include "consolewidget.h"
#include "developer/autotesterwidget.h"
#include "developer/clockswidget.h"
#include "developer/controlwidget.h"
#include "developer/cpuwidget.h"
#include "developer/devmiscwidget.h"
#include "developer/disassemblywidget.h"
#include "developer/flashramwidget.h"
#include "developer/memorywidget.h"
#include "developer/osstackswidget.h"
#include "developer/osvarswidget.h"
#include "developer/performancewidget.h"
#include "developer/portmonitorwidget.h"
#include "developer/visualizerwidget.h"
#include "developer/watchpointswidget.h"
#include "dockwidget.h"
#include "keyhistorywidget.h"
#include "keypad/qtkeypadbridge.h"
#include "romdialog.h"
#include "settings.h"
#include "settingsdialog.h"
#include "statewidget.h"
#include "util.h"
#include "variablewidget.h"

#include <kddockwidgets/LayoutSaver.h>

#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QTextEdit>

CoreWindow::CoreWindow(const QString &uniqueName,
                       KDDockWidgets::MainWindowOptions options,
                       QWidget *parent)
    : KDDockWidgets::MainWindow(uniqueName, options, parent),
      mKeypadBridge{new QtKeypadBridge{this}},
      mCalcOverlay{nullptr},
      mCalcType{ti_device_t::TI84PCE},
      mCore{nullptr}
{
    auto *menubar = menuBar();

    mCalcsMenu = new QMenu(tr("Calculator"), this);
    mDocksMenu = new QMenu(tr("Docks"), this);
    mDevMenu = new QMenu(tr("Developer"), this);

    menubar->addMenu(mCalcsMenu);
    menubar->addMenu(mDocksMenu);
    menubar->addMenu(mDevMenu);

    auto *resetAction = mCalcsMenu->addAction(tr("Reset"));
    connect(resetAction, &QAction::triggered, this, &CoreWindow::resetEmu);

    mCalcsMenu->addSeparator();

    auto *importRomAction = mCalcsMenu->addAction(tr("Import ROM"));
    connect(importRomAction, &QAction::triggered, this, &CoreWindow::importRom);

    auto *exportRomAction = mCalcsMenu->addAction(tr("Export ROM"));
    connect(exportRomAction, &QAction::triggered, this, &CoreWindow::exportRom);

    mCalcsMenu->addSeparator();

    auto *prefAction = mCalcsMenu->addAction(tr("Preferences"));
    connect(prefAction, &QAction::triggered, this, &CoreWindow::showPreferences);

    auto *quitAction = mCalcsMenu->addAction(tr("Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    createDockWidgets();

    mDocksMenu->addSeparator();

    auto *saveLayoutAction = mDocksMenu->addAction(tr("Save Layout"));
    connect(saveLayoutAction, &QAction::triggered, this, &CoreWindow::saveLayout);

    auto *restoreLayoutAction = mDocksMenu->addAction(tr("Restore Layout"));
    connect(restoreLayoutAction, &QAction::triggered, this, &CoreWindow::restoreLayout);

    setKeymap();

    if (!restoreLayout())
    {
        qDebug() << "Failed to restore layout";
        QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
        resize(screenGeometry.height() * .325, screenGeometry.height() * .8);
    }

    connect(this, &CoreWindow::romChanged, this, &CoreWindow::resetEmu);

    resetEmu();
}

CoreWindow::~CoreWindow()
{
    while (!mDockedWidgets.empty())
    {
        delete mDockedWidgets.back().dock();
    }
    cemucore_destroy(mCore);
}

void CoreWindow::createDockWidgets()
{
    Q_ASSERT(mDockedWidgets.empty());

    mCalcWidget = new CalculatorWidget{mDockedWidgets};
    auto *capture = new CaptureWidget{mDockedWidgets};
    auto *variable = new VariableWidget{mDockedWidgets, QStringList{"test", "test2"}};
    auto *keyHistory = new KeyHistoryWidget{mDockedWidgets};
    auto *state = new StateWidget{mDockedWidgets};

    mCalcOverlay = new CalculatorOverlay(mCalcWidget);
    mCalcOverlay->setVisible(false);

    connect(mCalcOverlay, &CalculatorOverlay::createRom, this, &CoreWindow::createRom);
    connect(mCalcOverlay, &CalculatorOverlay::loadRom, this, &CoreWindow::importRom);

    connect(mCalcWidget, &CalculatorWidget::keyPressed, keyHistory, &KeyHistoryWidget::add);
    connect(mKeypadBridge, &QtKeypadBridge::keyStateChanged, mCalcWidget, &CalculatorWidget::changeKeyState);
    mCalcWidget->installEventFilter(mKeypadBridge);

    mDocksMenu->addAction(mCalcWidget->dock()->toggleAction());
    mDocksMenu->addAction(capture->dock()->toggleAction());
    mDocksMenu->addAction(keyHistory->dock()->toggleAction());
    mDocksMenu->addAction(variable->dock()->toggleAction());
    mDocksMenu->addAction(state->dock()->toggleAction());

    addDockWidget(mCalcWidget->dock(), KDDockWidgets::Location_OnTop);

    createDeveloperWidgets();
}

void CoreWindow::createDeveloperWidgets()
{
    QList<Watchpoint> watchpointList =
    {
        {Watchpoint::Mode::R, 10, 5, "test"},
        {Watchpoint::Mode::W, 20, 15, "test2"},
        {Watchpoint::Mode::R, 30, 25, "test3"},
        {Watchpoint::Mode::X, 40, 35, "test4"}
    };
    QList<PortMonitor> portmonitorList =
    {
        {PortMonitor::Mode::R, 10 },
        {PortMonitor::Mode::W, 20 },
        {PortMonitor::Mode::W | PortMonitor::Mode::R, 30 },
        {PortMonitor::Mode::F, 40 }
    };

    auto *console = new ConsoleWidget{mDockedWidgets};
    auto *autotester = new AutotesterWidget{mDockedWidgets};
    auto *clocks = new ClocksWidget{mDockedWidgets};
    auto *control = new ControlWidget{mDockedWidgets};
    auto *cpu = new CpuWidget{mDockedWidgets};
    auto *devMisc = new DevMiscWidget{mDockedWidgets};
    auto *disassembly = new DisassemblyWidget{mDockedWidgets};
    auto *flashRam = new FlashRamWidget{mDockedWidgets};
    auto *osStacks = new OsStacksWidget{mDockedWidgets};
    auto *osVars = new OsVarsWidget{mDockedWidgets};
    auto *portMonitor = new PortMonitorWidget{mDockedWidgets, portmonitorList};
    auto *watchpoints = new WatchpointsWidget{mDockedWidgets, watchpointList};
    auto *performance = new PerformanceWidget{mDockedWidgets};

    mDevMenu->addAction(console->dock()->toggleAction());
    mDevMenu->addAction(control->dock()->toggleAction());
    mDevMenu->addAction(cpu->dock()->toggleAction());
    mDevMenu->addAction(disassembly->dock()->toggleAction());
    mDevMenu->addAction(watchpoints->dock()->toggleAction());
    mDevMenu->addAction(flashRam->dock()->toggleAction());
    mDevMenu->addAction(clocks->dock()->toggleAction());
    mDevMenu->addAction(portMonitor->dock()->toggleAction());
    mDevMenu->addAction(osVars->dock()->toggleAction());
    mDevMenu->addAction(osStacks->dock()->toggleAction());
    mDevMenu->addAction(devMisc->dock()->toggleAction());
    mDevMenu->addAction(performance->dock()->toggleAction());
    mDevMenu->addAction(autotester->dock()->toggleAction());

    mDevMenu->addSeparator();

    auto *memoryAction = mDevMenu->addAction(tr("Add Memory View"));
    connect(memoryAction, &QAction::triggered, [this]
    {
        auto *memory = new MemoryWidget{mDockedWidgets};
        memory->dock()->show();
    });

    auto *visualizerAction = mDevMenu->addAction(tr("Add LCD Visualizer"));
    connect(visualizerAction, &QAction::triggered, [this]
    {
        auto *visualizer = new VisualizerWidget{mDockedWidgets};
        visualizer->dock()->show();
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

void CoreWindow::importRom()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setWindowTitle(tr("Import ROM"));
    dialog.setNameFilter(tr("ROM Image (*.rom *.Rom *.ROM);;All Files (*.*)"));

    if (dialog.exec())
    {
        QString romFile = dialog.selectedFiles().first();

        Settings::setTextOption(Settings::RomFile, romFile);
        emit romChanged();
    }
}

void CoreWindow::exportRom()
{
    QFileDialog dialog(this, tr("Export ROM"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix(QStringLiteral(".rom"));
    dialog.setNameFilter(QStringLiteral("*.rom"));

    if (dialog.exec())
    {
        const QString romFile = dialog.selectedFiles().first();
        if (!romFile.isEmpty())
        {
            // temporary for testing
            QFile file(romFile);
            file.open(QIODevice::WriteOnly);
            file.putChar('R');
            file.close();
        }
    }
}

void CoreWindow::resetEmu()
{
    if (!mCore)
    {
        mCore = cemucore_init(CEMUCORE_INIT_CREATE_THREAD);
    }

    int keycolor = Settings::intOption(Settings::KeypadColor);

    // holds the path to the rom file to load into the emulator
    //Settings::textOption(Settings::RomFile);
    bool test = true;

    if (test)
    {

        mCalcWidget->setConfig(mCalcType, keycolor);
        mCalcOverlay->setVisible(false);
    }
    else
    {
        mCalcWidget->setConfig(mCalcType, keycolor);
        mCalcOverlay->setVisible(true);
    }
}

void CoreWindow::showPreferences()
{
    SettingsDialog dialog;

    connect(&dialog, &SettingsDialog::changedKeypadColor, [this](int color)
    {
        mCalcWidget->setConfig(mCalcType, color);
    });

    if (dialog.exec())
    {
        switch (dialog.checkForReset())
        {
            default:
            case Settings::Reset::None:
                break;
            case Settings::Reset::Langauge:
            case Settings::Reset::Portable:
                close();
                qApp->exit(CoreWindow::Restart);
                break;
            case Settings::Reset::Defaults:
                close();
                Settings::setDefaults(true);
                qApp->exit(CoreWindow::Restart);
                break;
            case Settings::Reset::Gui:
                close();
                QFile::remove(Settings::textOption(Settings::LayoutFile));
                qApp->exit(CoreWindow::Restart);
                break;
            case Settings::Reset::All:
                close();
                QDir tmp(Settings::textOption(Settings::SettingsPath));
                tmp.removeRecursively();
                qApp->exit(CoreWindow::Restart);
                break;
        }
    }
}

bool CoreWindow::saveLayout(bool ignoreErrors)
{
    KDDockWidgets::LayoutSaver saver;
    QJsonObject json;
    json[QLatin1String("layout")] = QJsonDocument::fromJson(saver.serializeLayout()).object();
    for (auto &dockedWidget : mDockedWidgets)
    {
        json[dockedWidget.dock()->uniqueName()] = dockedWidget.serialize();
    }

    QFile file(Settings::textOption(Settings::LayoutFile));
    if (!file.open(QIODevice::WriteOnly))
    {
        if (!ignoreErrors)
        {
            QMessageBox::critical(nullptr, Util::error, tr("Unable to save layout. Ensure that the preferences directory is writable and has the required permissions."));
        }
        return false;
    }
    file.write(QJsonDocument(json).toJson());
    return true;
}

bool CoreWindow::restoreLayout()
{
    QFile file(Settings::textOption(Settings::LayoutFile));
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }
    auto json = QJsonDocument::fromJson(file.readAll()).object();

    KDDockWidgets::RestoreOptions options = KDDockWidgets::RestoreOption_None;
    KDDockWidgets::LayoutSaver saver(options);
    if (!saver.restoreLayout(QJsonDocument(json.take(QLatin1String("layout")).toObject()).toJson()))
    {
        return false;
    }

    for (auto *restoredDockWidget : saver.restoredDockWidgets())
    {
        auto name = restoredDockWidget->uniqueName();
        auto *dockedWidget = static_cast<DockedWidget *>(restoredDockWidget->widget());
        if (!dockedWidget)
        {
            if (name.startsWith(QLatin1String("Visualizer #")))
            {
                dockedWidget = new VisualizerWidget{mDockedWidgets, restoredDockWidget};
            }
            else if (name.startsWith(QLatin1String("Memory #")))
            {
                dockedWidget = new MemoryWidget{mDockedWidgets, restoredDockWidget};
            }
            else
            {
                qInfo() << "Unknown spawnable dock:" << name;
                continue;
            }
        }
        dockedWidget->unserialize(json.take(name));
    }

    return json.isEmpty();
}

void CoreWindow::closeEvent(QCloseEvent *)
{
    saveLayout(true);
}
