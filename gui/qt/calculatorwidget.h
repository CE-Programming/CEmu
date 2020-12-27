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

#include <QWidget>
#include <QResizeEvent>

#include "keypad/keypadwidget.h"
#include "screenwidget.h"

#include "../../core/asic.h"

class CalculatorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CalculatorWidget(QWidget *parent = nullptr);
    ~CalculatorWidget();

    void setConfig(ti_device_t type, KeypadWidget::KeypadColor color);

public slots:
    void changeKeyState(KeyCode keycode, bool press);

signals:
    void keyPressed(const QString& key);

private:
    ScreenWidget *mScreen;
    KeypadWidget *mKeypad;
};

#endif
