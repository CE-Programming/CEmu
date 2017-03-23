#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QDrag>
#include <QtWidgets/QMenu>
#include <QtWidgets/QApplication>

#include "utils.h"
#include "lcdwidget.h"
#include "sendinghandler.h"
#include "keypad/qtkeypadbridge.h"
#include "../../core/link.h"
#include "../../core/debug/debug.h"

LCDWidget::LCDWidget(QWidget *p) : QWidget(p) {
    refreshTimer = new QTimer(this);
    setContextMenuPolicy(Qt::CustomContextMenu);

    installEventFilter(keypadBridge);

    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(repaint()));

    setAcceptDrops(true);

    // Default rate is 30 FPS
    refreshRate(30);
}

LCDWidget::~LCDWidget() {
    delete refreshTimer;
}

void LCDWidget::paintEvent(QPaintEvent*) {
    QPainter canvas(this);
    paintFramebuffer(&canvas, lcdState);
    if (in_drag) {
        left = canvas.window();
        right = left;
        left.setRight(left.right() >> 1);
        right.setLeft(left.right());
        canvas.fillRect(left, QColor(0, 0, side_drag == LCD_LEFT ? 245 : 200, 128));
        canvas.fillRect(right, QColor(0, side_drag == LCD_RIGHT ? 245 : 200, 0, 128));
        canvas.setPen(Qt::white);
        canvas.drawText(left, Qt::AlignCenter, QObject::tr("Archive"));
        canvas.drawText(right, Qt::AlignCenter, QObject::tr("RAM"));
    }
}

void LCDWidget::refreshRate(int newrate) {
    refreshTimer->stop();
    refreshTimer->setInterval(1000 / newrate);
    refreshTimer->start();
}

void LCDWidget::setLCD(lcd_state_t *x) {
    lcdState = x;
}

void LCDWidget::dropEvent(QDropEvent *e) {
    if (isSendingROM) {
        emit sendROM(dragROM);
    } else {
        in_send = true;
        in_drag = false;
        sendingHandler->dropOccured(e, (e->pos().x() < width() / 2) ? LINK_ARCH : LINK_RAM);
        in_send = false;
    }
}

void LCDWidget::dragEnterEvent(QDragEnterEvent *e) {

    // check if we are dragging a rom file
    dragROM = sendingROM(e, &isSendingROM);

    if (!isSendingROM) {
        in_drag = sendingHandler->dragOccured(e);
        side_drag = (e->pos().x() < width() / 2) ? LCD_LEFT : LCD_RIGHT;
    }
}

void LCDWidget::dragMoveEvent(QDragMoveEvent *e) {
    side_drag = (e->pos().x() < width() / 2) ? LCD_LEFT : LCD_RIGHT;
}

void LCDWidget::dragLeaveEvent(QDragLeaveEvent *e) {
    e->accept();
    in_drag = false;
}
