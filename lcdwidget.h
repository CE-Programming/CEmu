#ifndef LCDWIDGET_H
#define LCDWIDGET_H

#include <QtWidgets/QWidget>
#include <QtCore/QTimer>

#include "qtframebuffer.h"

class LCDWidget : public QWidget
{
  Q_OBJECT
  public:
      void refreshRate(int newrate);
      LCDWidget(QWidget *p = 0);
      ~LCDWidget();

  signals:
      void lcdOpenRequested();

  protected:
      virtual void paintEvent(QPaintEvent * /*event*/) Q_DECL_OVERRIDE;

  private:
      void drawContext(const QPoint& posa);

      QTimer refresh_timer;
  };

#endif
