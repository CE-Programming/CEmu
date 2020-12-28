/*
 * Copyright (c) 2015-2020 CE Programming.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ROMDIALOG_H
#define ROMDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QImage>
#include <QLabel>
#include <QDir>
#include <QMimeData>
#include <QDragLeaveEvent>
#include <QDragEnterEvent>

class DropArea : public QLabel
{
    Q_OBJECT

public:
    DropArea(QWidget *p = Q_NULLPTR);

public slots:
    void clear();

signals:
    void changed(const QMimeData *mimeData = nullptr);
    void processDrop(QDropEvent *event);
    void clicked(const QPoint &);

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dragMoveEvent(QDragMoveEvent *event) override;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;

private:
    QLabel *mLabel;
};

class RomDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RomDialog(QWidget *parent = nullptr);
    ~RomDialog();

    QString romFile() const;

private slots:
    void processDrop(QDropEvent *event);
    void saveImage();
    void saveDumper();
    void openSegments();

private:
    void parseROMSegments();

    QPushButton *mBtnSaveDumper;
    QPushButton *mBtnSaveImage;
    QDialogButtonBox *mBtnBox;
    DropArea *mDropArea;

    QDir mDir;
    uint8_t *mArray = nullptr;
    QString mRomPath;
    bool mStatus[30] = {0};
    int mTotalSegments = 0;
    int mNumSentSegments = 0;
    QStringList mSegments;

    bool mHasMetaSegment = false;
};

#endif
