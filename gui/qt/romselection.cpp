#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include "romselection.h"
#include "ui_romselection.h"

#include "utils.h"
#include "../../core/os/os.h"

#define ROM_SIZE 0x400000
#define SEG_SIZE 0xFFE9
#define CERT_LOC 0x3B0000

static const uint8_t dumper_program[] = {
    0x2A, 0x2A, 0x54, 0x49, 0x38, 0x33, 0x46, 0x2A, 0x1A, 0x0A, 0x00, 0x46, 0x69, 0x6C, 0x65, 0x20, 0x67, 0x65, 0x6E,
    0x65, 0x72, 0x61, 0x74, 0x65, 0x64, 0x20, 0x62, 0x79, 0x20, 0x57, 0x61, 0x62, 0x62, 0x69, 0x74, 0x53, 0x69, 0x67,
    0x6E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF1, 0x00, 0x0D, 0x00,
    0xE0, 0x00, 0x06, 0x44, 0x55, 0x4D, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x00, 0xDE, 0x00, 0xEF, 0x7B,
    0x21, 0xFF, 0xFF, 0x00, 0xCD, 0x1C, 0x05, 0x02, 0xD8, 0xCD, 0x14, 0x08, 0x02, 0x21, 0x43, 0xA9, 0xD1, 0x01, 0x03,
    0x00, 0x00, 0xCD, 0x20, 0xA9, 0xD1, 0xEB, 0xED, 0x5B, 0x05, 0x01, 0x02, 0xED, 0x1F, 0xCD, 0x48, 0x14, 0x02, 0x21,
    0x4B, 0xA9, 0xD1, 0x36, 0x31, 0xE5, 0x21, 0x43, 0xA9, 0xD1, 0x01, 0xE9, 0xFF, 0x00, 0xCD, 0x20, 0xA9, 0xD1, 0x21,
    0x00, 0x00, 0x3B, 0xED, 0xB0, 0xCD, 0x48, 0x14, 0x02, 0xE1, 0x36, 0x40, 0xCD, 0x28, 0x08, 0x02, 0x21, 0x4C, 0xA9,
    0xD1, 0xCD, 0xC0, 0x07, 0x02, 0x3A, 0x4B, 0xA9, 0xD1, 0x3C, 0x32, 0x4B, 0xA9, 0xD1, 0xCD, 0xB8, 0x07, 0x02, 0x3E,
    0x2E, 0xCD, 0xB8, 0x07, 0x02, 0xCD, 0xB8, 0x07, 0x02, 0xCD, 0xB8, 0x07, 0x02, 0x21, 0x43, 0xA9, 0xD1, 0x01, 0xE9,
    0xFF, 0x00, 0xC5, 0xCD, 0x20, 0xA9, 0xD1, 0x21, 0x00, 0x00, 0x00, 0xE5, 0xED, 0xB0, 0xE1, 0xC1, 0x09, 0x22, 0xFB,
    0xA8, 0xD1, 0xE5, 0xCD, 0x48, 0x14, 0x02, 0xE1, 0xED, 0x5B, 0x05, 0x01, 0x02, 0xB7, 0xED, 0x52, 0x38, 0xAE, 0xCD,
    0x28, 0x08, 0x02, 0xC3, 0x14, 0x08, 0x02, 0xC5, 0xC5, 0xE5, 0xCD, 0x20, 0x03, 0x02, 0xCD, 0x0C, 0x05, 0x02, 0xD4,
    0x34, 0x14, 0x02, 0xE1, 0xCD, 0x20, 0x03, 0x02, 0xE1, 0xCD, 0x30, 0x13, 0x02, 0xD5, 0xCD, 0xC8, 0x02, 0x02, 0xD1,
    0x13, 0x13, 0xC1, 0xC9, 0x15, 0x52, 0x4F, 0x4D, 0x44, 0x61, 0x74, 0x61, 0x30, 0x44, 0x75, 0x6D, 0x70, 0x69, 0x6E,
    0x67, 0x20, 0x53, 0x65, 0x67, 0x6D, 0x65, 0x6E, 0x74, 0x20, 0x00, 0x87, 0x5D
};

