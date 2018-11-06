#include "arrowkey.h"

#include <QtGui/QVector2D>

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
                   int offset, const QString &labelText, qreal gap)
    : Key{config.next(), inner, outer, config.graphColor}, mLabelColor{config.blackColor},
      mOuter{outer}, mInner{inner}, mOffset{2 * offset} {
    qreal innerAngle = computeAngle(inner, gap),
          outerAngle = computeAngle(outer, gap);
    int offsetAngle = 90 * offset;
    mLabelText = labelText;
    mKeyShape.arcMoveTo(inner, offsetAngle + innerAngle);
    mKeyShape.arcTo(outer, offsetAngle + outerAngle, -2 * outerAngle);
    mKeyShape.arcTo(inner, offsetAngle - innerAngle,  2 * innerAngle);
    mArrow.moveTo(rotatePoint({2 * gap, 0}, offset));
    mArrow.lineTo(rotatePoint({gap,  gap}, offset));
    mArrow.lineTo(rotatePoint({gap, -gap}, offset));
    mArrow.translate(QRectF{outer}.center() + rotatePoint({inner.width() * .5, 0}, offset) + QPointF{.5, .5});
}

void ArrowKey::paint(QPainter &painter) const {
    Key::paint(painter);
    painter.setPen(Qt::NoPen);
    painter.setBrush(mLabelColor);
    painter.drawPath(mArrow);
}

bool ArrowKey::isUnder(const QPainterPath &area) const {
    QPainterPath key;
    key.addEllipse(mOuter);
    if (!key.intersects(area) || !area.elementCount()) {
        return false;
    }
    QPointF center{area.isEmpty() ? area.elementAt(0) : area.controlPointRect().center()};
    QVector2D offset{center - mOuter.center()};
    return (static_cast<int>((2*M_2_PI * std::atan2(offset.y(), offset.x()) + 9.5) + mOffset) & 7) < 3;
}
