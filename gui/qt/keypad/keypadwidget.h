#ifndef KEYPADWIDGET_H
#define KEYPADWIDGET_H

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

    typedef enum {
        COLOR_BLACK,
        COLOR_WHITE,
        COLOR_TRUE_BLUE,
        COLOR_DENIM,
        COLOR_SILVER,
        COLOR_PINK,
        COLOR_PLUM,
        COLOR_RED,
        COLOR_LIGHTNING,
        COLOR_GOLDEN,
        COLOR_SPACEGREY,
        COLOR_CORAL,
        COLOR_MINT,
        COLOR_ROSEGOLD,
        COLOR_CRYSTALCLEAR,
        COLOR_MATTEBLACK,
        COLOR_TANGENTTEAL,
        COLOR_TOTALLYTEAL
    } keypad_color_t;

    void setType(bool is83, keypad_color_t color);
    void setHolding(bool);
    keypad_color_t getCurrColor(void);

signals:
    void keyPressed(const QString& key);
    void resized(QSize size);

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

    keypad_color_t color = KeypadWidget::COLOR_BLACK;
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
    int m22;
    int m11;
#ifndef Q_OS_WIN
    int fontId = -2;
#endif
    QColor cCenter, cSides, cNum, cText, cOther, cGraph;
};

#endif
