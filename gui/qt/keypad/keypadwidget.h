#ifndef KEYPADWIDGET_H
#define KEYPADWIDGET_H

#include "keyconfig.h"
#include "key.h"

#include <QtWidgets/QWidget>

enum keypad_colors { KEYPAD_BLACK=0, KEYPAD_WHITE, KEYPAD_TRUE_BLUE, KEYPAD_DENIM, KEYPAD_SILVER, KEYPAD_PINK, KEYPAD_PLUM, KEYPAD_RED, KEYPAD_LIGHTNING, KEYPAD_GOLDEN};

class KeypadWidget : public QWidget {
    Q_OBJECT

public:
    explicit KeypadWidget(QWidget *parent = Q_NULLPTR) : QWidget{parent}, m_keys{} { }
    virtual ~KeypadWidget();

    void setType(bool, unsigned);
    unsigned getCurrColor(void);

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

    unsigned curr_color = KEYPAD_BLACK;
    static const size_t s_rows{8}, s_cols{8};
    static const QRect s_baseRect;
    KeyConfig m_config;
    QLinearGradient m_background;
    QTransform m_transform, m_inverseTransform;
    Key *m_keys[s_rows][s_cols];
};

#endif
