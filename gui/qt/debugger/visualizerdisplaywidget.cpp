#include "visualizerdisplaywidget.h"
#include "keypad/qtkeypadbridge.h"
#include "utils.h"
#include "../../core/lcd.h"

#include <QtGui/QPainter>
#include <QtWidgets/QMenu>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>

VisualizerDisplayWidget::VisualizerDisplayWidget(QWidget *parent) : QWidget{parent} {
    m_refreshTimer = new QTimer(this);
    installEventFilter(keypadBridge);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &VisualizerDisplayWidget::customContextMenuRequested, this, &VisualizerDisplayWidget::contextMenu);

    m_image = QImage(LCD_WIDTH, LCD_HEIGHT, QImage::Format_RGBX8888);
}

VisualizerDisplayWidget::~VisualizerDisplayWidget() {
    delete m_refreshTimer;
}

void VisualizerDisplayWidget::draw() {
    if (!guiEmuValid) {
        return;
    }

    lcd_setptrs(&m_data, &m_data_end, m_width, m_height, m_upbase, m_control, false);
    lcd_drawframe(m_image.bits(), m_data, m_data_end, m_control, m_size);
    update();
}

void VisualizerDisplayWidget::paintEvent(QPaintEvent*) {
    if (!guiEmuValid) {
        return;
    }

    QPainter c(this);
    const QRect& cw = c.window();

    c.setRenderHint(QPainter::SmoothPixmapTransform, cw.width() < static_cast<int>(m_width));
    c.drawImage(cw, m_image);
}

void VisualizerDisplayWidget::setRefreshRate(int rate) {
    if (!rate) {
        return;
    }
    connect(m_refreshTimer, SIGNAL(timeout()), this, SLOT(draw()));
    m_refreshTimer->stop();
    m_refreshTimer->setInterval(1000 / rate);
    m_refreshTimer->start();
    m_refresh = rate;
}

void VisualizerDisplayWidget::setConfig(uint32_t h, uint32_t w, uint32_t u, uint32_t c, uint32_t *d, uint32_t *e) {
    m_height = h;
    m_width = w;
    m_upbase = u;
    m_control = c;
    m_data = d;
    m_data_end = e;
    m_size = m_width * m_height;
    m_image = QImage(m_width, m_height, QImage::Format_RGBX8888);
}

void VisualizerDisplayWidget::contextMenu(const QPoint& posa) {
    QString coord = tr("Coordinate: ");
    QString copy_addr = tr("Copy Address");

    QTransform tr;
    tr.scale(m_width * 1.0 / width(), m_height * 1.0 / height());
    QPoint point = tr.map(posa);
    uint32_t x = point.x();
    uint32_t y = point.y();

    QString addr = int2hex(m_upbase + (x + (m_width * y)), 6);

    coord += QString::number(x) + QStringLiteral("x") + QString::number(y);
    copy_addr += QStringLiteral(" '") + addr + QStringLiteral("'");

    QMenu menu;
    menu.addAction(coord);
    menu.addSeparator();
    menu.addAction(copy_addr);

    QAction *item = menu.exec(mapToGlobal(posa));
    if (item) {
        if (item->text() == copy_addr) {
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(addr.toLatin1());
        }
    }
}
