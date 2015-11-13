#include "optionswindow.h"
#include "ui_optionswindow.h"

OptionsWindow::OptionsWindow(QWidget *p) :
  QDialog(p),
  ui(new Ui::OptionsWindow)
{
  ui->setupUi(this);
}

OptionsWindow::~OptionsWindow()
{
  delete ui;
}
