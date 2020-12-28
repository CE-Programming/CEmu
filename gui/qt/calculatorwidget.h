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

#include "keypad/keypadwidget.h"
#include "screenwidget.h"
#include "overlaywidget.h"

#include "../../core/asic.h"

#include <QWidget>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPalette>

class CalculatorOverlay : public OverlayWidget
{
    Q_OBJECT

public:
   CalculatorOverlay(QWidget * parent = nullptr) : OverlayWidget{parent}
   {
       QVBoxLayout *vlayout = new QVBoxLayout(this);
       QVBoxLayout *layout = new QVBoxLayout();
       QPushButton *loadRom = new QPushButton("Load ROM image");
       QPushButton *createRom = new QPushButton("Create ROM image");
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
       p.setColor(QPalette::Window, {255, 255, 255, 243});
       window->setPalette(p);

       vlayout->setSizeConstraint(QLayout::SetMinimumSize);
       vlayout->addWidget(window);
       vlayout->addSpacerItem(spacer);
       vlayout->setAlignment(window, Qt::AlignHCenter);

       connect(loadRom, &QPushButton::clicked, this, &CalculatorOverlay::romLoad);
       connect(createRom, &QPushButton::clicked, this, &CalculatorOverlay::romCreate);
   }

signals:
   void romLoad();
   void romCreate();

protected:
   void paintEvent(QPaintEvent *) override
   {
      QPainter p{this};
      p.fillRect(rect(), {64, 64, 64, 128});
   }
};

class CalculatorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CalculatorWidget(QWidget *parent = nullptr);
    ~CalculatorWidget();

    void setConfig(ti_device_t type, KeypadWidget::Color color);

public slots:
    void changeKeyState(KeyCode keycode, bool press);

signals:
    void keyPressed(const QString& key);

private:
    ScreenWidget *mScreen;
    KeypadWidget *mKeypad;
};

#endif
