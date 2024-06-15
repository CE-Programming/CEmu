#include "lcdwidget.h"
#include "sendinghandler.h"
#include "keypad/qtkeypadbridge.h"
#include "capture/animated-png.h"
#include "utils.h"
#include "../../core/link.h"
#include "../../core/lcd.h"
#include "../../core/panel.h"
#include "../../core/cpu.h"
#include "../../core/emu.h"
#include "../../core/debug/debug.h"
#include "../../core/backlight.h"
#include "../../core/control.h"

#include <QtCore/QMetaObject>
#include <QtCore/QMetaMethod>
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QDrag>
#include <QtWidgets/QMenu>
#include <QtWidgets/QApplication>
#include <math.h>

LCDWidget::LCDWidget(QWidget *parent) : QWidget{parent} {
    installEventFilter(keypadBridge);
    m_mutex.lock();
    m_renderedFrame = QImage(LCD_WIDTH, LCD_HEIGHT, QImage::Format_RGBX8888);
    m_blendedFrame = QImage(LCD_WIDTH, LCD_HEIGHT, QImage::Format_RGBX8888);
    m_currFrame = &m_renderedFrame;
    m_mutex.unlock();
    for (int x = 0; x < 320; x++) {
        (x % 2 ? m_interlaceRight : m_interlaceLeft) |= QRegion(x, 0, 1, 240);
    }
}

void LCDWidget::paintEvent(QPaintEvent*) {
    if (!guiEmuValid) {
        return;
    }

    QPainter c(this);
    const QRect &cw = c.window();

    // Interpolation only for < 100% scale
    c.setRenderHint(QPainter::SmoothPixmapTransform, cw.width() < LCD_WIDTH);
    if ((control.ports[5] & 1 << 4) && (lcd.control & 1 << 11)) {
        m_mutex.lock();
        c.drawImage(cw, *m_currFrame);
        m_mutex.unlock();
    } else {
        c.fillRect(cw, Qt::black);
        c.setPen(Qt::white);
        c.drawText(cw, Qt::AlignCenter, tr("LCD OFF"));
    }
    if (m_transferDrag) {
        m_left = cw;
        m_right = m_left;
        m_left.setRight(m_left.right() >> 1);
        m_right.setLeft(m_left.right());
        c.fillRect(m_left, QColor(0, 0, m_side == LcdLeft ? 245 : 200, 128));
        c.fillRect(m_right, QColor(0, m_side == LcdRight ? 245 : 200, 0, 128));
        c.setPen(Qt::white);
        c.drawText(m_left, Qt::AlignCenter, tr("Archive"));
        c.drawText(m_right, Qt::AlignCenter, tr("RAM"));
    }
}

void LCDWidget::dropEvent(QDropEvent *e) {
    if (m_isSendingRom) {
        emit sendROM(m_dragRom);
    } else {
        m_transferDrag = false;
        sendingHandler->dropOccured(e, (e->pos().x() < width() / 2) ? LINK_ARCH : LINK_RAM);
    }
}

void LCDWidget::dragEnterEvent(QDragEnterEvent *e) {
    m_dragRom = sendingROM(e, &m_isSendingRom);

    if (!m_isSendingRom) {
        m_transferDrag = sendingHandler->dragOccured(e);
        m_side = (e->pos().x() < width() / 2) ? LcdLeft : LcdRight;
    }
}

void LCDWidget::dragMoveEvent(QDragMoveEvent *e) {
    m_side = (e->pos().x() < width() / 2) ? LcdLeft : LcdRight;
}

void LCDWidget::dragLeaveEvent(QDragLeaveEvent *e) {
    e->accept();
    m_transferDrag = false;
    m_screenshotDrag = false;
}

QImage LCDWidget::getImage() {
    QImage image;
    if ((control.ports[5] & 1 << 4) && (lcd.control & 1 << 11)) {
        m_mutex.lock();
        image = m_renderedFrame.copy();
        m_mutex.unlock();
    } else {
        image = QImage(LCD_WIDTH, LCD_HEIGHT, QImage::Format_RGBX8888);
        image.fill(Qt::black);
    }
    return image;
}

void LCDWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton && (control.ports[5] & 1 << 4)) {
        m_screenshotDrag = true;
        e->accept();
    } else {
        e->ignore();
    }
}

