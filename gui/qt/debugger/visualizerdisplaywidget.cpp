#include "visualizerdisplaywidget.h"
#include "keypad/qtkeypadbridge.h"
#include "utils.h"
#include "../../core/lcd.h"

#include <cmath>
#include <QtGui/QPainter>
#include <QtWidgets/QMenu>
#include <QtWidgets/QApplication>
#include <QAction> /* Different module in Qt5 vs Qt6 */

VisualizerDisplayWidget::VisualizerDisplayWidget(QWidget *parent) : QWidget{parent} {
    m_refreshTimer = new QTimer(this);
    installEventFilter(keypadBridge);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &VisualizerDisplayWidget::customContextMenuRequested, this, &VisualizerDisplayWidget::contextMenu);
    connect(m_refreshTimer, &QTimer::timeout, this, &VisualizerDisplayWidget::draw);

    m_image = new QImage(LCD_WIDTH, LCD_HEIGHT, QImage::Format_RGB32);
}

VisualizerDisplayWidget::~VisualizerDisplayWidget() {
    delete m_image;
    delete m_refreshTimer;
}

void VisualizerDisplayWidget::draw() {
    if (!guiEmuValid) {
        return;
    }

    emu_set_lcd_ptrs(&m_data, &m_data_end, m_width, m_height, m_upbase, m_control, false);
    emu_lcd_drawmem(m_image->bits(), m_data, m_data_end, m_control, m_size);
    update();
}

void VisualizerDisplayWidget::paintEvent(QPaintEvent*) {
    if (!guiEmuValid) {
        return;
    }

    QPainter c(this);
    const QRect& cw = c.window();

    c.setRenderHint(QPainter::SmoothPixmapTransform, cw.width() < m_width);
    if (m_image != Q_NULLPTR) {
        c.drawImage(cw, *m_image);

        // only draw grid if width/height scale >= 200%
        if (m_grid && (cw.width() >= (m_width * 2) && cw.height() >= (m_height * 2))) {
            QVarLengthArray<QLineF, 100> lines;

            for (qreal x = cw.left(); x < cw.right(); x += (cw.width() / m_width)) {
                lines.append(QLineF(x, cw.top(), x, cw.bottom()));
            }
            for (qreal y = cw.top(); y < cw.bottom(); y += (cw.height() / m_height)) {
                lines.append(QLineF(cw.left(), y, cw.right(), y));
            }

            c.drawLines(lines.data(), lines.size());
        }
    }
}

void VisualizerDisplayWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        QImage image = *m_image;
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
        e->accept();
    } else {
        e->ignore();
    }
}

void VisualizerDisplayWidget::setRefreshRate(int rate) {
    if (!rate) {
        return;
    }
    m_refreshTimer->stop();
    m_refreshTimer->setInterval(1000 / rate);
    m_refreshTimer->setTimerType(Qt::PreciseTimer);
    m_refreshTimer->start();
    m_refresh = rate;
}

void VisualizerDisplayWidget::setConfig(uint32_t bppstep, int w, int h, uint32_t u, uint32_t c, bool g, uint32_t *d, uint32_t *e) {
    m_bppstep = bppstep;
    m_width = w;
    m_height = h;
    m_upbase = u;
    m_control = c;
    m_data = d;
    m_data_end = e;
    m_size = w * h;
    m_grid = g;
    delete m_image;
    m_image = new QImage(w, h, QImage::Format_RGB32);
}

void VisualizerDisplayWidget::contextMenu(const QPoint& posa) const {
    QString copyStr = tr("Copy Address");
    QString coordStr = tr("Coordinate: ");

    QTransform tr;
    tr.scale(m_width * 1.0 / width(), m_height * 1.0 / height());
    QPoint point = tr.map(posa);
    uint32_t x = static_cast<uint32_t>(point.x());
    uint32_t y = static_cast<uint32_t>(point.y());

    uint32_t offset = (static_cast<unsigned int>(m_width) * y + x) * m_bppstep / 8;
    if (m_control & 0x200) {
        // reverse order within 32-bit word for BEBO mode
        offset ^= (-m_bppstep / 8) & 3;
    }
    QString addr = int2hex(m_upbase + offset, 6);

    coordStr += QString::number(x) + QStringLiteral("x") + QString::number(y);
    copyStr += QStringLiteral(" '") + addr + QStringLiteral("'");

    QMenu menu;
    QAction *copy = menu.addAction(copyStr);
    menu.addSeparator();
    QAction *coord = menu.addAction(coordStr);
    coord->setEnabled(false);

    QAction *item = menu.exec(mapToGlobal(posa));
    if (item == copy) {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(addr.toLatin1());
    }
}
