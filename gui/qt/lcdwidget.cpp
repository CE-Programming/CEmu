/* Copyright (C) 2015  Fabian Vogt
 * Modified for the CE calculator by CEmu developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include <QtGui/QPainter>
#include <QMenu>

#include "lcdwidget.h"
#include "qtframebuffer.h"

LCDWidget::LCDWidget(QWidget *p) : QWidget(p) {
    connect(&refresh_timer, SIGNAL(timeout()), this, SLOT(repaint()));
    connect(this, &QWidget::customContextMenuRequested, this, &LCDWidget::drawContext);

    setFixedSize(320, 240);
    lcd_size = 1;

    // Default rate is 60 FPS
    refreshRate(60);
}

void LCDWidget::drawContext(const QPoint& posa) {
    QString open  = "Open...",
            small = "Small",
            med   = "Medium",
            large = "Large",
            onTop = "Always on Top";

    QPoint globalPos = this->mapToGlobal(posa);

    QMenu contextMenu;
    contextMenu.addAction(open);                // 0
    contextMenu.addAction(small);               // 1
    contextMenu.addAction(med);                 // 2
    contextMenu.addAction(large);               // 3
    contextMenu.addAction(onTop);               // 4
    contextMenu.actions().at(1)->setCheckable(true);
    contextMenu.actions().at(2)->setCheckable(true);
    contextMenu.actions().at(3)->setCheckable(true);
    contextMenu.actions().at(4)->setCheckable(true);

    contextMenu.actions().at(4)->setChecked(state_set);
    contextMenu.actions().at(lcd_size)->setChecked(true);

    QAction* selectedItem = contextMenu.exec(globalPos);
    if (selectedItem) {
        if (selectedItem->text() == open) {
            emit lcdOpenRequested();
            show();
        } else if (selectedItem->text() == onTop){
            state_set = !state_set;
            setWindowFlags(state_set ? windowFlags() | Qt::WindowStaysOnTopHint : windowFlags() & ~Qt::WindowStaysOnTopHint);
            show();
        } else if (selectedItem->text() == small) {
            lcd_size = 1;
            setFixedSize(320*1, 240*1);
        } else if (selectedItem->text() == med) {
            lcd_size = 2;
            setFixedSize(320*2, 240*2);
        } else if (selectedItem->text() == large) {
            lcd_size = 3;
            setFixedSize(320*3, 240*3);
        }
    }
}

LCDWidget::~LCDWidget(){}

void LCDWidget::paintEvent(QPaintEvent */*event*/) {
    QPainter painter(this);
    paintFramebuffer(&painter);
}

void LCDWidget::refreshRate(int newrate) {
    refresh_timer.stop();
    refresh_timer.setInterval(1000 / newrate);
    refresh_timer.start();
}

void LCDWidget::showEvent(QShowEvent *e) {
    QWidget::showEvent(e);
}

void LCDWidget::hideEvent(QHideEvent *e) {
    QWidget::hideEvent(e);
}

void LCDWidget::closeEvent(QCloseEvent *e) {
    QWidget::closeEvent(e);
    emit closed();
}
