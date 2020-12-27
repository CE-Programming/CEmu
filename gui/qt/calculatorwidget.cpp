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

#include <QVBoxLayout>

CalculatorWidget::CalculatorWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    mKeypad = new KeypadWidget(this);
    mScreen = new ScreenWidget(this);

    mKeypad->setMinimumSize(50, 50);
    mScreen->setMinimumSize(50, 28);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(mScreen);
    layout->addWidget(mKeypad);

    layout->setStretch(0, 35);
    layout->setStretch(1, 65);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);

    connect(mKeypad, &KeypadWidget::keyPressed, this, [=](const QString &key)
    {
        emit keyPressed(key);
    });
}

CalculatorWidget::~CalculatorWidget()
{
}

void CalculatorWidget::setConfig(ti_device_t type, KeypadWidget::KeypadColor color)
{
    switch (type)
    {
        default:
            mKeypad->setType(false, color);
            mScreen->setModel(QStringLiteral("TI-84 "), QStringLiteral("Plus CE"));
            break;

        case ti_device_t::TI83PCE:
            mKeypad->setType(true, color);
            mScreen->setModel(QStringLiteral("TI-83 "), QStringLiteral("Premium CE"));
            break;
    }
}

void CalculatorWidget::changeKeyState(KeyCode code, bool press)
{
    mKeypad->changeKeyState(code, press);
}
