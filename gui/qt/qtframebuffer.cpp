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
#include "../../core/backlight.h"
#include "../../core/lcd.h"
#include "../../core/asic.h"

QImage renderFramebuffer(lcd_state_t *lcds) {
    lcd_drawframe(lcd_framebuffer, lcds);
    return QImage(reinterpret_cast<const uchar*>(lcd_framebuffer), 320, 240, QImage::Format_RGBA8888);
}

void paintFramebuffer(QPainter *p, lcd_state_t *lcds) {
    if (lcds && (lcd.control & 0x800) && !asic.ship_mode_enabled) {
        QImage img = renderFramebuffer(lcds);
        p->drawImage(p->window(), img);
        float factor = (310-(float)backlight.brightness)/160.0;
        if (factor < 1) {
            p->fillRect(p->window(), QColor(0, 0, 0, (1 - factor) * 255));
        }
    } else {
        p->fillRect(p->window(), Qt::black);
        p->setPen(Qt::white);
        p->drawText(p->window(), Qt::AlignCenter, QObject::tr("LCD OFF"));
    }
}
