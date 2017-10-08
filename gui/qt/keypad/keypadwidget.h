#ifndef KEYPADWIDGET_H
#define KEYPADWIDGET_H

#include "keyconfig.h"
#include "key.h"

#include <QtWidgets/QWidget>

enum keypad_colors { KEYPAD_BLACK=0, KEYPAD_WHITE, KEYPAD_TRUE_BLUE, KEYPAD_DENIM, KEYPAD_SILVER, KEYPAD_PINK, KEYPAD_PLUM, KEYPAD_RED, KEYPAD_LIGHTNING, KEYPAD_GOLDEN, KEYPAD_SPACEGREY, KEYPAD_CORAL, KEYPAD_MINT};

class KeypadWidget : public QWidget {
    Q_OBJECT

public:
    explicit KeypadWidget(QWidget *parent = Q_NULLPTR) : QWidget{parent}, mKeys{} { }
    virtual ~KeypadWidget();

    void setType(bool, unsigned int);
    unsigned getCurrColor(void);

signals:
    void keyPressed(QString key);

public slots:
    void changeKeyState(KeyCode keycode, bool press, bool toggleHold = false);

protected:
    virtual void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;
    virtual void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    virtual void mouseMoveEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent *) Q_DECL_OVERRIDE;

private:
    void addKey(Key *);

    unsigned int color = KEYPAD_BLACK;
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
