#include "qtframebuffer.h"
#include "core/asic.h"

QImage renderFramebuffer()
{
    uint16_t *framebuffer = reinterpret_cast<uint16_t*>(malloc(320 * 240 * 2));

    uint32_t bitfields[] = { 0x01F, 0x000, 0x000};

    lcd_drawframe(framebuffer, bitfields);
    QImage::Format format = bitfields[0] == 0x00F ? QImage::Format_RGB444 : QImage::Format_RGB16;

    QImage image(reinterpret_cast<const uchar*>(framebuffer), 320, 240, 320 * 2, format, free, framebuffer);

    return image;
}

void paintFramebuffer(QPainter *p)
{
    if(lcd.memory == NULL)
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
