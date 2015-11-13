#ifndef ROMSELECTION_H
#define ROMSELECTION_H

#include <QDialog>

namespace Ui {
  class RomSelection;
}

class RomSelection : public QDialog
{
  Q_OBJECT

public:
  explicit RomSelection(QWidget *parent = 0);
  ~RomSelection();

private slots:
  void checkInput(const QString &path);
  void on_create_sel_clicked();
  void on_cancel_clicked();
  void on_browse_sel_clicked();
  void on_browse_clicked();

  void on_next_clicked();

private:
  Ui::RomSelection *ui;
};

#endif // ROMSELECTION_H
