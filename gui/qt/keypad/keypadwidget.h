#ifndef KEYPADWIDGET_H
#define KEYPADWIDGET_H

#include "../../../core/asic.h"
#include "keyconfig.h"
#include "key.h"

#include <QtCore/QList>
#include <QtCore/QMultiHash>
#include <QtCore/QSet>
#include <QtGui/QTouchEvent>
#include <QtWidgets/QWidget>
QT_BEGIN_NAMESPACE
class QResizeEvent;
class QPaintEvent;
class QEvent;
class QMouseEvent;
QT_END_NAMESPACE

enum keypad_colors { KEYPAD_BLACK=0, KEYPAD_WHITE, KEYPAD_TRUE_BLUE, KEYPAD_DENIM, KEYPAD_SILVER, KEYPAD_PINK, KEYPAD_PLUM, KEYPAD_RED, KEYPAD_LIGHTNING, KEYPAD_GOLDEN, KEYPAD_SPACEGREY, KEYPAD_CORAL, KEYPAD_MINT, KEYPAD_ROSEGOLD, KEYPAD_CRYSTALCLEAR, KEYPAD_MATTEBLACK, KEYPAD_TANGENTTEAL, KEYPAD_TOTALLYTEAL };

class KeypadWidget : public QWidget {
    Q_OBJECT

public:
    explicit KeypadWidget(QWidget *parent = Q_NULLPTR) : QWidget{parent}, cclrBackground{Qt::gray}, mKeys{} {
        setAttribute(Qt::WA_AcceptTouchEvents);
        cclrBackground.setAlpha(100);
        keypadPath.setFillRule(Qt::WindingFill);
        keypadPath.addRoundedRect(sBaseRect, 20, 20);
        keypadPath.addRect(QRect(0, 0, 20, 20));
        keypadPath.addRect(QRect(sBaseRect.width()-20, 0, 20, 20));
        keypadPath = keypadPath.simplified();
    }
    virtual ~KeypadWidget();

    void setType(emu_device_t, unsigned int);
    void setHolding(bool);
    unsigned getCurrColor(void) const;

signals:
    void keyPressed(const QString& key);

public slots:
    void changeKeyState(KeyCode keycode, bool press);

    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;
    void mouseUpdate(const QPointF &pos);
    void mouseEnd(bool toggleHeld);
    void mouseEvent(QMouseEvent *event);
    void touchUpdate(const QList<QTouchEvent::TouchPoint> &points);
    void touchEnd();
    void touchEvent(QTouchEvent *event);

private:
    void updateKey(Key *key, bool);
    void addKey(Key *key);

    unsigned int color = KEYPAD_BLACK;
    bool mHoldingEnabled = true;
    QColor cclrBackground;
    QPainterPath keypadPath;
    static const size_t sRows{8}, sCols{8};
    static const QRect sBaseRect;
    KeyConfig mConfig;
    QLinearGradient mBackground;
    QTransform mTransform, mInverseTransform;
    Key *mKeys[sRows][sCols];
    QSet<KeyCode> mClicked;
    QSet<KeyCode> mTouched;
#ifndef Q_OS_WIN
    int fontId = -2;
#endif
    QColor cCenter, cSides, cNum, cText, cOther, cGraph;
};

#endif
