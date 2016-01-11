#ifndef LCDWIDGET_H
#define LCDWIDGET_H

#include <QtWidgets/QWidget>
#include <QtCore/QTimer>

#include "qtframebuffer.h"

class LCDWidget : public QWidget
{
  Q_OBJECT
  public:
      LCDWidget(QWidget *parent, Qt::WindowFlags f = Qt::WindowFlags());
      ~LCDWidget();
      void refreshRate(int newrate);

    public slots:
      void showEvent(QShowEvent *e) override;
      void hideEvent(QHideEvent *e) override;
      void closeEvent(QCloseEvent *e) override;

  signals:
      void lcdOpenRequested();
      void closed();

  protected:
      virtual void paintEvent(QPaintEvent */*event*/) Q_DECL_OVERRIDE;

  private:
      void drawContext(const QPoint& posa);

      bool state_set = false;
      QTimer refresh_timer;
  };

#endif
