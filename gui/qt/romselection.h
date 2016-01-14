#ifndef ROMSELECTION_H
#define ROMSELECTION_H

#include <QtWidgets/QDialog>

class QString;

bool fileExists(const QString &path);

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
  bool flash_open(const char *filename);
  void checkInput(const QString &path);
  void on_create_sel_clicked();
  void on_cancel_clicked();
  void on_browse_sel_clicked();
  void on_browse_clicked();
  void on_next_clicked();
  void on_nextButton_2_clicked();
  void on_mergeButton_clicked();
  void on_romsaveBrowse_clicked();
  void on_dumpButton_clicked();

private:
  Ui::RomSelection *ui;
  uint8_t *rom_array = nullptr;
  unsigned int num_read_segments = 0;
  bool segment_filled[20] = {0};

};

/* External path for mainwindow */
extern std::string romImagePath;

#endif
