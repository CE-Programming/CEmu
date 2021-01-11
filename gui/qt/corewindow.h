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

#ifndef COREWINDOW_H
#define COREWINDOW_H

#include "corewrapper.h"
#include "developer/visualizerwidget.h"
#include "developer/watchpointswidget.h"
class QtKeypadBridge;

#include <kddockwidgets/DockWidget.h>
#include <kddockwidgets/MainWindow.h>

#include <QtCore/QString>
QT_BEGIN_NAMESPACE
class QCloseEvent;
class QMenu;
QT_END_NAMESPACE

class CalculatorOverlay;
class CalculatorWidget;

class CoreWindow : public KDDockWidgets::MainWindow
{
    Q_OBJECT
public:
    explicit CoreWindow(const QString &uniqueName, KDDockWidgets::MainWindowOptions options, QWidget *parent = nullptr);
    ~CoreWindow() override;

    DockedWidgetList &dockedWidgets();
    CoreWrapper &core();

    enum ExitCode
    {
        Quit = 0,
        Restart = 1000
    };

signals:
    void romChanged();

private slots:
    void createRom();
    void importRom();
    void exportRom();
    void resetEmu();
    void showPreferences();
    bool saveLayout(bool ignoreErrors = false);
    bool restoreLayout();
    void softCmd();

private:
    void createFileMenu();
    void createDocksMenu();
    void createDebugMenu();
    void createExtrasMenu();

    void createDockWidgets();
    void createDeveloperWidgets();
    void setKeymap();

    void closeEvent(QCloseEvent *) override;

    DockedWidgetList mDockedWidgets;
    QStringList mVisualizerConfigs;

    QtKeypadBridge *mKeypadBridge;

    QMenu *mCalcsMenu;
    QMenu *mDocksMenu;
    QMenu *mDevMenu;

    CalculatorOverlay *mCalcOverlay;
    CalculatorWidget *mCalcWidget;

    cemucore::ti_device_t mCalcType;
    CoreWrapper mCore;
};

#endif
