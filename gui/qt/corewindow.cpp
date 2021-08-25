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
#include "developer/basicwidget.h"
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
#include "developer/sourceswidget.h"
#include "developer/usbwidget.h"
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

#include <QtCore/QEvent>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtGui/QScreen>
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
      mCore{this}
{
    connect(&mCore, &CoreWrapper::softCmd, this, &CoreWindow::softCmd);

    auto *menubar = menuBar();

    mCalcsMenu = new QMenu(tr("Calculator"), this);
    mDocksMenu = new QMenu(tr("Docks"), this);
    mDevMenu = new QMenu(tr("Developer"), this);

    menubar->addMenu(mCalcsMenu);
    menubar->addMenu(mDocksMenu);
    menubar->addMenu(mDevMenu);

    auto *resetAction = mCalcsMenu->addAction(tr("Reset"));
    resetAction->setIcon(QIcon(QStringLiteral(":/assets/icons/synchronize.svg")));
    connect(resetAction, &QAction::triggered, this, &CoreWindow::resetEmu);

    mCalcsMenu->addSeparator();

    auto *importRomAction = mCalcsMenu->addAction(tr("Import ROM"));
    importRomAction->setIcon(QIcon(QStringLiteral(":/assets/icons/import.svg")));
    connect(importRomAction, &QAction::triggered, this, &CoreWindow::importRom);

    auto *exportRomAction = mCalcsMenu->addAction(tr("Export ROM"));
    exportRomAction->setIcon(QIcon(QStringLiteral(":/assets/icons/export.svg")));
    connect(exportRomAction, &QAction::triggered, this, &CoreWindow::exportRom);

    mCalcsMenu->addSeparator();

    auto *prefAction = mCalcsMenu->addAction(tr("Preferences"));
    prefAction->setIcon(QIcon(QStringLiteral(":/assets/icons/support.svg")));
    connect(prefAction, &QAction::triggered, this, &CoreWindow::showPreferences);

    auto *aboutAction = mCalcsMenu->addAction(tr("About"));
    aboutAction->setIcon(QIcon(QStringLiteral(":/assets/icons/about.svg")));
    connect(aboutAction, &QAction::triggered, qApp, &QApplication::quit);

    auto *quitAction = mCalcsMenu->addAction(tr("Quit"));
    quitAction->setIcon(QIcon(QStringLiteral(":/assets/icons/cross.svg")));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    createDockWidgets();

    mDocksMenu->addSeparator();

    auto *saveLayoutAction = mDocksMenu->addAction(tr("Save Layout"));
    saveLayoutAction->setIcon(QIcon(QStringLiteral(":/assets/icons/template.svg")));
    connect(saveLayoutAction, &QAction::triggered, this, &CoreWindow::saveLayout);

    auto *restoreLayoutAction = mDocksMenu->addAction(tr("Restore Layout"));
    restoreLayoutAction->setIcon(QIcon(QStringLiteral(":/assets/icons/reload_template.svg")));
    connect(restoreLayoutAction, &QAction::triggered, this, &CoreWindow::restoreLayout);

    setKeymap();

    if (!restoreLayout())
    {
        QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
        resize(screenGeometry.height() * .325, screenGeometry.height() * .8);
    }

    connect(this, &CoreWindow::romChanged, this, &CoreWindow::resetEmu);

    mCalcWidget->setColor(Settings::intOption(Settings::KeypadColor));

    resetEmu();
}

CoreWindow::~CoreWindow()
{
    while (!mDockedWidgets.empty())
    {
        delete mDockedWidgets.back().dock();
    }
}

DockedWidgetList &CoreWindow::dockedWidgets()
{
    return mDockedWidgets;
}

CoreWrapper &CoreWindow::core()
{
    return mCore;
}

