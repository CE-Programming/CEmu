#include "screenwidget.h"

#include <QPainter>
#include <QSizePolicy>

ScreenWidget::ScreenWidget(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

ScreenWidget::~ScreenWidget()
{
}

void ScreenWidget::setSkin(const QString &skin)
{
    m_skin = QImage(skin);
}

void ScreenWidget::paintEvent(QPaintEvent */*event*/)
{
    QPainter painter{this};
    const static QRect screen{61, 78, 320, 240};

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setTransform(m_transform);

    painter.drawImage(m_skin.rect(), m_skin, m_skin.rect());

    painter.fillRect(screen, Qt::black);
    painter.setPen(Qt::white);
    painter.setFont(QFont("arial", 18));
    painter.drawText(screen, Qt::AlignCenter, tr("LCD OFF"));
}

void ScreenWidget::resizeEvent(QResizeEvent *event)
{
    QSize size{m_skin.size().scaled(event->size(), Qt::KeepAspectRatio)},
        origin{(event->size() - size) / 2};

    qreal m11 = static_cast<qreal>(size.width()) / m_skin.width();
    qreal m22 = static_cast<qreal>(size.height()) / m_skin.height();
    qreal dx = origin.width();
    qreal dy = origin.height() * 2;

    m_transform.setMatrix(m11,  0, 0,
                          0,  m22, 0,
                          dx,  dy, 1);
}
