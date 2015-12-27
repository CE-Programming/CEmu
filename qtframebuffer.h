#ifndef QMLFRAMEBUFFER_H
#define QMLFRAMEBUFFER_H

#include <QQuickPaintedItem>

class QMLFramebuffer : public QQuickPaintedItem
{
public:
    QMLFramebuffer(QQuickItem *p = 0);
    virtual void paint(QPainter *p) override;
};

QImage renderFramebuffer();
void paintFramebuffer(QPainter *p);

#endif // QMLFRAMEBUFFER_H
