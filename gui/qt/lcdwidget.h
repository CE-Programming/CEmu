#ifndef LCDWIDGET_H
#define LCDWIDGET_H

#include <QtWidgets/QWidget>
#include <QtCore/QTimer>

#include "../../core/lcd.h"

class LCDWidget : public QWidget {
  Q_OBJECT

public:
    explicit LCDWidget(QWidget *p = Q_NULLPTR);
    ~LCDWidget();
    void setRefreshRate(int rate);
    void setFrameskip(int skip);
    void setLCD(lcd_state_t*);
    void callback(void);
    int getFPS();

protected:
    virtual void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent*) Q_DECL_OVERRIDE;
    virtual void dragEnterEvent(QDragEnterEvent*) Q_DECL_OVERRIDE;
    virtual void dragLeaveEvent(QDragLeaveEvent*) Q_DECL_OVERRIDE;
    virtual void dragMoveEvent(QDragMoveEvent*) Q_DECL_OVERRIDE;

signals:
    void sendROM(const QString& romPath);

private slots:
    void draw();

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
    QImage image;

    int fps = 0;
    int skip = 0;
    int frameskip = 0;
    int refresh;

    // for dragable roms
    QString dragROM;
    bool isSendingROM;
};

#endif
