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

#include "screenwidget.h"

#include <kddockwidgets/DockWidget.h>

#include <QtGui/QPalette>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

CalculatorOverlay::CalculatorOverlay(QWidget *parent)
    : OverlayWidget{parent}
{
    QVBoxLayout *vlayout = new QVBoxLayout(this);
    QVBoxLayout *layout = new QVBoxLayout();
    QPushButton *loadRom = new QPushButton(QIcon(QStringLiteral(":/assets/icons/opened_folder.svg")), tr("Load ROM image"));
    QPushButton *createRom = new QPushButton(QIcon(QStringLiteral(":/assets/icons/circuit.svg")), tr("Create ROM image"));
    QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding);
    QLabel *label = new QLabel();
    QWidget *window = new QWidget();

    label->setText(tr("Welcome!\n\nTo begin using CEmu, you will need a ROM image. "
                      "This is a file that contains the code required to run the calculator's operating system. "
                      "CEmu offers the ability to create a ROM image from a real calculator.\n\n"
                      "Additionally, CEmu uses a customizable dock-style interface. "
                      "Drag and drop to move tabs and windows around on the screen, "
                      "and choose which docks are available in the 'Docks' menu in the topmost bar."));
    label->setWordWrap(true);

    layout->setSizeConstraint(QLayout::SetMinimumSize);
    layout->addWidget(label);
    layout->addWidget(createRom);
    layout->addWidget(loadRom);

    window->setLayout(layout);
    window->setAutoFillBackground(true);
    QPalette p(window->palette());
    QColor windowColor = p.color(QPalette::Window);
    windowColor.setAlpha(243);
    p.setColor(QPalette::Window, windowColor);
    window->setPalette(p);

    vlayout->setSizeConstraint(QLayout::SetMinimumSize);
    vlayout->addWidget(window);
    vlayout->addSpacerItem(spacer);
    vlayout->setAlignment(window, Qt::AlignHCenter);

    connect(loadRom, &QPushButton::clicked, this, &CalculatorOverlay::loadRom);
    connect(createRom, &QPushButton::clicked, this, &CalculatorOverlay::createRom);
}

void CalculatorOverlay::paintEvent(QPaintEvent *)
{
    QPainter p{this};
    p.fillRect(rect(), {64, 64, 64, 128});
}

CalculatorWidget::CalculatorWidget(DockedWidgetList &list)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("Calculator")},
                   QIcon(QStringLiteral(":/assets/icons/calculator.svg")),
                   list}
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

    layout->setStretch(0, ScreenWidget::sOuterRect.height() * KeypadWidget::sBaseRect.width());
    layout->setStretch(1, KeypadWidget::sBaseRect.height() * ScreenWidget::sOuterRect.width());

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);

    connect(mKeypad, &KeypadWidget::keyPressed, this, &CalculatorWidget::keyPressed);
}

CalculatorWidget::~CalculatorWidget()
{
}

void CalculatorWidget::setConfig(ti_device_t type, int color)
{
    KeypadWidget::Color keycolor = static_cast<KeypadWidget::Color>(color);

    switch (type)
    {
        default:
            mKeypad->setType(false, keycolor);
            mScreen->setModel(QStringLiteral("TI-84 "), QStringLiteral("Plus CE")/*, QStringLiteral("PYTHON EDITION")*/);
            break;

        case ti_device_t::TI83PCE:
            mKeypad->setType(true, keycolor);
            mScreen->setModel(QStringLiteral("TI-83 "), QStringLiteral("Premium CE")/*, QStringLiteral("EDITION PYTHON")*/);
            break;
    }
    mScreen->setScreen(QStringLiteral(":/assets/test/screen.png"));
}

void CalculatorWidget::changeKeyState(KeyCode code, bool press)
{
    mKeypad->changeKeyState(code, press);
}
