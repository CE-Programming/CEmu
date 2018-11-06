#ifndef KEY_H
#define KEY_H

#include "keycode.h"

#include <QtCore/QRect>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

#include <assert.h>

class Key {
public:
    Key(KeyCode keycode, const QRect &textGeometry, const QRect &keyGeometry, const QColor &keyColor) :
        mTextGeometry{textGeometry}, mKeyGeometry{keyGeometry}, mKeyColor{keyColor},
        mSelected{}, mKeycode{keycode}, mHeld{}, mPressed{} {}
    virtual ~Key() {}

    const QString getLabel() const { return mLabelText; }
    const QRect &textGeometry() const { return mTextGeometry; }
    const QRect &keyGeometry() const { return mKeyGeometry; }
    const KeyCode keycode() const { return mKeycode; }
    bool isPressed() const { return mPressed; }
    bool isHeld() const { return mHeld; }
    bool isSelected() const { return mSelected; }

    void setPressed(bool pressed) {
        if (mPressed != pressed) {
            pressOrRelease(mPressed = pressed);
        }
    }
    void toggleHeld() {
        pressOrRelease(mHeld = !mHeld);
    }

    virtual void paint(QPainter &painter) const {
        painter.setBrush(mKeyColor);
        painter.setPen({QColor::fromRgb(0x808080), .375});
        painter.drawPath(mKeyShape);
        if (isSelected()) {
            painter.setBrush(QColor::fromRgba(0x80800000));
            painter.drawPath(mKeyShape);
        }
    }
    virtual bool isUnder(const QPainterPath &area) const = 0;
    void pressOrRelease(bool isPress) {
        if (isPress) {
            press();
        } else {
            release();
        }
    }
    void press() {
        assert(static_cast<decltype(mSelected)>(mSelected + 1));
        ++mSelected;
    }
    void release() {
        assert(mSelected);
        --mSelected;
    }

protected:
    QString mLabelText;
    QPainterPath mKeyShape;

private:
    QRect mTextGeometry, mKeyGeometry;
    QColor mKeyColor;
    unsigned mSelected;
    const KeyCode mKeycode;
    bool mHeld;
    bool mPressed;
};

#endif
