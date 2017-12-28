#ifndef QTFRAMEBUFFER_H
#define QTFRAMEBUFFER_H

#include "../../core/lcd.h"

#include <QtWidgets/QWidget>

class QtFramebuffer : public QWidget {
public:
    explicit QtFramebuffer(QWidget *parent = Q_NULLPTR);
    virtual void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
};

QImage renderFramebuffer(lcd_state_t *lcds);
void paintFramebuffer(QPainter *p, lcd_state_t *lcds);

#endif
