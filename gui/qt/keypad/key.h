#ifndef KEY_H
#define KEY_H

#include "keycode.h"

#include <QtCore/QRect>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

class Key {
public:
    Key(KeyCode keycode, const QRect &textGeometry, const QRect &keyGeometry, const QColor &keyColor) :
        mTextGeometry{textGeometry}, mKeyGeometry{keyGeometry}, mKeyColor{keyColor},
    mKeycode{keycode}, mPressed{}, mHeld{}, mAccepted{} {}
    virtual ~Key() {}

    const QString getLabel() const { return mLabelText; }
    const QRect &textGeometry() const { return mTextGeometry; }
    const QRect &keyGeometry() const { return mKeyGeometry; }
    const KeyCode keycode() const { return mKeycode; }
    bool isPressed() const { return mPressed; }
    bool isHeld() const { return mHeld; }
    bool isSelected() const { return isPressed() | isHeld(); }

    void setPressed(bool pressed) { mPressed = pressed; }
    void toggleHeld() { mHeld = !mHeld; }

    virtual void paint(QPainter &painter) const {
    painter.setBrush(mKeyColor);
	painter.setPen({QColor::fromRgb(0x808080), .375});
    painter.drawPath(mKeyShape);
	if (isSelected()) {
	    painter.setBrush(QColor::fromRgba(0x80800000));
        painter.drawPath(mKeyShape);
	}
    }
    bool accept(const QPointF &point) {
        bool accepted = canAccept(point);
        mAccepted |= accepted;
        return accepted;
    }
    bool unaccept() {
    bool accepted = mAccepted;
    mAccepted &= false;
	return accepted;
    }

protected:
    virtual bool canAccept(const QPointF &) = 0;
    QString mLabelText;
    QPainterPath mKeyShape;

private:
    QRect mTextGeometry, mKeyGeometry;
    QColor mKeyColor;
    const KeyCode mKeycode;
    quint8 mPressed  : 1;
    quint8 mHeld     : 1;
    quint8 mAccepted : 1;
};

#endif
