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
#include <QDrag>
#include <QMimeData>
#include <QClipboard>
#include <QApplication>

#include "lcdwidget.h"
#include "qtframebuffer.h"
#include "../../core/lcd.h"

LCDWidget::LCDWidget(QWidget *p) : QWidget(p) {
    lcdState = &lcd;
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(&refreshTimer, SIGNAL(timeout()), this, SLOT(repaint()));

    setAcceptDrops(true);
    // Default rate is 60 FPS
    refreshRate(60);
}

LCDWidget::~LCDWidget(){}

void LCDWidget::paintEvent(QPaintEvent */*event*/) {
    QPainter painter(this);
    paintFramebuffer(&painter, lcdState);
}

void LCDWidget::refreshRate(int newrate) {
    refreshTimer.stop();
    refreshTimer.setInterval(1000 / newrate);
    refreshTimer.start();
}

void LCDWidget::setLCD(lcd_state_t *lcdS) {
    lcdState = lcdS;
}

void LCDWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        QImage image = renderFramebuffer(lcdState);
        QPixmap mymap = QPixmap::fromImage(image);

        mimeData->setImageData(image);
        drag->setMimeData(mimeData);
        drag->setHotSpot(e->pos());
        drag->setPixmap(mymap);

        drag->exec(Qt::CopyAction | Qt::MoveAction);
        e->accept();
    } else {
        e->ignore();
    }
}
