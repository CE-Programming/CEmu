#include <QtGui/QPainter>
#include <QtWidgets/QApplication>

#include "utils.h"
#include "memoryvisualizerwidget.h"
#include "keypad/qtkeypadbridge.h"
#include "../../core/lcd.h"

MemoryVisualizerWidget::MemoryVisualizerWidget(QWidget *p) : QWidget(p) {
    refreshTimer = new QTimer(this);
    installEventFilter(keypadBridge);
    image = QImage(LCD_WIDTH, LCD_HEIGHT, QImage::Format_RGBX8888);
}

MemoryVisualizerWidget::~MemoryVisualizerWidget() {
    delete refreshTimer;
}

void MemoryVisualizerWidget::draw() {
    if (!guiEmuValid) {
        return;
    }

    lcd_setptrs(&data, &data_end, width, height, upbase, control, false);
    lcd_drawframe(image.bits(), data, data_end, control, size);
    update();
}

void MemoryVisualizerWidget::paintEvent(QPaintEvent*) {
    if (!guiEmuValid) {
        return;
    }

    QPainter c(this);
    const QRect& cw = c.window();

    c.setRenderHint(QPainter::SmoothPixmapTransform, cw.width() < static_cast<int>(width));
    c.drawImage(cw, image);
}

void MemoryVisualizerWidget::setRefreshRate(int rate) {
    if (!rate) {
        return;
    }
    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(draw()));
    refreshTimer->stop();
    refreshTimer->setInterval(1000 / rate);
    refreshTimer->start();
    refresh = rate;
}

void MemoryVisualizerWidget::setConfig(uint32_t h, uint32_t w, uint32_t u, uint32_t c, uint32_t *d, uint32_t *e) {
    height = h;
    width = w;
    upbase = u;
    control = c;
    data = d;
    data_end = e;
    size = width * height;
    image = QImage(width, height, QImage::Format_RGBX8888);
}
