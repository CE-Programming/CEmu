#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QDrag>
#include <QtWidgets/QMenu>
#include <QtWidgets/QApplication>

#include "utils.h"
#include "lcdwidget.h"
#include "sendinghandler.h"
#include "keypad/qtkeypadbridge.h"
#include "capture/animated-png.h"
#include "../../core/link.h"
#include "../../core/spi.h"
#include "../../core/debug/debug.h"
#include "../../core/backlight.h"
#include "../../core/control.h"

LCDWidget::LCDWidget(QWidget *p) : QWidget(p) {
    installEventFilter(keypadBridge);
    image = QImage(LCD_WIDTH, LCD_HEIGHT, QImage::Format_RGBX8888);
}

void LCDWidget::paintEvent(QPaintEvent*) {
    if (!guiEmuValid) {
        return;
    }

    QPainter c(this);
    const QRect& cw = c.window();

    // Interpolation only for < 100% scale
    c.setRenderHint(QPainter::SmoothPixmapTransform, cw.width() < LCD_WIDTH);
    if (control.ports[5] & 1 << 4) {
        c.drawImage(cw, image);
        if (backlight.factor < 1) {
            c.fillRect(cw, QColor(0, 0, 0, (1 - backlight.factor) * 255));
        }
    } else {
        c.fillRect(cw, Qt::black);
        c.setPen(Qt::white);
        c.drawText(cw, Qt::AlignCenter, tr("LCD OFF"));
    }
    if (drag) {
        left = cw;
        right = left;
        left.setRight(left.right() >> 1);
        right.setLeft(left.right());
        c.fillRect(left, QColor(0, 0, sideDrag == LCD_LEFT ? 245 : 200, 128));
        c.fillRect(right, QColor(0, sideDrag == LCD_RIGHT ? 245 : 200, 0, 128));
        c.setPen(Qt::white);
        c.drawText(left, Qt::AlignCenter, tr("Archive"));
        c.drawText(right, Qt::AlignCenter, tr("RAM"));
    }
}

void LCDWidget::setMode(bool state) {
    mode = state;
}

void LCDWidget::callback(void) {
    if (!guiEmuValid) {
        return;
    }

    if (!skip--) {
        skip = frameskip;
        if (mode) {
            memcpy(image.bits(), spi.display, sizeof(spi.display));
        } else {
            lcd_drawframe(image.bits(), lcd.control & 1 << 11 ? lcd.data : nullptr, lcd.data_end, lcd.control, LCD_SIZE);
        }
    }

#ifdef PNG_WRITE_APNG_SUPPORTED
    apng_add_frame();
#endif

    fps = 24e6 / (lcd.PCD * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) * (lcd.VSW + lcd.VBP + lcd.LPP + lcd.VFP) * (frameskip + 1));
    update();
}

double LCDWidget::getFPS() {
    return fps / (frameskip + 1);
}

void LCDWidget::setFrameskip(int value) {
    frameskip = value;
    skip = value;
}

void LCDWidget::dropEvent(QDropEvent *e) {
    if (isSendingROM) {
        emit sendROM(dragROM);
    } else {
        drag = false;
        sendingHandler->dropOccured(e, (e->pos().x() < width() / 2) ? LINK_ARCH : LINK_RAM);
    }
}

void LCDWidget::dragEnterEvent(QDragEnterEvent *e) {
    dragROM = sendingROM(e, &isSendingROM);

    if (!isSendingROM) {
        drag = sendingHandler->dragOccured(e);
        sideDrag = (e->pos().x() < width() / 2) ? LCD_LEFT : LCD_RIGHT;
    }
}

void LCDWidget::dragMoveEvent(QDragMoveEvent *e) {
    sideDrag = (e->pos().x() < width() / 2) ? LCD_LEFT : LCD_RIGHT;
}

void LCDWidget::dragLeaveEvent(QDragLeaveEvent *e) {
    e->accept();
    drag = false;
}

QImage LCDWidget::getImage() {
    return image;
}
