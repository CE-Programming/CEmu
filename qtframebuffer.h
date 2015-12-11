#ifndef QTFRAMEBUFFER_H
#define QTFRAMEBUFFER_H

#include <QImage>

class QPainter;

#include <core/asic.h>

QImage renderFramebuffer();
void paintFramebuffer(QPainter *p);

#endif // QTFRAMEBUFFER
