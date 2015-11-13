#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H

#include <QDialog>

namespace Ui {
  class AboutWindow;
}

class AboutWindow : public QDialog
{
  Q_OBJECT

public:
  explicit AboutWindow(QWidget *parent = 0);
  ~AboutWindow();

private slots:
  void on_pushButton_clicked();

private:
  Ui::AboutWindow *ui;
};

#endif // ABOUTWINDOW_H
