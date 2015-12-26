#include "qtframebuffer.h"
#include <QtGui/QPainter>
#include "core/asic.h"

#include <iostream>

#define CLAMP(a) ( ((a) > 255) ? 255 : (((a) < 0) ? 0 : (int)(a)) )

QImage brighten(QImage &img, float factor)
{
    /* Scale each RGB value by the brightening factor */
    for(int y = 0; y < img.height(); y++) {
        for(int x = 0; x < img.width(); x++) {
            QRgb p = img.pixel(x, y);
            int r = qRed(p) * factor;
            r = CLAMP(r);
            int g = qGreen(p) * factor;
            g = CLAMP(g);
            int b = qBlue(p) * factor;
            b = CLAMP(b);
            img.setPixel(x, y, qRgb(r, g, b));
          }
    }

    return img;
}

QImage renderFramebuffer()
{
    uint16_t *framebuffer = reinterpret_cast<uint16_t*>(malloc(320 * 240 * 2));

    uint32_t bitfields[] = { 0x01F, 0x000, 0x000};

    lcd_drawframe(framebuffer, bitfields);
    QImage::Format format = bitfields[0] == 0x00F ? QImage::Format_RGB444 : QImage::Format_RGB16;

    QImage image(reinterpret_cast<const uchar*>(framebuffer), 320, 240, 320 * 2, format, free, framebuffer);

    return brighten(image,(300-(float)backlight.brightness)/160.0);
}

void paintFramebuffer(QPainter *p)
{
    if(!(lcd.control & 0x800))
    {
        p->fillRect(p->window(), Qt::black);
        p->setPen(Qt::white);
        p->drawText(p->window(), Qt::AlignCenter, QObject::tr("LCD OFF"));
    }
    else
    {
        QImage img = renderFramebuffer();
        p->drawImage(p->window(), img);
    }
}
