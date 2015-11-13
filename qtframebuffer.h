#ifndef QTFRAMEBUFFER_H
#define QTFRAMEBUFFER_H

#include <QImage>
#include <QPainter>

#include <core/asic.h>

QImage renderFramebuffer();
void paintFramebuffer(QPainter *p);

#endif // QTFRAMEBUFFER
