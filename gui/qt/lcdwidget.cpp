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

LCDWidget::LCDWidget(QWidget *p) : QWidget(p) {
    refreshTimer = new QTimer(this);
    setContextMenuPolicy(Qt::CustomContextMenu);
    installEventFilter(keypadBridge);
    setAcceptDrops(true);
    image = QImage(LCD_WIDTH, LCD_HEIGHT, QImage::Format_RGBX8888);
}

LCDWidget::~LCDWidget() {
    delete refreshTimer;
}

void LCDWidget::draw() {
    if (!guiEmuValid || !lcdState) {
        return;
    }

    lcd_drawframe(image.bits(), lcd_setptrs(lcdState));
    update();
}

void LCDWidget::paintEvent(QPaintEvent*) {
    if (!guiEmuValid || !lcdState) {
        return;
    }

    QPainter c(this);
    const QRect& cw = c.window();

    // Interpolation only for < 100% scale
    c.setRenderHint(QPainter::SmoothPixmapTransform, cw.width() < static_cast<int>(lcdState->width));
    c.drawImage(cw, image);

    if (main) {
        if (backlight.factor < 1) {
            c.fillRect(cw, QColor(0, 0, 0, (1 - backlight.factor) * 255));
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
}

void LCDWidget::callback(void) {
    if (!guiEmuValid) {
        return;
    }

    //lcd_drawframe(&lcd);

    if (!skip--) {
        skip = frameskip;
        memcpy(image.bits(), spi.display, sizeof(spi.display));
    }

#ifdef PNG_WRITE_APNG_SUPPORTED
    apng_add_frame();
#endif

    if (lcd.off) {
        QPainter p(&image);
        p.fillRect(p.window(), Qt::black);
        p.setPen(Qt::white);
        p.drawText(p.window(), Qt::AlignCenter, tr("LCD OFF"));
        fps = 0;
    } else {
        fps = 24e6 / (lcd.PCD * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) * (lcd.VSW + lcd.VBP + lcd.LPP + lcd.VFP) * (frameskip + 1));
    }
    update();
}

int LCDWidget::getFPS() {
    return fps;
}

void LCDWidget::setRefreshRate(int rate) {
    if (!rate || lcdState == &lcd) {
        return;
    }
    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(draw()));
    refreshTimer->stop();
    refreshTimer->setInterval(1000 / rate);
    refreshTimer->start();
    refresh = rate;
}

void LCDWidget::setFrameskip(int value) {
    frameskip = value;
    skip = value;
}

void LCDWidget::setLCD(lcd_state_t *x) {
    lcdState = x;
    if (lcdState == &lcd) {
        disconnect(refreshTimer, SIGNAL(timeout()), this, SLOT(draw()));
        refreshTimer->stop();
        main = true;
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

    // check if we are dragging a rom file
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