RomSelection::RomSelection(QWidget *p) : QDialog(p), ui(new Ui::RomSelection) {
    ui->setupUi(this);

    setWindowModality(Qt::NonModal);
    setWindowFlags(Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint );

    connect(ui->buttonClose, &QToolButton::clicked, this, &RomSelection::close);
    connect(ui->buttonOpen, &QToolButton::clicked, this, &RomSelection::browseForROM);
    connect(ui->buttonCreate, &QToolButton::clicked, this, &RomSelection::nextPage);
    connect(ui->buttonNext2, &QPushButton::clicked, this, &RomSelection::nextPage);
    connect(ui->buttonBack1, &QPushButton::clicked, this, &RomSelection::prevPage);
    connect(ui->buttonBack2, &QPushButton::clicked, this, &RomSelection::prevPage);
    connect(ui->buttonDump, &QPushButton::clicked, this, &RomSelection::saveDumpProgram);
    connect(ui->buttonBrowseSave, &QPushButton::clicked, this, &RomSelection::saveROMImage);
    connect(ui->dropArea, &DropArea::clicked, this, &RomSelection::openROMSegments);

    // ensure we are on the correct page
    ui->stackedWidget->setCurrentIndex(0);

    // drop stuff
    ui->dropArea->clear();
    connect(ui->dropArea, &DropArea::processDrop, this, &RomSelection::processDrop);

    ui->versionLabel->setText(ui->versionLabel->text() + QStringLiteral(CEMU_VERSION " (git: " CEMU_GIT_SHA ")"));

    ui->progressBar->setEnabled(false);
}

RomSelection::~RomSelection() {
    delete romArray;
    delete ui;
}

void RomSelection::browseForROM() {
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setWindowTitle(tr("Select ROM file"));
    dialog.setNameFilter(tr("ROM Image (*.rom *.Rom *.ROM);;All Files (*.*)"));
    if (dialog.exec()) {
        QStringList selected = dialog.selectedFiles();
        rom = selected.first();
        close();
    }
}

void RomSelection::nextPage() {
    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex() + 1);
}

void RomSelection::prevPage() {
    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex() - 1);
}

void RomSelection::openROMSegments() {
    QFileDialog dialog(this);


    dialog.setDirectory(currentDir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilter(QStringLiteral("ROMData (*.8xv *.8Xv *.8xV .8XV)"));

    if (!dialog.exec()) {
        return;
    }

    currentDir = dialog.directory();

    segmentFileList = dialog.selectedFiles();
    parseROMSegments();
}

void RomSelection::parseROMSegments() {
    FILE* seg;
    int i, segint;
    uint8_t buf[10];
    uint16_t u16 = 0;

    if (!allocedmem) {
        romArray = new uint8_t[ROM_SIZE];
        memset(romArray, 255, ROM_SIZE);
        allocedmem = true;
    }

    for (i = 0; i < segmentFileList.size(); i++) {
        ui->progressBar->setEnabled(true);
        seg = fopen_utf8(segmentFileList.at(i).toStdString().c_str(), "rb");
        if (!seg) goto _err;
        if (fseek(seg, 0x3C, SEEK_SET)) goto _err;
        if (fread(buf, 1, 8, seg) != 8) goto _err;

        if (!memcmp(buf, "ROMData", 7)) {
            if (buf[7] == '1') {
                if (fseek(seg, 0x48, 0)) goto _err;
                if (fread(&u16, sizeof(u16), 1 ,seg) != 1) goto _err;
                if (SEG_SIZE == u16) {
                    if (fread(&romArray[CERT_LOC], SEG_SIZE, 1, seg) != 1) {
                        goto _err;
                    }
                    ui->progressBar->setValue(ui->progressBar->value() + 1);
                }
            } else if (buf[7] == '0') {
                if (fseek(seg, 0x4A, 0)) goto _err;
                if (fread(buf, 1, 3, seg) != 3) goto _err;

                osSize = static_cast<uint32_t>(buf[0] << 0) |
                         static_cast<uint32_t>(buf[1] << 8) |
                         static_cast<uint32_t>(buf[2] << 16);

                numSegments = (osSize/SEG_SIZE) + 2;
                ui->progressBar->setMaximum(numSegments);
                config = true;
            } else {
                if (fseek(seg, 0x48, 0)) goto _err;
                if (fread(&u16, sizeof(u16), 1 ,seg) != 1) goto _err;
                if (SEG_SIZE == u16) {
                    segint = buf[7] - 'A';
                    if (segmentFilledStatus[segint] == false) {
                        segmentFilledStatus[segint] = true;
                        if (fread(&romArray[SEG_SIZE * segint], SEG_SIZE, 1, seg) != 1) {
                            goto _err;
                        }
                        ui->progressBar->setValue(ui->progressBar->value() + 1);
                    }
                }
            }
        } else {
            goto _err;
        }

        fclose(seg);
    }
    segmentFileList.clear();
    if (config && ui->progressBar->value() == numSegments) {
        ui->buttonBrowseSave->setEnabled(true);
    }
    return;

_err:
    QMessageBox::critical(this, tr("Error"), tr("Invalid ROM segment\n") + segmentFileList.at(i));
    fclose(seg);
    return;
}

void RomSelection::saveDumpProgram() {
    FILE* file;
    QFileDialog dialog(this);

    dialog.setFileMode(QFileDialog::AnyFile);
    QString filename = dialog.getSaveFileName(this, tr("Save ROM Dumper Program"), currentDir.absolutePath(), tr("ROM Dumper (*.8xp)"));

    currentDir = dialog.directory();

    if (filename.isEmpty()) { return; }

    if (!filename.endsWith(QStringLiteral(".8xp"), Qt::CaseInsensitive)) {
        filename += QStringLiteral(".8xp");
    }

    file = fopen_utf8(filename.toStdString().c_str(), "wb");

    if (file) {
        fwrite(dumper_program, sizeof(dumper_program), 1, file);
        fclose(file);
    }
}

void RomSelection::saveROMImage() {
    FILE* saveRom;

    QFileDialog dialog(this);

    dialog.setFileMode(QFileDialog::AnyFile);
    QString filename = dialog.getSaveFileName(this, tr("Save ROM"), currentDir.absolutePath(), tr("ROM Image (*.rom)"));

    currentDir = dialog.directory();

    if  (filename.isEmpty()) { return; }

    if (!filename.endsWith(QStringLiteral(".rom"), Qt::CaseInsensitive)) {
        filename += QStringLiteral(".rom");
    }

    saveRom = fopen_utf8(filename.toStdString().c_str(), "wb");

    if (saveRom) {
        fwrite(romArray, 1, ROM_SIZE, saveRom);
        fclose(saveRom);
        rom = filename;
        close();
    }
}

QString RomSelection::getRomPath() {
    return rom;
}

/*!
 * DropArea
 */

DropArea::DropArea(QWidget *p) : QLabel(p) {
    setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);
    setAlignment(Qt::AlignCenter);
    setAcceptDrops(true);
    setAutoFillBackground(true);
    clear();
}

