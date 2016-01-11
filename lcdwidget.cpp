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
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(&refresh_timer, SIGNAL(timeout()), this, SLOT(repaint()));
    connect(this, &QWidget::customContextMenuRequested, this, &LCDWidget::drawContext);

    // Default rate is 60 FPS
    refreshRate(60);
}

void LCDWidget::drawContext(const QPoint& posa) {
    QString open = "Open...";
    QPoint globalPos = this->mapToGlobal(posa);

    QMenu contextMenu;
    contextMenu.addAction(open);

    QAction* selectedItem = contextMenu.exec(globalPos);
    if (selectedItem) {
        if (selectedItem->text() == open) {
            emit lcdOpenRequested();
            show();
        }
    }
}

LCDWidget::~LCDWidget(){}

void LCDWidget::paintEvent(QPaintEvent */*event*/) {
    QPainter painter(this);
    paintFramebuffer(&painter);
}

void LCDWidget::refreshRate(int newrate) {
    // Change fps to fit cpu load
    refresh_timer.stop();
    refresh_timer.setInterval(1000 / newrate);
    refresh_timer.start();
}
