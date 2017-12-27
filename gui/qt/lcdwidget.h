#ifndef LCDWIDGET_H
#define LCDWIDGET_H

#include <QtWidgets/QWidget>
#include <QtCore/QTimer>

#include "qtframebuffer.h"
#include "../../core/lcd/lcd.h"

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

signals:
    void sendROM(const QString& romPath);

private:
    enum lcd_side {
        LCD_LEFT=0,
        LCD_RIGHT
    };

    unsigned int sideDrag;
    bool inDrag = false;
    QTimer *refreshTimer;
    lcd_state_t *lcdState = Q_NULLPTR;
    QRect left, right;

    // for dragable roms
    QString dragROM;
    bool isSendingROM;
};

#endif
