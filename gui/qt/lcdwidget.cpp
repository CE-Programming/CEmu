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
#include "../../core/lcd.h"
#include "../../core/cpu.h"
#include "../../core/emu.h"
#include "../../core/debug/debug.h"
#include "../../core/backlight.h"
#include "../../core/control.h"

LCDWidget::LCDWidget(QWidget *p) : QWidget(p) {
    installEventFilter(keypadBridge);
    mutex.lock();
    image = QImage(LCD_WIDTH, LCD_HEIGHT, QImage::Format_RGBX8888);
    mutex.unlock();
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
        mutex.lock();
        c.drawImage(cw, image);
        mutex.unlock();
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
    mutex.lock();
    QImage ret(image);
    mutex.unlock();
    return ret;
}

double LCDWidget::refresh() {
    unsigned int msNFramesAgo = array[index];
    array[index] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    double guiFps = (1e3*ARRAY_SIZE) / (array[index] - msNFramesAgo);
    index = (index + 1) % ARRAY_SIZE;
    update();
    return guiFps;
}

void LCDWidget::setMain() {
    mutex.lock();
    image.fill(Qt::black);
    mutex.unlock();
    lcd_gui_callback_data = this;
    lcd_gui_callback = [](void *lcd) { reinterpret_cast<LCDWidget*>(lcd)->draw(); };
}

void LCDWidget::setMode(bool state) {
    spiMode = state;
}

void LCDWidget::setFrameskip(int value) {
    frameskip = value;
    skip = value;
}

// called by the emu thread to draw the lcd
void LCDWidget::draw() {
    if (skip) {
        skip--;
    } else {
        skip = frameskip;
        if (spiMode) {
            mutex.lock();
            memcpy(image.bits(), spi.display, sizeof(spi.display));
            mutex.unlock();
        } else {
            mutex.lock();
            lcd_drawframe(image.bits(), lcd.control & 1 << 11 ? lcd.data : nullptr, lcd.data_end, lcd.control, LCD_SIZE);
            mutex.unlock();
        }
#ifdef PNG_WRITE_APNG_SUPPORTED
        apng_add_frame(image.constBits());
#endif
        double guiFps = 24e6 / (lcd.PCD * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) * (lcd.VSW + lcd.VBP + lcd.LPP + lcd.VFP));
        emit updateLcd(guiFps / (frameskip + 1));
    }
}

