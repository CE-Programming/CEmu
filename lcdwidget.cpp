#include <QtGui/QPainter>

#include "lcdwidget.h"
#include "qtframebuffer.h"

LCDWidget::LCDWidget(QWidget *p) : QWidget(p) {
    connect(&refresh_timer, SIGNAL(timeout()), this, SLOT(repaint()));

    // Default rate is 60 FPS
    refreshRate(60);
}

LCDWidget::~LCDWidget(){}

void LCDWidget::paintEvent(QPaintEvent */*event*/) {
    QPainter painter(this);
    paintFramebuffer(&painter);
}

void LCDWidget::refreshRate(int newrate) {
    // Change fps to fit cpu load
    refresh_timer.stop();
    refresh_timer.setInterval(1000 / newrate);
    refresh_timer.start();
}
