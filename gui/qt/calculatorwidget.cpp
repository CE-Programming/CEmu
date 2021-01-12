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

#include "keypad/keypadwidget.h"
#include "screenwidget.h"

#include <kddockwidgets/DockWidget.h>

#include <QtGui/QPainter>
#include <QtGui/QPalette>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

CalculatorOverlay::CalculatorOverlay(QWidget *parent)
    : OverlayWidget{parent}
{
    QPushButton *loadRom = new QPushButton(QIcon(QStringLiteral(":/assets/icons/opened_folder.svg")), tr("Load ROM image"));
    QPushButton *createRom = new QPushButton(QIcon(QStringLiteral(":/assets/icons/circuit.svg")), tr("Create ROM image"));
    QLabel *label = new QLabel();
    QWidget *window = new QWidget();

    label->setText(tr("Welcome!\n\nTo begin using CEmu, you will need a ROM image. "
                      "This is a file that contains the code required to run the calculator's operating system. "
                      "CEmu offers the ability to create a ROM image from a real calculator.\n\n"
                      "Additionally, CEmu uses a customizable dock-style interface. "
                      "Drag and drop to move tabs and windows around on the screen, "
                      "and choose which docks are available in the 'Docks' menu in the topmost bar."));
    label->setWordWrap(true);

    QVBoxLayout *winLayout = new QVBoxLayout;
    winLayout->setSizeConstraint(QLayout::SetMinimumSize);
    winLayout->addWidget(label);
    winLayout->addWidget(createRom);
    winLayout->addWidget(loadRom);

    window->setLayout(winLayout);
    window->setAutoFillBackground(true);
    QPalette p(window->palette());
    QColor windowColor = p.color(QPalette::Window);
    windowColor.setAlpha(243);
    p.setColor(QPalette::Window, windowColor);
    window->setPalette(p);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->setSizeConstraint(QLayout::SetMinimumSize);
    vLayout->addWidget(window);
    vLayout->addStretch();
    vLayout->setAlignment(window, Qt::AlignHCenter);
    setLayout(vLayout);

    connect(loadRom, &QPushButton::clicked, this, &CalculatorOverlay::loadRom);
    connect(createRom, &QPushButton::clicked, this, &CalculatorOverlay::createRom);
}

void CalculatorOverlay::paintEvent(QPaintEvent *)
{
    QPainter p{this};
    p.fillRect(rect(), {64, 64, 64, 128});
}

CalculatorWidget::CalculatorWidget(CoreWindow *coreWindow)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("Calculator")},
                   QIcon(QStringLiteral(":/assets/icons/calculator.svg")),
                   coreWindow}
{
    mScreen = new ScreenWidget{this};
    mKeypad = new KeypadWidget{this};

    mScreen->setMinimumSize(50, 28);
    mKeypad->setMinimumSize(50, 50);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(mScreen);
    layout->addWidget(mKeypad);
    layout->setStretch(0, ScreenWidget::sOuterRect.height() * KeypadWidget::sBaseRect.width());
    layout->setStretch(1, KeypadWidget::sBaseRect.height() * ScreenWidget::sOuterRect.width());
    setLayout(layout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);

    connect(this, &CalculatorWidget::lcdFrame, mScreen, &ScreenWidget::lcdFrame);
    connect(mKeypad, &KeypadWidget::keyPressed, this, &CalculatorWidget::keyPressed);

    mScreen->setScreen(QStringLiteral(":/assets/test/screen.png"));
}

void CalculatorWidget::setDev(cemucore::dev dev)
{
    switch (dev)
    {
        case cemucore::CEMUCORE_DEV_TI84PCE:
        case cemucore::CEMUCORE_DEV_TI84PCEPE:
        case cemucore::CEMUCORE_DEV_TI84PCET:
        case cemucore::CEMUCORE_DEV_TI84PCETPE:
            mKeypad->setType(false);
            break;
        case cemucore::CEMUCORE_DEV_TI83PCE:
        case cemucore::CEMUCORE_DEV_TI83PCEEP:
            mKeypad->setType(true);
            break;
    }
    switch (dev)
    {
        case cemucore::CEMUCORE_DEV_TI84PCE:
            mScreen->setModel(QStringLiteral("TI-84 "), QStringLiteral("Plus CE"));
            break;
        case cemucore::CEMUCORE_DEV_TI84PCEPE:
            mScreen->setModel(QStringLiteral("TI-84 "), QStringLiteral("Plus CE"),
                              QStringLiteral("PYTHON EDITION"));
            break;
        case cemucore::CEMUCORE_DEV_TI83PCE:
            mScreen->setModel(QStringLiteral("TI-83 "), QStringLiteral("Premium CE"));
            break;
        case cemucore::CEMUCORE_DEV_TI83PCEEP:
            mScreen->setModel(QStringLiteral("TI-83 "), QStringLiteral("Premium CE"),
                              QStringLiteral("EDITION PYTHON"));
            break;
        case cemucore::CEMUCORE_DEV_TI84PCET:
            mScreen->setModel(QStringLiteral("TI-84 "), QStringLiteral("Plus CE-T"));
            break;
        case cemucore::CEMUCORE_DEV_TI84PCETPE:
            mScreen->setModel(QStringLiteral("TI-84 "), QStringLiteral("Plus CE-T"),
                              QStringLiteral("PYTHON EDITION"));
            break;
    }
}

void CalculatorWidget::setColor(int color)
{
    mKeypad->setColor(KeypadWidget::Color(color));
}

void CalculatorWidget::changeKeyState(KeyCode code, bool press)
{
    mKeypad->changeKeyState(code, press);
}
