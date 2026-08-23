#include "visualizerdisplaywidget.h"
#include "keypad/qtkeypadbridge.h"
#include "utils.h"
#include "../../core/lcd.h"

#include <cmath>
#include <utility>
#include <QtGui/QPainter>
#include <QtWidgets/QMenu>
#include <QtWidgets/QApplication>
#include <QAction> /* Different module in Qt5 vs Qt6 */

namespace {

bool transformSwapsDimensions(VisualizerTransform transform) {
    return transform == VisualizerTransform::Rotate90
        || transform == VisualizerTransform::Rotate270
        || transform == VisualizerTransform::Transpose
        || transform == VisualizerTransform::Transverse;
}

QSize transformedSize(int width, int height, const QVector<VisualizerTransform> &transforms) {
    for (VisualizerTransform transform : transforms) {
        if (transformSwapsDimensions(transform)) {
            std::swap(width, height);
        }
    }
    return QSize(width, height);
}

QPoint mapSourceToDisplay(QPoint point, int width, int height,
                          const QVector<VisualizerTransform> &transforms) {
    for (VisualizerTransform transform : transforms) {
        const int x = point.x();
        const int y = point.y();
        switch (transform) {
            case VisualizerTransform::Rotate90:
                point = QPoint(height - 1 - y, x);
                break;
            case VisualizerTransform::Rotate180:
                point = QPoint(width - 1 - x, height - 1 - y);
                break;
            case VisualizerTransform::Rotate270:
                point = QPoint(y, width - 1 - x);
                break;
            case VisualizerTransform::FlipHorizontal:
                point.setX(width - 1 - x);
                break;
            case VisualizerTransform::FlipVertical:
                point.setY(height - 1 - y);
                break;
            case VisualizerTransform::Transpose:
                point = QPoint(y, x);
                break;
            case VisualizerTransform::Transverse:
                point = QPoint(height - 1 - y, width - 1 - x);
                break;
        }
        if (transformSwapsDimensions(transform)) {
            std::swap(width, height);
        }
    }
    return point;
}

QPoint mapDisplayToSource(QPoint point, int width, int height,
                          const QVector<VisualizerTransform> &transforms) {
    QVector<QSize> sizes;
    sizes.reserve(transforms.size() + 1);
    sizes.append(QSize(width, height));
    for (VisualizerTransform transform : transforms) {
        if (transformSwapsDimensions(transform)) {
            std::swap(width, height);
        }
        sizes.append(QSize(width, height));
    }

    for (int index = static_cast<int>(transforms.size()); index-- > 0;) {
        width = sizes.at(index).width();
        height = sizes.at(index).height();
        const int x = point.x();
        const int y = point.y();
        switch (transforms.at(index)) {
            case VisualizerTransform::Rotate90:
                point = QPoint(y, height - 1 - x);
                break;
            case VisualizerTransform::Rotate180:
                point = QPoint(width - 1 - x, height - 1 - y);
                break;
            case VisualizerTransform::Rotate270:
                point = QPoint(width - 1 - y, x);
                break;
            case VisualizerTransform::FlipHorizontal:
                point.setX(width - 1 - x);
                break;
            case VisualizerTransform::FlipVertical:
                point.setY(height - 1 - y);
                break;
            case VisualizerTransform::Transpose:
                point = QPoint(y, x);
                break;
            case VisualizerTransform::Transverse:
                point = QPoint(width - 1 - y, height - 1 - x);
                break;
        }
    }
    return point;
}

}

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
    updateDisplayImage();
    update();
}

void VisualizerDisplayWidget::updateDisplayImage() {
    if (m_transforms.isEmpty()) {
        return;
    }

    const QSize displaySize = transformedSize(m_width, m_height, m_transforms);
    if (m_displayImage.size() != displaySize) {
        m_displayImage = QImage(displaySize, QImage::Format_RGB32);
    }

    for (int sourceY = 0; sourceY < m_height; sourceY++) {
        const QRgb *source = reinterpret_cast<const QRgb *>(m_image->constScanLine(sourceY));
        for (int sourceX = 0; sourceX < m_width; sourceX++) {
            const QPoint display = mapSourceToDisplay(QPoint(sourceX, sourceY), m_width, m_height,
                                                       m_transforms);
            reinterpret_cast<QRgb *>(m_displayImage.scanLine(display.y()))[display.x()] = source[sourceX];
        }
    }
}

void VisualizerDisplayWidget::paintEvent(QPaintEvent*) {
    if (!guiEmuValid) {
        return;
    }

    QPainter c(this);
    const QRect& cw = c.window();

    const QImage &image = m_transforms.isEmpty() ? *m_image : m_displayImage;
    c.setRenderHint(QPainter::SmoothPixmapTransform, cw.width() < image.width());
    if (m_image != Q_NULLPTR) {
        c.drawImage(cw, image);

        // only draw grid if width/height scale >= 200%
        if (m_grid && (cw.width() >= (image.width() * 2) && cw.height() >= (image.height() * 2))) {
            QVarLengthArray<QLineF, 100> lines;

            for (qreal x = cw.left(); x < cw.right(); x += (cw.width() / image.width())) {
                lines.append(QLineF(x, cw.top(), x, cw.bottom()));
            }
            for (qreal y = cw.top(); y < cw.bottom(); y += (cw.height() / image.height())) {
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
        QImage image = m_transforms.isEmpty() ? *m_image : m_displayImage;
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

void VisualizerDisplayWidget::setConfig(uint32_t bppstep, int w, int h, uint32_t u, uint32_t c, bool g,
                                        const QVector<VisualizerTransform> &transforms,
                                        uint32_t *d, uint32_t *e) {
    m_bppstep = bppstep;
    m_width = w;
    m_height = h;
    m_upbase = u;
    m_control = c;
    m_data = d;
    m_data_end = e;
    m_size = w * h;
    m_grid = g;
    m_transforms = transforms;
    delete m_image;
    m_image = new QImage(w, h, QImage::Format_RGB32);
    m_image->fill(Qt::black);
    m_displayImage = QImage();
    updateDisplayImage();
}

void VisualizerDisplayWidget::contextMenu(const QPoint& posa) const {
    QString copyStr = tr("Copy Address");
    QString coordStr = tr("Coordinate: ");

    const QSize displaySize = transformedSize(m_width, m_height, m_transforms);
    const int displayWidth = displaySize.width();
    const int displayHeight = displaySize.height();
    const int displayX = posa.x() * displayWidth / width();
    const int displayY = posa.y() * displayHeight / height();
    const QPoint source = mapDisplayToSource(QPoint(displayX, displayY), m_width, m_height,
                                              m_transforms);
    const int x = source.x();
    const int y = source.y();

    uint32_t offset = (static_cast<unsigned int>(m_width) * static_cast<unsigned int>(y)
                     + static_cast<unsigned int>(x)) * m_bppstep / 8;
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
