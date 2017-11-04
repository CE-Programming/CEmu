#ifndef ROMSELECTION_H
#define ROMSELECTION_H

#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtCore/QDir>
#include <QtCore/QMimeData>
#include <QtCore/QPoint>
#include <QtGui/QDrag>

class QString;

namespace Ui { class RomSelection; }

class DropArea : public QLabel {
    Q_OBJECT

public:
    DropArea(QWidget *p = Q_NULLPTR);

public slots:
    void clear();

signals:
    void changed(const QMimeData *mimeData = Q_NULLPTR);
    void processDrop(QDropEvent*);
    void clicked(const QPoint&);

protected:
    virtual void dragEnterEvent(QDragEnterEvent *e) Q_DECL_OVERRIDE;
    virtual void dragMoveEvent(QDragMoveEvent *e) Q_DECL_OVERRIDE;
    virtual void dragLeaveEvent(QDragLeaveEvent *e) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent *e) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;

private:
    QLabel *label;
};

class RomSelection : public QDialog {
    Q_OBJECT

public:
    explicit RomSelection(QWidget *parent = Q_NULLPTR);
    ~RomSelection();
    QString getRomPath();

public slots:
    void processDrop(QDropEvent*);

private slots:
    void saveROMImage();
    void browseForROM();
    void saveDumpProgram();
    void openROMSegments();

private:
    void parseROMSegments();
    void nextPage();
    void prevPage();

    Ui::RomSelection *ui;

    QDir currentDir;
    uint8_t *romArray = Q_NULLPTR;
    uint32_t imageSize = 0;
    QString rom;
    bool segmentFilledStatus[30] = {0};
    int numROMSegments = 0;
    QStringList segmentFileList;

    bool config = false;
    bool allocedmem = false;
};

#endif
