#ifndef DEBUGWINDOW_H
#define DEBUGWINDOW_H

#include <QGraphicsScene>
#include <QMainWindow>

namespace Ui {
  class DebugWindow;
}

class DebugWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit DebugWindow(QWidget *parent = 0);
  ~DebugWindow();
  void add_disasm_row();

private:
  Ui::DebugWindow *ui;
  QGraphicsScene *scene;
};

#endif // DEBUGWINDOW_H
