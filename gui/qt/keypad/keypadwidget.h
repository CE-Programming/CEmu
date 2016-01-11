#ifndef KEYPADWIDGET_H
#define KEYPADWIDGET_H

#include "keyconfig.h"
#include "key.h"

#include <QtWidgets/QWidget>

class KeypadWidget : public QWidget {
    Q_OBJECT

public:
    KeypadWidget(QWidget *parent = nullptr) : QWidget{parent}, m_keys{} { }
    virtual ~KeypadWidget();

    void setType(bool);

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

    static const size_t s_rows{8}, s_cols{8};
    static const QRect s_baseRect;
    bool m_type;
    KeyConfig m_config;
    QLinearGradient m_background;
    QTransform m_transform, m_inverseTransform;
    Key *m_keys[s_rows][s_cols];
};

#endif
