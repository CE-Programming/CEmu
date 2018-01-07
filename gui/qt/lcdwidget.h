#ifndef LCDWIDGET_H
#define LCDWIDGET_H

#include <QtWidgets/QWidget>
#include <QtCore/QTimer>

#include "../../core/lcd.h"

class LCDWidget : public QWidget {
  Q_OBJECT

public:
    explicit LCDWidget(QWidget *p = Q_NULLPTR);
    void setMode(bool mode);
    void setFrameskip(int skip);
    void callback(void);
    double getFPS();
    QImage getImage();

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

    // true = emulate spi
    bool mode;

    unsigned int sideDrag;
    bool drag = false;
    QRect left, right;
    QImage image;

    double fps = 0;
    int skip = 0;
    int frameskip = 0;

    // for dragable roms
    QString dragROM;
    bool isSendingROM;
};

#endif
