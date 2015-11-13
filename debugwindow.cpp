#include "debugwindow.h"
#include "ui_debugwindow.h"
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QFontMetrics>
#include <QFont>

DebugWindow::DebugWindow(QWidget *parent) :  QMainWindow(parent),ui(new Ui::DebugWindow)
{
  ui->setupUi(this);
  ui->disasm_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  scene = new QGraphicsScene();
  QFont font("Courier",9);
  QFontMetrics font_metrics(font);
  int padding = 1;
  int column_width = font_metrics.width("X") + 30 * 2;
  int row_height = font_metrics.height() + padding * 2;
  int rows = 255, columns = 3;
  QGraphicsSimpleTextItem *item = scene->addSimpleText(QString("Address"), font);
  item->setPos(0, 0);
  for(int x = 0; x < columns; x++) {
    for(int y = 1; y < rows+1; y++) {
      item = scene->addSimpleText(QString("%1").arg(y, 5, 16, QChar('0')), font);
      item->setPos(x * column_width, y * row_height);
    }
  }
  ui->disasm_view->setScene(scene);
}

DebugWindow::~DebugWindow()
{
  delete ui;
}

void DebugWindow::add_disasm_row()
{

}
