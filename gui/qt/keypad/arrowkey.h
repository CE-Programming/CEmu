#ifndef ARROWKEY_H
#define ARROWKEY_H

/* Enable math constants on MSVC */
#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif

#include "key.h"
#include "keyconfig.h"

class ArrowKey : public Key {
public:
    ArrowKey(KeyConfig &config, const QRect &outer, const QRect &inner,
             int offset, const QString &labelText, qreal gap = 2);

    virtual void paint(QPainter &) const;

protected:
    virtual bool canAccept(const QPointF &) const;

private:
    static qreal computeAngle(const QRect &, int);
    static QPointF rotatePoint(QPointF, int);

    QColor mLabelColor;
    QPainterPath mArrow;
    QRect mOuter, mInner;
    int mOffset;
};

#endif
