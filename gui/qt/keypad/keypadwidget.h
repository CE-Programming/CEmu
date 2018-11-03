#ifndef KEYPADWIDGET_H
#define KEYPADWIDGET_H

#include "keyconfig.h"
#include "key.h"

#include <QtWidgets/QWidget>
QT_BEGIN_NAMESPACE
class QResizeEvent;
class QPaintEvent;
class QEvent;
class QMouseEvent;
class QTouchEvent;
QT_END_NAMESPACE

enum keypad_colors { KEYPAD_BLACK=0, KEYPAD_WHITE, KEYPAD_TRUE_BLUE, KEYPAD_DENIM, KEYPAD_SILVER, KEYPAD_PINK, KEYPAD_PLUM, KEYPAD_RED, KEYPAD_LIGHTNING, KEYPAD_GOLDEN, KEYPAD_SPACEGREY, KEYPAD_CORAL, KEYPAD_MINT, KEYPAD_ROSEGOLD, KEYPAD_CRYSTALCLEAR };

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

    void setType(bool, unsigned int);
    unsigned getCurrColor(void);

signals:
    void keyPressed(const QString& key);

public slots:
    void changeKeyState(KeyCode keycode, bool press);

    void resizeEvent(QResizeEvent *) override;
    void paintEvent(QPaintEvent *) override;
    bool event(QEvent *) override;
    void mouseUpdateEvent(QMouseEvent *);
    void mouseEndEvent(QMouseEvent *);
    void mouseEvent(QMouseEvent *);
    void touchUpdateEvent(QTouchEvent *);
    void touchEndEvent();
    void touchEvent(QTouchEvent *);

private:
    void updateKey(Key *, bool);
    void addKey(Key *);

    unsigned int color = KEYPAD_BLACK;
    QColor cclrBackground;
    QPainterPath keypadPath;
    static const size_t sRows{8}, sCols{8};
    static const QRect sBaseRect;
    KeyConfig mConfig;
    QLinearGradient mBackground;
    QTransform mTransform, mInverseTransform;
    Key *mKeys[sRows][sCols];
#ifndef Q_OS_WIN
    int fontId = -2;
#endif
    QColor cCenter, cSides, cNum, cText, cOther, cGraph;
};

#endif
