#ifndef LCDWIDGET_H
#define LCDWIDGET_H

#include <QWidget>
#include <QTimer>

#include <qtframebuffer.h>

class LCDWidget : public QWidget
{
  Q_OBJECT
  public:
      void refreshRate(int newrate);
      LCDWidget(QWidget *p = 0);
      ~LCDWidget();

  protected:
      virtual void paintEvent(QPaintEvent * /*event*/) Q_DECL_OVERRIDE;

  private:
      QTimer refresh_timer;
  };

#endif // LCDWIDGET_H
