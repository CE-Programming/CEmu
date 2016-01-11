#ifndef ARROWKEY_H
#define ARROWKEY_H

#include "key.h"
#include "keyconfig.h"

class ArrowKey : public Key {
public:
    ArrowKey(KeyConfig &config, const QRect &outer, const QRect &inner,
	     int offset, qreal gap = 2);

    virtual void paint(QPainter &) const;

protected:
    virtual bool canAccept(const QPointF &);

private:
    static qreal computeAngle(const QRect &, int);
    static QPointF rotatePoint(QPointF, int);

    QColor m_labelColor;
    QPainterPath m_arrow;
    QRect m_outer, m_inner;
    int m_offset;
};

#endif
