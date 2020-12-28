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

#include "romdialog.h"

#include "settings.h"

#include "../../core/os/os.h"

#include <QtCore/QMimeData>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDragLeaveEvent>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#define ROM_SIZE 0x400000
#define CERT_LOC 0x3B0000
#define SEG_SIZE 65512

static const uint8_t sPrgmDumper[] =
{
    0x2a, 0x2a, 0x54, 0x49, 0x38, 0x33, 0x46, 0x2a, 0x1a, 0x0a, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xf1, 0x00, 0x0d, 0x00, 0xe0, 0x00, 0x06,
    0x44, 0x55, 0x4d, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00,
    0xde, 0x00, 0xef, 0x7b, 0x21, 0x01, 0x01, 0x01, 0xcd, 0x1c, 0x05, 0x02,
    0xd8, 0xcd, 0x14, 0x08, 0x02, 0x21, 0x43, 0xa9, 0xd1, 0x01, 0x03, 0x00,
    0x00, 0xcd, 0x20, 0xa9, 0xd1, 0xeb, 0xed, 0x5b, 0x05, 0x01, 0x02, 0xed,
    0x1f, 0xcd, 0x48, 0x14, 0x02, 0x21, 0x4b, 0xa9, 0xd1, 0x36, 0x31, 0xe5,
    0x21, 0x43, 0xa9, 0xd1, 0x01, 0xe8, 0xff, 0x00, 0xcd, 0x20, 0xa9, 0xd1,
    0x21, 0x00, 0x00, 0x3b, 0xed, 0xb0, 0xcd, 0x48, 0x14, 0x02, 0xe1, 0x36,
    0x40, 0xcd, 0x28, 0x08, 0x02, 0x21, 0x4c, 0xa9, 0xd1, 0xcd, 0xc0, 0x07,
    0x02, 0x3a, 0x4b, 0xa9, 0xd1, 0x3c, 0x32, 0x4b, 0xa9, 0xd1, 0xcd, 0xb8,
    0x07, 0x02, 0x3e, 0x2e, 0xcd, 0xb8, 0x07, 0x02, 0xcd, 0xb8, 0x07, 0x02,
    0xcd, 0xb8, 0x07, 0x02, 0x21, 0x43, 0xa9, 0xd1, 0x01, 0xe8, 0xff, 0x00,
    0xc5, 0xcd, 0x20, 0xa9, 0xd1, 0x21, 0x00, 0x00, 0x00, 0xe5, 0xed, 0xb0,
    0xe1, 0xc1, 0x09, 0x22, 0xfb, 0xa8, 0xd1, 0xe5, 0xcd, 0x48, 0x14, 0x02,
    0xe1, 0xed, 0x5b, 0x05, 0x01, 0x02, 0xb7, 0xed, 0x52, 0x38, 0xae, 0xcd,
    0x28, 0x08, 0x02, 0xc3, 0x14, 0x08, 0x02, 0xc5, 0xc5, 0xe5, 0xcd, 0x20,
    0x03, 0x02, 0xcd, 0x0c, 0x05, 0x02, 0xd4, 0x34, 0x14, 0x02, 0xe1, 0xcd,
    0x20, 0x03, 0x02, 0xe1, 0xcd, 0x30, 0x13, 0x02, 0xd5, 0xcd, 0xc8, 0x02,
    0x02, 0xd1, 0x13, 0x13, 0xc1, 0xc9, 0x15, 0x52, 0x4f, 0x4d, 0x44, 0x61,
    0x74, 0x61, 0x30, 0x44, 0x75, 0x6d, 0x70, 0x69, 0x6e, 0x67, 0x20, 0x53,
    0x65, 0x67, 0x6d, 0x65, 0x6e, 0x74, 0x20, 0x00, 0x8a, 0x5b
};

