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
        mKeycode{keycode}, mAcceptedTouch{}, mPressed{}, mHeld{}, mAccepted{} {}
    virtual ~Key() {}

    const QString getLabel() const { return mLabelText; }
    const QRect &textGeometry() const { return mTextGeometry; }
    const QRect &keyGeometry() const { return mKeyGeometry; }
    const KeyCode keycode() const { return mKeycode; }
    bool isPressed() const { return mPressed; }
    bool isHeld() const { return mHeld; }
    bool isSelected() const { return isPressed() || isHeld() || mAccepted || mAcceptedTouch; }

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
    bool accept(const QPointF &pos) {
        if (!canAccept(pos)) {
            return unaccept();
        }
        bool wasAccepted = mAccepted;
        mAccepted = true;
        return !wasAccepted;
    }
    bool unaccept() {
        bool wasAccepted = mAccepted;
        mAccepted = false;
        return wasAccepted;
    }
    bool acceptTouch(const QPointF &pos) {
        if (!canAccept(pos)) {
            return false;
        }
        return !mAcceptedTouch++;
    }
    bool unacceptTouch(const QPointF &pos) {
        if (!canAccept(pos)) {
            return false;
        }
        return !--mAcceptedTouch;
    }
    bool unacceptAllTouch() {
        bool wasAcceptedTouch = mAcceptedTouch;
        mAcceptedTouch = 0;
        return wasAcceptedTouch;
    }

protected:
    virtual bool canAccept(const QPointF &) const = 0;
    QString mLabelText;
    QPainterPath mKeyShape;

private:
    QRect mTextGeometry, mKeyGeometry;
    QColor mKeyColor;
    const KeyCode mKeycode;
    unsigned mAcceptedTouch;
    bool mPressed  : 1;
    bool mHeld     : 1;
    bool mAccepted : 1;
};

#endif
