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
      LCDWidget(QWidget *p = 0);
      ~LCDWidget();
      void refreshRate(int newrate);
      void setLCD(lcd_state_t*);

  protected:
      virtual void paintEvent(QPaintEvent */*event*/) Q_DECL_OVERRIDE;

  private:
      int lcdSize = 0;
      bool state_set = false;
      QTimer refreshTimer;
      lcd_state_t *lcdState;
  };

#endif
