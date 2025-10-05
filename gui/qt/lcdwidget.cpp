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
#include "../../core/backlight.h"
#include "../../core/control.h"

#include <QtCore/QtMath>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaMethod>
#include <QtCore/QTimer>
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QDrag>
#include <QtWidgets/QMenu>
#include <QtWidgets/QApplication>
#include <cmath>

LCDWidget::LCDWidget(QWidget *parent) : QWidget{parent} {
    installEventFilter(keypadBridge);
    m_mutex.lock();
    m_renderedFrame = QImage(LCD_WIDTH, LCD_HEIGHT, QImage::Format_RGB32);
    m_blendedFrame = QImage(LCD_WIDTH, LCD_HEIGHT, QImage::Format_RGB32);
    m_currFrame = &m_renderedFrame;
    m_mutex.unlock();
    m_interlaceAlpha = QImage(LCD_WIDTH, LCD_HEIGHT, QImage::Format_Alpha8);
    uint8_t* interlacePixels = reinterpret_cast<uint8_t*>(m_interlaceAlpha.bits());
    for (size_t index = 0; index < LCD_WIDTH * LCD_HEIGHT; index += 2) {
        interlacePixels[index] = 255 * 4 / 10;
        interlacePixels[index + 1] = 255 * 6 / 10;
    }
}

void LCDWidget::paintEvent(QPaintEvent*) {
    if (!guiEmuValid) {
        return;
    }

    QPainter c(this);
    const QRect &cw = c.window();

    if ((control.ports[5] & 1 << 4) && (lcd.control & 1 << 11)) {
        QSizeF targetSize = cw.size();
        if (isFullScreen() && m_fullscreenArea != StretchToFill) {
            targetSize = QSizeF(LCD_WIDTH, LCD_HEIGHT).scaled(cw.size(), Qt::KeepAspectRatio);
        }
        QSizeF pixelSize = targetSize * c.device()->devicePixelRatioF();
        qreal widthScale = pixelSize.width() / LCD_WIDTH;
        qreal heightScale = pixelSize.height() / LCD_HEIGHT;
        bool forceBilinear = m_upscale == Bilinear;
        if (isFullScreen() && m_fullscreenArea == IntegerUpscale) {
            /* widthScale and heightScale should be equal since we kept the aspect ratio */
            targetSize /= widthScale;
            widthScale = heightScale = qFloor(widthScale);
            targetSize *= widthScale;
            forceBilinear = false;
        }
        QRectF target = QRectF(QPointF(), targetSize);
        target.moveCenter(QRectF(cw).center());
        QSize integerSize = QSize(qMax(qRound(widthScale), 1) * LCD_WIDTH, qMax(qRound(heightScale), 1) * LCD_HEIGHT);
        bool sharpUsesBilinear = m_upscale == SharpBilinear && integerSize != pixelSize;
        c.setRenderHint(QPainter::SmoothPixmapTransform, forceBilinear || sharpUsesBilinear);
        if (sharpUsesBilinear && integerSize != QSize(LCD_WIDTH, LCD_HEIGHT)) {
            m_mutex.lock();
            QImage integerFrame = m_currFrame->scaled(integerSize);
            m_mutex.unlock();
            c.drawImage(target, integerFrame);
        } else {
            m_mutex.lock();
            c.drawImage(target, *m_currFrame);
            m_mutex.unlock();
        }
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
        sendingHandler->dropOccured(e, (e->position().x() < width() / 2.) ? LINK_ARCH : LINK_RAM);
    }
}

void LCDWidget::dragEnterEvent(QDragEnterEvent *e) {
    m_dragRom = sendingROM(e, &m_isSendingRom);

    if (!m_isSendingRom) {
        m_transferDrag = sendingHandler->dragOccured(e);
        m_side = (e->position().x() < width() / 2.) ? LcdLeft : LcdRight;
    }
}

void LCDWidget::dragMoveEvent(QDragMoveEvent *e) {
    m_side = (e->position().x() < width() / 2.) ? LcdLeft : LcdRight;
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
        image = QImage(LCD_WIDTH, LCD_HEIGHT, QImage::Format_RGB32);
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
        QImage image = getImage();
        QString path = QDir::tempPath() + QDir::separator() + QStringLiteral("CEmu_screenshot_") + randomString(5) + QStringLiteral(".png");
        if (image.save(path, "PNG")) {
            QDrag *drag = new QDrag(this);
            QMimeData *mimeData = new QMimeData;
            QPixmap mymap = QPixmap::fromImage(image);
            mimeData->setImageData(image);
            mimeData->setUrls(QList<QUrl>() << QUrl::fromLocalFile(path));
            drag->setMimeData(mimeData);
            drag->setHotSpot(e->pos() * ((double)image.rect().width() / rect().width()));
            drag->setPixmap(mymap);
            switch (drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::MoveAction)) {
                case Qt::IgnoreAction:
                case Qt::CopyAction:
                    QTimer::singleShot(1000, this, [path] { QFile::remove(path); });
                    break;
                default:
                    break;
            }
            e->accept();
        } else {
            e->ignore();
        }
        m_screenshotDrag = false;
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

void LCDWidget::setUpscaleMethod(int value) {
    m_upscale = static_cast<UpscaleMethod>(value);
}

void LCDWidget::setFullscreenArea(int value) {
    m_fullscreenArea = static_cast<FullscreenArea>(value);
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
            QPainter c;
            if (lcd.useDma && panel.params.GATECTRL.SM) {
                /* hack to get around format of QImage paint engine being cached forever */
                QImage blendedFrame(m_blendedFrame.bits(), LCD_WIDTH, LCD_HEIGHT, QImage::Format_ARGB32_Premultiplied);
                c.begin(&blendedFrame);
                c.setCompositionMode(QPainter::CompositionMode_DestinationIn);
                c.drawImage(QPoint(0, 0), m_interlaceAlpha);
                c.setCompositionMode(QPainter::CompositionMode_DestinationOver);
                c.drawImage(QPoint(0, 0), m_renderedFrame);
                c.end();
            } else {
                c.begin(&m_blendedFrame);
                c.setOpacity(0.6);
                c.drawImage(QPoint(0, 0), m_renderedFrame);
                c.end();
            }
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

