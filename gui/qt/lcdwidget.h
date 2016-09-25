#ifndef LCDWIDGET_H
#define LCDWIDGET_H

#include <QtWidgets/QWidget>
#include <QtCore/QTimer>

#include "qtframebuffer.h"
#include "../../core/lcd.h"

class LCDWidget : public QWidget
{
  Q_OBJECT
  public:
      explicit LCDWidget(QWidget *p = Q_NULLPTR);
      ~LCDWidget();
      void refreshRate(int newrate);
      void setLCD(lcd_state_t*);

  protected:
      virtual void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;
      virtual void dropEvent(QDropEvent*) Q_DECL_OVERRIDE;
      virtual void dragEnterEvent(QDragEnterEvent*) Q_DECL_OVERRIDE;
      virtual void dragLeaveEvent(QDragLeaveEvent *e) Q_DECL_OVERRIDE;

  private:
      int lcdSize = 0;
      bool state_set = false;
      bool in_drag = false;
      QTimer refreshTimer;
      lcd_state_t *lcdState;
  };

#endif
