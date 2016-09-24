#include "arrowkey.h"

#include <cmath>

qreal ArrowKey::computeAngle(const QRect &bounds, int gap) {
    return 45 - 180 * M_1_PI * std::asin(2. * gap / bounds.width());
}
QPointF ArrowKey::rotatePoint(QPointF point, int offset) {
    while (offset--) {
        std::swap(point.rx() *= -1, point.ry());
    }
    return point;
}

ArrowKey::ArrowKey(KeyConfig &config, const QRect &outer, const QRect &inner,
                   int offset, qreal gap)
    : Key{config.next(), inner, outer, config.graphColor}, m_labelColor{config.blackColor},
      m_outer{outer}, m_inner{inner}, m_offset{2 * offset} {
    qreal innerAngle = computeAngle(inner, gap),
          outerAngle = computeAngle(outer, gap);
    int offsetAngle = 90 * offset;
    m_keyShape.arcMoveTo(inner, offsetAngle + innerAngle);
    m_keyShape.arcTo(outer, offsetAngle + outerAngle, -2 * outerAngle);
    m_keyShape.arcTo(inner, offsetAngle - innerAngle,  2 * innerAngle);
    m_arrow.moveTo(rotatePoint({2 * gap, 0}, offset));
    m_arrow.lineTo(rotatePoint({gap,  gap}, offset));
    m_arrow.lineTo(rotatePoint({gap, -gap}, offset));
    m_arrow.translate(QRectF{outer}.center() + rotatePoint({inner.width() * .5, 0}, offset) + QPointF{.5, .5});
}

void ArrowKey::paint(QPainter &painter) const {
    Key::paint(painter);
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_labelColor);
    painter.drawPath(m_arrow);
}

bool ArrowKey::canAccept(const QPointF &point) {
    QPointF norm{point - m_outer.center()};
    qreal outerRadius = m_outer.width() * .7,
        radiusSquared = QPointF::dotProduct(norm, norm);
    return radiusSquared <= outerRadius * outerRadius &&
        (static_cast<int>((2 * M_2_PI * std::atan2(norm.y(), norm.x()) + 9.5) + m_offset) & 7) < 3;
}
