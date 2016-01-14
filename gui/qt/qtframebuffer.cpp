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
#include "../../core/lcd.h"
#include "../../core/backlight.h"
#include "../../core/lcd.h"

#define CLAMP(a) ( ((a) > 255) ? 255 : (((a) < 0) ? 0 : (int)(a)) )

QImage brighten(QImage &img, float factor) {
    int r,g,b;
    /* Scale each RGB value by the brightening factor */
    for(int y = 0; y < img.height(); y++) {
        for(int x = 0; x < img.width(); x++) {
            QRgb p = img.pixel(x, y);
            r = (int)(factor * qRed(p));
            r = CLAMP(r);
            g = (int)(factor * qGreen(p));
            g = CLAMP(g);
            b = (int)(factor * qBlue(p));
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

    QImage image(reinterpret_cast<const uchar*>(lcd.framebuffer), 320, 240, 320 * 2, format);

    float factor = (310-(float)backlight.brightness)/160.0;
    factor = (factor > 1) ? 1 : factor;
    return brighten(image, factor);
}

void paintFramebuffer(QPainter *p) {
    if (lcd.control & 0x800) {
        QImage img = renderFramebuffer();
        p->drawImage(p->window(), img);
    } else {
        p->fillRect(p->window(), Qt::black);
        p->setPen(Qt::white);
        p->drawText(p->window(), Qt::AlignCenter, QObject::tr("LCD OFF"));
    }
}

void QMLFramebuffer::paint(QPainter *p) {
    paintFramebuffer(p);
}
