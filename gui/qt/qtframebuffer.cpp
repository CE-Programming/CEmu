#include "utils.h"
#include "qtframebuffer.h"
#include "../../core/backlight.h"
#include "../../core/asic.h"
#include "../../core/emu.h"

#include <QtGui/QPainter>

QImage renderFramebuffer(lcd_state_t *lcds) {
    lcd_drawframe(lcds->framebuffer, lcds);
    return QImage(reinterpret_cast<const uint8_t*>(lcds->framebuffer), lcds->width, lcds->height, QImage::Format_RGBA8888);
}

void paintFramebuffer(QPainter *p, lcd_state_t *lcds) {
    if (guiEmuValid && lcds && lcds->control & 0x800) {
        QImage img = renderFramebuffer(lcds);

        // Interpolation only for < 100% scale
        p->setRenderHint(QPainter::SmoothPixmapTransform, (p->window().width() < static_cast<int>(lcds->width)));
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