void CoreWindow::createDockWidgets()
{
    Q_ASSERT(mDockedWidgets.empty());

    mCalcWidget = new CalculatorWidget{this};
    auto *capture = new CaptureWidget{this};
    auto *variable = new VariableWidget{this, QStringList{"test", "test2"}};
    auto *keyHistory = new KeyHistoryWidget{this};
    auto *state = new StateWidget{this};

    mCalcOverlay = new CalculatorOverlay(mCalcWidget);
    mCalcOverlay->setVisible(false);

    connect(mCalcOverlay, &CalculatorOverlay::createRom, this, &CoreWindow::createRom);
    connect(mCalcOverlay, &CalculatorOverlay::loadRom, this, &CoreWindow::importRom);

    connect(&mCore, &CoreWrapper::devChanged, mCalcWidget, &CalculatorWidget::setDev);
    connect(&mCore, &CoreWrapper::lcdFrame, mCalcWidget, &CalculatorWidget::lcdFrame);
    connect(mCalcWidget, &CalculatorWidget::keyPressed, keyHistory, &KeyHistoryWidget::add);
    connect(&mCore, &CoreWrapper::devChanged, mKeypadBridge, &QtKeypadBridge::setDev);
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
        {PortMonitor::Mode::R, 40 }
    };

    auto *console = new ConsoleWidget{this};
    auto *autotester = new AutotesterWidget{this};
    auto *basic = new BasicWidget{this};
    auto *clocks = new ClocksWidget{this};
    auto *control = new ControlWidget{this};
    auto *cpu = new CpuWidget{this};
    auto *devMisc = new DevMiscWidget{this};
    auto *disassembly = new DisassemblyWidget{this};
    auto *flashRam = new FlashRamWidget{this};
    auto *osStacks = new OsStacksWidget{this};
    auto *osVars = new OsVarsWidget{this};
    auto *portMonitor = new PortMonitorWidget{this, portmonitorList};
    auto *usbDevices = new UsbWidget{this};
    auto *sources = new SourcesWidget{this};
    auto *watchpoints = new WatchpointsWidget{this, watchpointList};
    auto *performance = new PerformanceWidget{this};

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
    mDevMenu->addAction(usbDevices->dock()->toggleAction());
    mDevMenu->addAction(sources->dock()->toggleAction());
    mDevMenu->addAction(basic->dock()->toggleAction());
    mDevMenu->addAction(autotester->dock()->toggleAction());

    mDevMenu->addSeparator();

    auto *memoryAction = mDevMenu->addAction(tr("Add Memory View"));
    memoryAction->setIcon(QIcon(QStringLiteral(":/assets/icons/add_grid.svg")));
    connect(memoryAction, &QAction::triggered, [this]
    {
        auto *memory = new MemoryWidget{this};
        memory->dock()->show();
    });

    auto *visualizerAction = mDevMenu->addAction(tr("Add LCD Visualizer"));
    visualizerAction->setIcon(QIcon(QStringLiteral(":/assets/icons/add_image.svg")));
    connect(visualizerAction, &QAction::triggered, [this]
    {
        auto *visualizer = new VisualizerWidget{this};
        connect(&mCore, &CoreWrapper::lcdFrame, visualizer, &VisualizerWidget::lcdFrame);
        visualizer->dock()->show();
    });

    connect(console, &ConsoleWidget::inputLine, &mCore, QOverload<const QString &>::of(&CoreWrapper::command));
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
            mCore.command({"store", "rom", romFile});
        }
    }
}

void CoreWindow::resetEmu()
{
    mCalcOverlay->setVisible(mCore.command({"load", "rom", Settings::textOption(Settings::RomFile)}));
}

void CoreWindow::showPreferences()
{
    SettingsDialog dialog;

    connect(&dialog, &SettingsDialog::changedKeypadColor, mCalcWidget, &CalculatorWidget::setColor);

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
                auto *visualizer = new VisualizerWidget{this, restoredDockWidget};
                connect(&mCore, &CoreWrapper::lcdFrame, visualizer, &VisualizerWidget::lcdFrame);
                dockedWidget = visualizer;
            }
            else if (name.startsWith(QLatin1String("Memory #")))
            {
                dockedWidget = new MemoryWidget{this, restoredDockWidget};
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

void CoreWindow::softCmd()
{
    for (auto &dockedWidget : mDockedWidgets)
    {
        dockedWidget.loadFromCore(mCore);
        dockedWidget.enableDebugWidgets(true);
    }
    for (auto &dockedWidget : mDockedWidgets)
    {
        //dockedWidget.enableDebugWidgets(false);
        dockedWidget.storeToCore(mCore);
    }
    mCore.wake();
}

void CoreWindow::closeEvent(QCloseEvent *)
{
    saveLayout(true);
}
