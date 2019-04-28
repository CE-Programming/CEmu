#include "lcdwidget.h"
#include "sendinghandler.h"
#include "keypad/qtkeypadbridge.h"
#include "capture/animated-png.h"
#include "utils.h"
#include "../../core/link.h"
#include "../../core/lcd.h"
#include "../../core/cpu.h"
#include "../../core/emu.h"
#include "../../core/debug/debug.h"
#include "../../core/backlight.h"
#include "../../core/control.h"

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QDrag>
#include <QtWidgets/QMenu>
#include <QtWidgets/QApplication>

LCDWidget::LCDWidget(QWidget *parent) : QWidget{parent} {
    installEventFilter(keypadBridge);
    m_mutex.lock();
    m_image = QImage(LCD_WIDTH, LCD_HEIGHT, QImage::Format_RGBX8888);
    m_mutex.unlock();
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
        c.drawImage(cw, m_image);
        m_mutex.unlock();
        if (backlight.factor < 1) {
            c.fillRect(cw, QColor(0, 0, 0, (1 - backlight.factor) * 255));
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
    QPixmap pixmap(size());
    render(&pixmap);
    return pixmap.toImage();
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
        drag->setHotSpot(e->pos());
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
    unsigned int msNFramesAgo = m_array[m_index];
    m_array[m_index] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    double guiFps = (1e3*ArraySize) / (m_array[m_index] - msNFramesAgo);
    m_index = (m_index + 1) % ArraySize;
    update();
    return guiFps;
}

void LCDWidget::setMain() {
    m_mutex.lock();
    m_image.fill(Qt::black);
    m_mutex.unlock();
    emu_set_lcd_callback([](void *lcd) { reinterpret_cast<LCDWidget*>(lcd)->draw(); }, this);
}

void LCDWidget::setMode(bool state) {
    m_spiMode = state;
}

void LCDWidget::setFrameskip(int value) {
    m_frameskip = value;
    m_skip = value;
}

// called by the emu thread to draw the lcd
void LCDWidget::draw() {
    if (m_skip) {
        m_skip--;
    } else {
        m_skip = m_frameskip;
        m_mutex.lock();
        emu_lcd_drawframe(m_image.bits());
        m_mutex.unlock();
#ifdef PNG_WRITE_APNG_SUPPORTED
        apng_add_frame(m_image.constBits());
#endif
        double guiFps = 24e6 / (lcd.PCD * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) * (lcd.VSW + lcd.VBP + lcd.LPP + lcd.VFP));
        emit updateLcd(guiFps / (m_frameskip + 1));
    }
}

