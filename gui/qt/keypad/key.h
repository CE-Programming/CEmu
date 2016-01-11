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
        m_textGeometry{textGeometry}, m_keyGeometry{keyGeometry}, m_keyColor{keyColor},
	m_keycode{keycode}, m_pressed{}, m_held{}, m_accepted{} {}
    virtual ~Key() {}

    const QRect &textGeometry() const { return m_textGeometry; }
    const QRect &keyGeometry() const { return m_keyGeometry; }
    const KeyCode keycode() const { return m_keycode; }
    bool isPressed() const { return m_pressed; }
    bool isHeld() const { return m_held; }
    bool isSelected() const { return isPressed() | isHeld(); }

    void setPressed(bool pressed) { m_pressed = pressed; }
    void toggleHeld() { m_held = !m_held; }

    virtual void paint(QPainter &painter) const {
	painter.setBrush(m_keyColor);
	painter.setPen({QColor::fromRgb(0x808080), .375});
	painter.drawPath(m_keyShape);
	if (isSelected()) {
	    painter.setBrush(QColor::fromRgba(0x80800000));
	    painter.drawPath(m_keyShape);
	}
    }
    bool accept(const QPointF &point) {
        bool accepted = canAccept(point);
        m_accepted |= accepted;
        return accepted;
    }
    bool unaccept() {
	bool accepted = m_accepted;
	m_accepted &= false;
	return accepted;
    }

protected:
    virtual bool canAccept(const QPointF &) = 0;

    QPainterPath m_keyShape;

private:
    QRect m_textGeometry, m_keyGeometry;
    QColor m_keyColor;
    const KeyCode m_keycode;
    quint8 m_pressed  : 1;
    quint8 m_held     : 1;
    quint8 m_accepted : 1;
};

#endif