void DropArea::dragEnterEvent(QDragEnterEvent *e) {
    setBackgroundRole(QPalette::Highlight);

    e->acceptProposedAction();
    emit changed(e->mimeData());
}

void DropArea::dragMoveEvent(QDragMoveEvent *e) {
    e->acceptProposedAction();
}

void DropArea::dragLeaveEvent(QDragLeaveEvent *e) {
    clear();
    e->accept();
}

void DropArea::clear() {
    setBackgroundRole(QPalette::Dark);
    emit changed();
}

void DropArea::dropEvent(QDropEvent *e) {
    setBackgroundRole(QPalette::Dark);
    emit processDrop(e);
}

void DropArea::mousePressEvent(QMouseEvent *e) {
    emit clicked(e->pos());
}

void RomSelection::processDrop(QDropEvent *e) {
    const QMimeData *mimeData = e->mimeData();

    segmentFileList.clear();

    if (mimeData->hasUrls()) {
        const QList<QUrl> urlList = mimeData->urls();
        foreach (const QUrl &url, urlList) {
            segmentFileList.append(url.toLocalFile());
        }
    }

    e->acceptProposedAction();
    parseROMSegments();
}

/*!
 * rom dumping program
 * assemble with spasm-ng

#include "include/ti84pce.inc"

.assume ADL=1
.db $EF,$7B
.org usermem

dump:
	ld	hl,$FFFF
	call	_EnoughMem
	ret	c
	call	_ClrScrn
	ld	hl,name
	ld	bc,3
	call	create
	ex	de,hl
	ld	de,(_OSSize+1)
	ld	(hl),de
	call	_Arc_Unarc
	ld	hl,num
	ld	(hl),'1'
	push	hl
	ld	hl,name
	ld	bc,$FFFF-22
	call	create
	ld	hl,$3b0000
	ldir
	call	_Arc_Unarc
	pop	hl
	ld	(hl),'A'-1
loop:
	call	_HomeUp
	ld	hl,string
	call	_PutS
	ld	a,(num)
	inc	a
	ld	(num),a
	call	_PutC
	ld	a,'.'
	call	_PutC
	call	_PutC
	call	_PutC
	ld	hl,name
	ld	bc,$FFFF-22
	push	bc
	call	create
addr:
	ld	hl,0
	push	hl
	ldir
	pop	hl
	pop	bc
	add	hl,bc
	ld	(addr+1),hl
	push	hl
	call	_Arc_Unarc
	pop	hl
	ld	de,(_OSSize+1)
	or	a,a
	sbc	hl,de
	jr	c,loop
	call	_HomeUp
	jp	_ClrScrn

; input:
;  hl -> name
;  bc = size
; output:
;  de -> location
;  bc = size
;  op1 = name
create:
	push	bc
	push	bc
	push	hl
	call	_Mov9ToOP1
	call	_ChkFindSym
	call	nc,_DelVarArc
	pop	hl
	call	_Mov9ToOP1
	pop	hl
	call	_CreateAppVar
	push	de
	call	_OP4ToOP1
	pop	de
	inc	de
	inc	de
	pop	bc
	ret

name:
	.db	appvarObj,"ROMData"
num:
	.db	"0"
string:
	.db	"Dumping Segment ",0



*/
