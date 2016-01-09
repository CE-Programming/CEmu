#ifndef ROMSELECTION_H
#define ROMSELECTION_H

#include <QtWidgets/QDialog>
#include <stdint.h>

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
#if (defined(_MSC_VER) && _MSC_VER < 1900)
  bool segment_filled[20]; /* No support of non-static data member initializers. Filled later */
#else
  bool segment_filled[20] = {0};
#endif

};

/* External path for mainwindow */
extern std::string romImagePath;

#endif
