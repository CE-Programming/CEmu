#ifndef ROMSELECTION_H
#define ROMSELECTION_H

#include <QtWidgets/QDialog>
#include <QtCore/QDir>

class QString;

namespace Ui { class RomSelection; }

class RomSelection : public QDialog {
    Q_OBJECT

public:
    explicit RomSelection(QWidget *parent = 0);
    ~RomSelection();
    std::string getROMImage();

private slots:
    bool checkImageSize(const char *filename);
    void checkInput(const QString &path);
    void createROMImageSelected();
    void openROMImageSelected();
    void browseForROM();
    void nextPageOne();
    void nextPageTwo();
    void openROMSegments();
    void saveROMImage();
    void saveDumpProgram();
    void openROMConfig();

private:
    Ui::RomSelection *ui;

    QDir currentDir;
    uint8_t *romArray = nullptr;
    uint32_t imageSize = 0;
    std::string romImagePath = "";
    bool segmentFilledStatus[30] = {0};
    int numROMSegments = 0;
};

/* External path for mainwindow */
extern std::string romImagePath;

#endif
