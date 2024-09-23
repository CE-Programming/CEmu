#ifndef KEYPADWIDGET_H
#define KEYPADWIDGET_H

#include "keyconfig.h"
#include "key.h"
class CalculatorWidget;

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

class KeypadWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KeypadWidget(CalculatorWidget *parent);
    virtual ~KeypadWidget();
    CalculatorWidget *parent() const;

    enum class Color {
        Black,
        White,
        TrueBlue,
        Denim,
        Silver,
        Pink,
        Plum,
        Red,
        Lightning,
        Gold,
        SpaceGrey,
        Coral,
        Mint,
        RoseGold,
        CrystalClear,
        MatteBlack,
        TangentTeal,
        TotallyTeal
    };

    Color color();
    void setColor(Color color);
    void setType(bool is83);
    void setHolding(bool);

    static const QRect sBaseRect;

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
    void reset();

    Color mColor = KeypadWidget::Color::Black;
    bool m83 = false;
    bool mHoldingEnabled = true;
    QColor cclrBackground;
    QPainterPath keypadPath;
    static const size_t sRows{8}, sCols{8};
    KeyConfig mConfig;
    QLinearGradient mBackground;
    QTransform mTransform, mInverseTransform;
    Key *mKeys[sRows][sCols];
    QSet<KeyCode> mClicked;
    QSet<KeyCode> mTouched;
#ifndef Q_OS_WIN
    int fontId = -2;
#endif
};

#endif
