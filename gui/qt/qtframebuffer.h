#ifndef QMLFRAMEBUFFER_H
#define QMLFRAMEBUFFER_H

#include <QtQuick/QQuickPaintedItem>
#include "../../core/lcd.h"

class QMLFramebuffer : public QQuickPaintedItem {
public:
    QMLFramebuffer(QQuickItem *p = 0);
    virtual void paint(QPainter *p) override;
};

QImage renderFramebuffer(lcd_state_t *lcds);
void paintFramebuffer(QPainter *p, lcd_state_t *lcds);

#endif
