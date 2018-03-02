#include "disasmwidget.h"

#include <QtGui/QWheelEvent>
#include <QtGui/QPainter>
#include <QtCore/QDebug>

DisasmWidget::DisasmWidget(QWidget *parent) : QWidget{parent} {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QSize DisasmWidget::sizeHint() const {
    return {800, 100};
}

void DisasmWidget::wheelEvent(QWheelEvent *event) {
    QPoint delta = event->pixelDelta();
    if (delta.isNull()) {
        delta = event->angleDelta() / 2;
    }
    if (static_cast<int>(m_scroll) >= delta.y()) {
        m_scroll -= delta.y();
        repaint();
    }
    event->accept();
}

void DisasmWidget::paintEvent(QPaintEvent *event) {
    const int lineHeight = fontMetrics().height();
    QPainter painter{this};
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::white);
    unsigned line = m_scroll / lineHeight;
    for (QRect lineRect{0, -static_cast<int>(m_scroll % lineHeight), width(), static_cast<int>(lineHeight)};
         lineRect.y() < height(); lineRect.translate(0, lineHeight), ++line) {
        painter.drawText(lineRect, Qt::TextSingleLine, QString::number(m_baseAddr + line));
    }
}
