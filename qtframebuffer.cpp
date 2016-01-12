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

#include "qtframebuffer.h"
#include "qtkeypadbridge.h"
#include "core/lcd.h"

#define CLAMP(a) ( ((a) > 255) ? 255 : (((a) < 0) ? 0 : (int)(a)) )

QImage brighten(QImage &img, float factor) {
    int r,g,b;
    /* Scale each RGB value by the brightening factor */
    for(int y = 0; y < img.height(); y++) {
        for(int x = 0; x < img.width(); x++) {
            QRgb p = img.pixel(x, y);
            r = qRed(p) * factor;
            r = CLAMP(r);
            g = qGreen(p) * factor;
            g = CLAMP(g);
            b = qBlue(p) * factor;
            b = CLAMP(b);
            img.setPixel(x, y, qRgb(r, g, b));
          }
    }

    return img;
}

static uint32_t bitfields[3] = { 0x01F, 0x000, 0x000};

QImage renderFramebuffer() {
    lcd_drawframe(lcd.framebuffer, bitfields);

    QImage::Format format = *bitfields == 0x00F ? QImage::Format_RGB444 : QImage::Format_RGB16;

    return QImage(reinterpret_cast<const uchar*>(lcd.framebuffer), 320, 240, 320 * 2, format);
}

void paintFramebuffer(QPainter *p) {
    if (!(lcd.control & 0x800)) {
        p->fillRect(p->window(), Qt::black);
        p->setPen(Qt::white);
        p->drawText(p->window(), Qt::AlignCenter, QObject::tr("LCD OFF"));
    } else {
        QImage img = renderFramebuffer();
        p->drawImage(p->window(), img);
    }
}

void QMLFramebuffer::paint(QPainter *p) {
    paintFramebuffer(p);
}
