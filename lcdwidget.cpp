#include "lcdwidget.h"
#include <QtGui/QPainter>
#include "qtframebuffer.h"

LCDWidget::LCDWidget(QWidget *p) : QWidget(p) {
    connect(&refresh_timer, SIGNAL(timeout()), this, SLOT(repaint()));

    refresh_timer.setInterval(1000 / 30); // 30 fps
    refresh_timer.start();
}

LCDWidget::~LCDWidget(){
}

void LCDWidget::paintEvent(QPaintEvent */*event*/) {
    QPainter painter(this);
    paintFramebuffer(&painter);
}
