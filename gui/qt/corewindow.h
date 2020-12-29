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

class QtKeypadBridge;

#include "../../core/asic.h"

#include <kddockwidgets/DockWidget.h>
#include <kddockwidgets/MainWindow.h>

QT_BEGIN_NAMESPACE
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

signals:
    void romChanged();

private slots:
    void createRom();
    void loadRom();
    void resetEmu();
    void showPreferences();
    void saveLayout();
    void restoreLayout();

private:
    void createFileMenu();
    void createDocksMenu();
    void createDebugMenu();
    void createExtrasMenu();

    void createDockWidgets();
    void createDeveloperWidgets();
    void setKeymap();

    KDDockWidgets::DockWidget::List mDockWidgets;

    QtKeypadBridge *mKeypadBridge;

    QMenu *mCalcsMenu;
    QMenu *mDocksMenu;
    QMenu *mDevMenu;
    QMenu *mCaptureMenu;

    CalculatorOverlay *mCalcOverlay;
    CalculatorWidget *mCalc;

    ti_device_t mCalcType;

    static const QString sErrorStr;
    static const QString sWarningStr;
    static const QString sInformationStr;
};

#endif