void LCDWidget::mouseMoveEvent(QMouseEvent *e) {
    if (m_screenshotDrag)
    {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        QImage image = getImage();
        QPixmap mymap = QPixmap::fromImage(image);
        QString path = QDir::tempPath() + QDir::separator() + QStringLiteral("cemu_") + randomString(5) + QStringLiteral(".png");
        image.save(path, "PNG", 0);
        mimeData->setImageData(image);
        mimeData->setUrls(QList<QUrl>() << QUrl::fromLocalFile(path));
        drag->setMimeData(mimeData);
        drag->setHotSpot(e->pos() * ((double)image.rect().width() / rect().width()));
        drag->setPixmap(mymap);
        switch (drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::MoveAction)) {
            case Qt::IgnoreAction:
            case Qt::CopyAction:
                QFile::remove(path);
                break;
            default:
                break;
        }
        m_screenshotDrag = false;
        e->accept();
    } else {
        e->ignore();
    }
}

double LCDWidget::refresh() {
    std::chrono::steady_clock::time_point timeNFramesAgo = m_perfArray[m_perfIndex];
    m_perfArray[m_perfIndex] = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = m_perfArray[m_perfIndex] - timeNFramesAgo;
    double guiFrameTime = diff.count() / PerfArraySize;
    if (++m_perfIndex == PerfArraySize) {
        m_perfIndex = 0;
    }
    update();
    return guiFrameTime;
}

void LCDWidget::setMain() {
    m_mutex.lock();
    m_renderedFrame.fill(Qt::black);
    m_blendedFrame.fill(Qt::black);
    m_currFrame = &m_renderedFrame;
    m_mutex.unlock();
    emu_set_lcd_callback([](void *lcd) { return static_cast<LCDWidget*>(lcd)->draw(); }, this);
}

void LCDWidget::setResponseMode(bool state) {
    m_responseMode = state;
}

void LCDWidget::setFrameskip(int value) {
    m_frameskip = value;
    m_skip = value;
}

void LCDWidget::disableBlend() {
    if (m_currFrame == &m_blendedFrame) {
        m_mutex.lock();
        m_currFrame = &m_renderedFrame;
        m_mutex.unlock();
        static QMetaMethod updateMethod = staticMetaObject.method(staticMetaObject.indexOfSlot("update()"));
        updateMethod.invoke(this);
    }
}

// called by the emu thread to draw the lcd
bool LCDWidget::draw() {
    if (m_skip) {
        m_skip--;
        disableBlend();
    } else {
        m_skip = m_frameskip;
        m_mutex.lock();
        m_blendedFrame.swap(m_renderedFrame);
        emu_lcd_drawframe(m_renderedFrame.bits());
        if (!lcd.useDma && backlight.factor < 1.0f) {
            QPainter c(&m_renderedFrame);
            c.fillRect(c.window(), QColor(0, 0, 0, (1.0f - backlight.factor) * 255.0f));
        }
        if (m_responseMode) {
            QPainter c(&m_blendedFrame);
            if (lcd.useDma && panel.params.GATECTRL.SM) {
                c.setClipRegion(m_interlaceRight);
                c.setOpacity(0.4);
                c.drawImage(QPoint(0, 0), m_renderedFrame);
                c.setClipRegion(m_interlaceLeft);
                c.setOpacity(0.6);
            } else {
                c.setOpacity(0.6);
            }
            c.drawImage(QPoint(0, 0), m_renderedFrame);
            m_currFrame = &m_blendedFrame;
        } else {
            m_currFrame = &m_renderedFrame;
        }
        m_mutex.unlock();
#ifdef PNG_WRITE_APNG_SUPPORTED
        apng_add_frame(m_currFrame->constBits());
#endif
        double guiFps = 24e6 / (lcd.PCD * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) * (lcd.VSW + lcd.VBP + lcd.LPP + lcd.VFP));
        if (lcd.useDma && panel.displayMode != PANEL_DM_RGB && panel.lineStartTick != 0) {
            double internalFps = sched_get_clock_rate_precise(CLOCK_PANEL) / panel.lineStartTick;
            if (panel.displayMode != PANEL_DM_VSYNC) {
                guiFps = internalFps;
            } else if (unlikely(internalFps < guiFps)) {
                guiFps /= ceil(guiFps / internalFps);
            }
        }
        emit updateLcd(guiFps / (m_frameskip + 1));
    }
    // return whether next frame should be rendered
    return m_skip == 0;
}

