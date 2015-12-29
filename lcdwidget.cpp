#include "lcdwidget.h"
#include <QtGui/QPainter>
#include "qtframebuffer.h"

LCDWidget::LCDWidget(QWidget *p) : QWidget(p) {
    connect(&refresh_timer, SIGNAL(timeout()), this, SLOT(repaint()));

    refresh_timer.setInterval(1000 / 60); // default is 60 fps
    refresh_timer.start();
}

LCDWidget::~LCDWidget(){
}

void LCDWidget::paintEvent(QPaintEvent */*event*/) {
    QPainter painter(this);
    paintFramebuffer(&painter);
}

void LCDWidget::refreshRate(int newrate) {
    refresh_timer.stop();
    refresh_timer.setInterval(1000 / newrate); // change fps to fit cpu load
    refresh_timer.start();
}
