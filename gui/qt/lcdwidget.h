#ifndef LCDWIDGET_H
#define LCDWIDGET_H

#include <QtWidgets/QWidget>
#include <QtCore/QTimer>

#include "qtframebuffer.h"

class LCDWidget : public QWidget
{
  Q_OBJECT
  public:
      LCDWidget(QWidget *p = 0);
      ~LCDWidget();
      void refreshRate(int newrate);

  signals:
      void lcdOpenRequested();
      void closed();

  protected:
      virtual void paintEvent(QPaintEvent */*event*/) Q_DECL_OVERRIDE;
      virtual void showEvent(QShowEvent *e) Q_DECL_OVERRIDE;
      virtual void hideEvent(QHideEvent *e) Q_DECL_OVERRIDE;
      virtual void closeEvent(QCloseEvent *e) Q_DECL_OVERRIDE;

  private:
      void drawContext(const QPoint& posa);

      int lcdSize = 0;
      bool state_set = false;
      QTimer refreshTimer;
  };

#endif