RomDialog::RomDialog(QWidget *parent)
    : QDialog{parent}
{
    mBtnBox = new QDialogButtonBox(QDialogButtonBox::Cancel);

    QLabel *info0 = new QLabel(tr("A ROM dumper is used to store the contents required for emulation into variables that are then combined to create a standalone ROM image. "
                                  "Click the below button to save the ROM dumper program, and then send the program to a real calculator."));
    QLabel *info1 = new QLabel(tr("To run the program, select Asm( from the catolog by pressing [2nd][0], and then pressing enter at the following prompt. "
                                  "With some models, you may not need the Asm( token, and can run the program directly."));
    QLabel *info2 = new QLabel(tr("When the program has completed, use the calculator connectivity software to store all variables prefixed with \"ROMData\" to your computer. "
                                  "Drag and drop all the saved files into the box below."));
    QLabel *info3 = new QLabel(tr("Once all the files have been transfered, the below button will be enabled. "
                                  "Choose a place to save the ROM image. CEmu will use this ROM image across program runs."));

    QImage scrn(":/assets/rom/prgm.png");
    QLabel *lblScrn = new QLabel;
    lblScrn->setPixmap(QPixmap::fromImage(scrn));

    info0->setWordWrap(true);
    info1->setWordWrap(true);
    info2->setWordWrap(true);
    info3->setWordWrap(true);

    mBtnSaveDumper = new QPushButton("Save ROM dumper", this);
    mBtnSaveImage = new QPushButton("Save ROM image", this);

    mDropArea = new DropArea;
    mDropArea->setText(tr("Drop ROMData files here (0/unk)"));
    mDropArea->setMinimumSize(scrn.width(), scrn.height() * 0.8);

    connect(mBtnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(mBtnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->addWidget(info0, Qt::AlignCenter);
    layout->addWidget(mBtnSaveDumper, Qt::AlignCenter);
    layout->addWidget(info1, Qt::AlignCenter);
    layout->addWidget(lblScrn, Qt::AlignCenter);
    layout->addWidget(info2, Qt::AlignCenter);
    layout->addWidget(mDropArea, Qt::AlignCenter);
    layout->addWidget(info3, Qt::AlignCenter);
    layout->addWidget(mBtnSaveImage, Qt::AlignCenter);
    layout->addWidget(mBtnBox, Qt::AlignCenter);

    mBtnSaveImage->setEnabled(false);

    setLayout(layout);

    setWindowTitle(tr("Create ROM"));

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    connect(mBtnSaveDumper, &QPushButton::clicked, this, &RomDialog::saveDumper);
    connect(mBtnSaveImage, &QPushButton::clicked, this, &RomDialog::saveImage);
    connect(mDropArea, &DropArea::clicked, this, &RomDialog::openSegments);
    connect(mDropArea, &DropArea::processDrop, this, &RomDialog::processDrop);
}

RomDialog::~RomDialog()
{
}

void RomDialog::processDrop(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    mSegments.clear();

    if (mimeData->hasUrls())
    {
        const QList<QUrl> urlList = mimeData->urls();
        foreach (const QUrl &url, urlList)
        {
            mSegments.append(url.toLocalFile());
        }
    }

    event->acceptProposedAction();
    parseROMSegments();
}

void RomDialog::openSegments()
{
    QFileDialog dialog(this);

    dialog.setDirectory(mDir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilter(QStringLiteral("ROMData (*.8xv *.8Xv *.8xV .8XV)"));

    if (!dialog.exec())
    {
        return;
    }

    mDir = dialog.directory();

    mSegments = dialog.selectedFiles();
    parseROMSegments();
}

void RomDialog::parseROMSegments()
{
    FILE *fd;
    int i, seg;
    uint8_t buf[10];
    uint16_t size = 0;
    int dumpsize;

    if (mArray == nullptr)
    {
        mArray = new uint8_t[ROM_SIZE];
        memset(mArray, 255, ROM_SIZE);
    }

    for (i = 0; i < mSegments.size(); ++i)
    {
        fd = fopen_utf8(mSegments.at(i).toStdString().c_str(), "rb");
        if (!fd) goto invalid;
        if (fseek(fd, 0x3C, SEEK_SET)) goto invalid;
        if (fread(buf, 1, 8, fd) != 8) goto invalid;
        if (memcmp(buf, "ROMData", 7)) goto invalid;

        switch (buf[7]) {
            // metadata
            case '0':
                if (fseek(fd, 0x4A, 0)) goto invalid;
                if (fread(buf, 1, 3, fd) != 3) goto invalid;

                dumpsize = static_cast<int>(buf[0] << 0) |
                           static_cast<int>(buf[1] << 8) |
                           static_cast<int>(buf[2] << 16);

                mTotalSegments = (dumpsize / SEG_SIZE) + 2;
                mHasMetaSegment = true;
                mNumSentSegments++;
                break;

            // certificate
            case '1':
                if (fseek(fd, 0x48, 0)) goto invalid;
                if (fread(&size, sizeof(size), 1 ,fd) != 1) goto invalid;
                if (size != SEG_SIZE) goto invalid;

                if (fread(&mArray[CERT_LOC], SEG_SIZE, 1, fd) != 1) {
                    goto invalid;
                }
                mNumSentSegments++;
                break;

            default:
                if (fseek(fd, 0x48, 0)) goto invalid;
                if (fread(&size, sizeof(size), 1 ,fd) != 1) goto invalid;
                if (size != SEG_SIZE) goto invalid;

                seg = buf[7] - 'A';
                if (mStatus[seg] == false) {
                    mStatus[seg] = true;
                    if (fread(&mArray[SEG_SIZE * seg], SEG_SIZE, 1, fd) != 1) {
                        goto invalid;
                    }
                    mNumSentSegments++;
                }
                break;
        }

        fclose(fd);
    }
    mDropArea->setText(tr("Drop ROMData files here (%1/%2)").arg(mNumSentSegments).arg(mTotalSegments == 0 ? "unk" : QString(mTotalSegments)));

    mSegments.clear();
    if (mHasMetaSegment && mNumSentSegments == mTotalSegments)
    {
        mBtnSaveImage->setEnabled(true);
    }
    return;

invalid:
    QMessageBox::critical(this, tr("Error"), tr("Invalid ROM segment\n") + mSegments.at(i));
    fclose(fd);
    return;
}

void RomDialog::saveDumper()
{
    FILE* file;
    QFileDialog dialog(this);

    dialog.setFileMode(QFileDialog::AnyFile);
    QString filename = dialog.getSaveFileName(this, tr("Save ROM Dumper Program"), mDir.absolutePath(), tr("ROM Dumper (*.8xp)"));

    mDir = dialog.directory();

    if (filename.isEmpty()) {
        return;
    }

    if (!filename.endsWith(QStringLiteral(".8xp"), Qt::CaseInsensitive))
    {
        filename += QStringLiteral(".8xp");
    }

    file = fopen_utf8(filename.toStdString().c_str(), "wb");

    if (file)
    {
        fwrite(sPrgmDumper, sizeof(sPrgmDumper), 1, file);
        fclose(file);
    }
}

void RomDialog::saveImage()
{
    FILE* saveRom;

    QFileDialog dialog(this);

    dialog.setFileMode(QFileDialog::AnyFile);
    QString filename = dialog.getSaveFileName(this, tr("Save ROM"), mDir.absolutePath(), tr("ROM Image (*.rom)"));

    mDir = dialog.directory();

    if  (filename.isEmpty())
        return;

    if (!filename.endsWith(QStringLiteral(".rom"), Qt::CaseInsensitive))
        filename += QStringLiteral(".rom");

    saveRom = fopen_utf8(filename.toStdString().c_str(), "wb");
    if (saveRom)
    {
        mRomPath = filename;

        fwrite(mArray, 1, ROM_SIZE, saveRom);
        fclose(saveRom);
        close();
    }
    else
    {
        QMessageBox::critical(this, tr("Error"), tr("Could not save ROM image."));
    }
}

QString RomDialog::romFile() const
{
    return mRomPath;
}

DropArea::DropArea(QWidget *parent)
    : QLabel{parent}
{
    setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);
    setAlignment(Qt::AlignCenter);
    setAcceptDrops(true);
    setAutoFillBackground(true);
    clear();
}

void DropArea::dragEnterEvent(QDragEnterEvent *event)
{
    setBackgroundRole(QPalette::Highlight);

    event->acceptProposedAction();
    emit changed(event->mimeData());
}

void DropArea::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void DropArea::dragLeaveEvent(QDragLeaveEvent *event)
{
    clear();
    event->accept();
}

void DropArea::clear()
{
    setBackgroundRole(QPalette::Dark);
    emit changed();
}

void DropArea::dropEvent(QDropEvent *event)
{
    setBackgroundRole(QPalette::Dark);
    emit processDrop(event);
}

void DropArea::mousePressEvent(QMouseEvent *event)
{
    emit clicked(event->pos());
}
