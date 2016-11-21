#ifndef ROMSELECTION_H
#define ROMSELECTION_H

#include <QtWidgets/QDialog>
#include <QtCore/QDir>

class QString;

namespace Ui { class RomSelection; }

class RomSelection : public QDialog {
    Q_OBJECT

public:
    explicit RomSelection(QWidget *parent = Q_NULLPTR);
    ~RomSelection();
    QString romPath();

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
    void backPage();

private:
    Ui::RomSelection *ui;

    QDir currentDir;
    uint8_t *romArray = Q_NULLPTR;
    uint32_t imageSize = 0;
    QString rom;
    bool segmentFilledStatus[30] = {0};
    int numROMSegments = 0;
};

#endif
