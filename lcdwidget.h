#ifndef LCDWIDGET_H
#define LCDWIDGET_H

#include <QWidget>
#include <QImage>
#include <QTimer>

#include <qtframebuffer.h>

class LCDWidget : public QWidget
{
  Q_OBJECT
  public:
      LCDWidget(QWidget *parent);
      ~LCDWidget();

  protected:
      virtual void paintEvent(QPaintEvent * /*event*/) Q_DECL_OVERRIDE;

  private:
      QTimer refresh_timer;
  };

#endif // LCDWIDGET_H
