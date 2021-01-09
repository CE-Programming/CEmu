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

#ifndef CALCULATORWIDGET_H
#define CALCULATORWIDGET_H

#include "corewrapper.h"
#include "dockedwidget.h"
#include "keypad/keypadwidget.h"
#include "overlaywidget.h"
class CoreWindow;
class ScreenWidget;

#include <QtWidgets/QWidget>

class CalculatorOverlay : public OverlayWidget
{
    Q_OBJECT

public:
    CalculatorOverlay(QWidget *parent = nullptr);

signals:
    void loadRom();
    void createRom();

protected:
    void paintEvent(QPaintEvent *) override;
};

class CalculatorWidget : public DockedWidget
{
    Q_OBJECT

public:
    explicit CalculatorWidget(CoreWindow *coreWindow);

    void setConfig(cemucore::ti_device_t type, int color);

public slots:
    void changeKeyState(KeyCode keycode, bool press);

signals:
    void lcdFrame();
    void keyPressed(const QString& key);

private:
    ScreenWidget *mScreen;
    KeypadWidget *mKeypad;
};

#endif
