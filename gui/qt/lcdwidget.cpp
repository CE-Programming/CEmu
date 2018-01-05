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
#include "../../core/debug/debug.h"
#include "../../core/backlight.h"

LCDWidget::LCDWidget(QWidget *p) : QWidget(p) {
    refreshTimer = new QTimer(this);
    setContextMenuPolicy(Qt::CustomContextMenu);
    installEventFilter(keypadBridge);
    setAcceptDrops(true);
    image = QImage(LCD_WIDTH, LCD_HEIGHT, QImage::Format_RGB888);
}

LCDWidget::~LCDWidget() {
    delete refreshTimer;
}

void LCDWidget::draw() {
    if (!guiEmuValid || !lcdState) {
        return;
    }

    lcd_drawframe(lcd_setptrs(lcdState));
    image = QImage(reinterpret_cast<const uint8_t*>(lcdState->frame),
                   lcdState->width, lcdState->height, QImage::Format_RGB888);
    update();
}

void LCDWidget::paintEvent(QPaintEvent*) {
    if (!guiEmuValid || !lcdState) {
        return;
    }

    QPainter c(this);
    const QRect &cw = c.window();

    if (lcdState->control & 0x800) {
        // Interpolation only for < 100% scale
        c.setRenderHint(QPainter::SmoothPixmapTransform, cw.width() < static_cast<int>(lcdState->width));
        c.drawImage(cw, image);

        float factor = (310 - (float)backlight.brightness) / 160.0;
        if (factor < 1) {
            c.fillRect(cw, QColor(0, 0, 0, (1 - factor) * 255));
        }
    } else {
        c.fillRect(cw, Qt::black);
        c.setPen(Qt::white);
        c.drawText(cw, Qt::AlignCenter, QObject::tr("LCD OFF"));
    }
    if (inDrag) {
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

void LCDWidget::callback(void) {
    if (!guiEmuValid) {
        return;
    }

    lcd_drawframe(&lcd);

    if (!skip--) {
        skip = frameskip;
        memcpy(image.bits(), lcd.frame, LCD_FRAME_SIZE);
    }

#ifdef PNG_WRITE_APNG_SUPPORTED
    apng_add_frame();
#endif

    fps = 24e6 / (lcd.PCD * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) * (lcd.VSW + lcd.VBP + lcd.LPP + lcd.VFP) * (frameskip + 1));
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
    }
}

void LCDWidget::dropEvent(QDropEvent *e) {
    if (isSendingROM) {
        emit sendROM(dragROM);
    } else {
        inDrag = false;
        sendingHandler->dropOccured(e, (e->pos().x() < width() / 2) ? LINK_ARCH : LINK_RAM);
    }
}

void LCDWidget::dragEnterEvent(QDragEnterEvent *e) {

    // check if we are dragging a rom file
    dragROM = sendingROM(e, &isSendingROM);

    if (!isSendingROM) {
        inDrag = sendingHandler->dragOccured(e);
        sideDrag = (e->pos().x() < width() / 2) ? LCD_LEFT : LCD_RIGHT;
    }
}

void LCDWidget::dragMoveEvent(QDragMoveEvent *e) {
    sideDrag = (e->pos().x() < width() / 2) ? LCD_LEFT : LCD_RIGHT;
}

void LCDWidget::dragLeaveEvent(QDragLeaveEvent *e) {
    e->accept();
    inDrag = false;
}
