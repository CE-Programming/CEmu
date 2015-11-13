#include "lcdwidget.h"
#include "qtframebuffer.h"

LCDWidget::LCDWidget(QWidget *p) : QWidget(p)
{
  connect(&refresh_timer, SIGNAL(timeout()), this, SLOT(update()));

  refresh_timer.setInterval(1000 / 60); // 60 fps
  refresh_timer.start();
}

LCDWidget::~LCDWidget()
{

}

void LCDWidget::paintEvent(QPaintEvent */*event*/)
{
    QPainter painter(this);
    paintFramebuffer(&painter);
}
