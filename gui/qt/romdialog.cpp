/*
 * Copyright (c) 2015-2021 CE Programming.
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

#include <QtCore/QDataStream>
#include <QtCore/QFile>
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

    QLabel *info0 = new QLabel(tr("CEmu uses an assembly program that is run on a physical calculator to extract a ROM image. "
                                  "Click the below button to save the ROM dumper program, and then send the program to a calculator."));
    QLabel *info1 = new QLabel(tr("To run the program, select Asm( from the catolog by pressing [2nd][0], select the \"DUMP\" program using the [prgm] button, "
                                  "and then press [enter] at the following prompt:"));
    QLabel *info2 = new QLabel(tr("If the Asm( token is not available, you may need to use arTIfiCE in order to run assembly programs, available at the following link:"));
    QLabel *info3 = new QLabel(tr("<a href=\"https://yvantt.github.io/arTIfiCE\">https://yvantt.github.io/arTIfiCE</a>"));
    QLabel *info4 = new QLabel(tr("When the program has completed, send all variables prefixed with \"ROMData\" stored on the calculator to your computer. "
                                  "Drag and drop all the saved files into the box below. "
                                  "Once all the \"ROMData\" files have been added, the below save button will be enabled. "));

    info3->setTextFormat(Qt::RichText);
    info3->setTextInteractionFlags(Qt::TextBrowserInteraction);
    info3->setOpenExternalLinks(true);

    QImage scrn(":/assets/rom/prgm.png");
    QLabel *lblScrn = new QLabel;
    lblScrn->setPixmap(QPixmap::fromImage(scrn));

    info0->setWordWrap(true);
    info1->setWordWrap(true);
    info2->setWordWrap(true);
    info3->setWordWrap(true);
    info4->setWordWrap(true);

    mBtnSaveDumper = new QPushButton(QIcon(QStringLiteral(":/assets/icons/save.svg")), "Save ROM dumper", this);
    mBtnSaveImage = new QPushButton(QIcon(QStringLiteral(":/assets/icons/save.svg")), "Save ROM image", this);

    mDropArea = new DropArea;
    mDropArea->setText(tr("Drop ROMData files here"));
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
    layout->addWidget(info3, Qt::AlignCenter);
    layout->addWidget(info4, Qt::AlignCenter);
    layout->addWidget(mDropArea, Qt::AlignCenter);
    layout->addWidget(mBtnSaveImage, Qt::AlignCenter);
    layout->addWidget(mBtnBox, Qt::AlignCenter);

    mBtnSaveImage->setEnabled(false);

    setLayout(layout);

    setWindowTitle(tr("Create CEmu ROM"));

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

bool RomDialog::validateROMSegment(const QString &filename)
{
    QFile file(filename);
    char type;
    if (!file.open(QIODevice::ReadOnly) || !file.seek(0x3C) ||
        file.read(7) != "ROMData" || !file.getChar(&type))
    {
        return false;
    }
    if (type == '0')
    {
        // metadata
        if (!file.seek(0x4A))
        {
            return false;
        }
        QDataStream stream(file.read(3) + '\0');
        stream.setByteOrder(QDataStream::LittleEndian);
        quint32 dumpsize;
        stream >> dumpsize;
        if (stream.status())
        {
            return false;
        }
        mTotalSegments = dumpsize / SEG_SIZE + 2;
        mHasMetaSegment = true;
    }
    else
    {
        if (!file.seek(0x48))
        {
            return false;
        }
        QDataStream stream(&file);
        stream.setByteOrder(QDataStream::LittleEndian);
        quint16 size;
        stream >> size;
        if (size != SEG_SIZE)
        {
            return false;
        }
        if (type == '1')
        {
            // certificate
            stream.readRawData(mArray.data() + CERT_LOC, SEG_SIZE);
        }
        else if (type >= 'A' && type < 'A' + 32)
        {
            int seg = type - 'A';
            if (mStatus & 1 << seg)
            {
                return true;
            }
            mStatus |= 1 << seg;
            stream.readRawData(mArray.data() + SEG_SIZE * seg, SEG_SIZE);
        }
        if (stream.status())
        {
            return false;
        }
    }
    mNumSentSegments++;
    return true;
}

void RomDialog::parseROMSegments()
{
    mArray.fill('\xFF', ROM_SIZE);

    foreach (const QString &filename, mSegments)
    {
        if (!validateROMSegment(filename))
        {
            QMessageBox::critical(this, tr("Error"), tr("Invalid ROM segment\n") + filename);
            return;
        }
    }
    mDropArea->setText(tr("Drop ROMData files here (%1/%2)").arg(mNumSentSegments).arg(mTotalSegments == 0 ? "unk" : QString::number(mTotalSegments)));

    mSegments.clear();
    if (mHasMetaSegment && mNumSentSegments == mTotalSegments)
    {
        mBtnSaveImage->setEnabled(true);
    }
    return;
}

void RomDialog::saveDumper()
{
    QFileDialog dialog{this};

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

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly) || file.write(reinterpret_cast<const char *>(sPrgmDumper),
                                                       sizeof(sPrgmDumper)) != sizeof(sPrgmDumper))
    {
        QMessageBox::critical(this, tr("Error"), tr("Could not save ROM Dumper program."));
    }
}

void RomDialog::saveImage()
{
    QFileDialog dialog(this);

    dialog.setFileMode(QFileDialog::AnyFile);
    QString filename = dialog.getSaveFileName(this, tr("Save ROM"), mDir.absolutePath(), tr("ROM Image (*.rom)"));

    mDir = dialog.directory();

    if  (filename.isEmpty())
        return;

    if (!filename.endsWith(QStringLiteral(".rom"), Qt::CaseInsensitive))
        filename += QStringLiteral(".rom");

    QFile saveRom(filename);
    if (saveRom.open(QIODevice::WriteOnly) && saveRom.write(mArray) == ROM_SIZE)
    {
        mRomPath = filename;
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
