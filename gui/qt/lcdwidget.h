#ifndef LCDWIDGET_H
#define LCDWIDGET_H

#include <QtWidgets/QWidget>
#include <QtCore/QTimer>

#include "qtframebuffer.h"
#include "../../core/lcd.h"

class LCDWidget : public QWidget {
  Q_OBJECT

public:
    explicit LCDWidget(QWidget *p = Q_NULLPTR);
    ~LCDWidget();
    void refreshRate(int);
    void setLCD(lcd_state_t*);

protected:
    virtual void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent*) Q_DECL_OVERRIDE;
    virtual void dragEnterEvent(QDragEnterEvent*) Q_DECL_OVERRIDE;
    virtual void dragLeaveEvent(QDragLeaveEvent*) Q_DECL_OVERRIDE;
    virtual void dragMoveEvent(QDragMoveEvent*) Q_DECL_OVERRIDE;

private:
    enum lcd_side {
        LCD_LEFT=0,
        LCD_RIGHT
    };

    int lcdSize = 0;
    unsigned int side_drag;
    bool state_set = false;
    bool in_drag = false;
    QTimer *refreshTimer;
    QPainter *painter;
    lcd_state_t *lcdState;
};

#endif
