#include "romselection.h"
#include "ui_romselection.h"
#include "utils.h"
#include "../../core/os/os.h"

#include <QtCore/QtEndian>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#define ROM_SIZE 0x400000
#define CERT_LOC 0x3B0000
#define SEG_SIZE 65512

static const uint8_t dumper_program[] = {
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

RomSelection::RomSelection(QWidget *parent) : QDialog{parent}, ui(new Ui::RomSelection) {
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
    delete m_array;
    delete ui;
}

void RomSelection::browseForROM() {
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setWindowTitle(tr("Select ROM file"));
    dialog.setNameFilter(tr("ROM Image (*.rom *.Rom *.ROM);;All Files (*.*)"));
    if (dialog.exec()) {
        QStringList selected = dialog.selectedFiles();
        m_rom = selected.first();
        close();
    }
}

void RomSelection::nextPage() const {
    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex() + 1);
}

void RomSelection::prevPage() const {
    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex() - 1);
}

void RomSelection::openROMSegments() {
    QFileDialog dialog(this);


    dialog.setDirectory(m_dir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilter(QStringLiteral("ROMData (*.8xv *.8Xv *.8xV .8XV)"));

    if (!dialog.exec()) {
        return;
    }

    m_dir = dialog.directory();

    m_segs = dialog.selectedFiles();
    parseROMSegments();
}

void RomSelection::parseROMSegments() {
    FILE *fd;
    int i, seg;
    uint8_t buf[10];
    uint16_t size = 0;
    int dumpsize;

    if (!m_alloced) {
        m_array = new uint8_t[ROM_SIZE];
        memset(m_array, 255, ROM_SIZE);
        m_alloced = true;
    }

    for (i = 0; i < m_segs.size(); i++) {
        ui->progressBar->setEnabled(true);
        fd = fopen_utf8(m_segs.at(i).toStdString().c_str(), "rb");
        if (!fd) goto invalid;
        if (fseek(fd, 0x3C, SEEK_SET)) goto invalid;
        if (fread(buf, 1, 8, fd) != 8) goto invalid;
        if (memcmp(buf, "ROMData", 7)) goto invalid;

        switch (buf[7]) {
            case '0':
                if (fseek(fd, 0x4A, 0)) goto invalid;
                if (fread(buf, 1, 3, fd) != 3) goto invalid;

                dumpsize = static_cast<int>(buf[0] << 0) |
                           static_cast<int>(buf[1] << 8) |
                           static_cast<int>(buf[2] << 16);

                m_num = (dumpsize / SEG_SIZE) + 2;
                ui->progressBar->setMaximum(m_num);
                m_config = true;
                break;

            case '1':
                if (fseek(fd, 0x48, 0)) goto invalid;
                if (fread(&size, sizeof(size), 1 ,fd) != 1) goto invalid;
                size = qFromLittleEndian(size);
                if (size != SEG_SIZE) goto invalid;

                if (fread(&m_array[CERT_LOC], SEG_SIZE, 1, fd) != 1) {
                    goto invalid;
                }
                ui->progressBar->setValue(ui->progressBar->value() + 1);
                break;

            default:
                if (fseek(fd, 0x48, 0)) goto invalid;
                if (fread(&size, sizeof(size), 1 ,fd) != 1) goto invalid;
                size = qFromLittleEndian(size);
                if (size != SEG_SIZE) goto invalid;

                seg = buf[7] - 'A';
                if (m_status[seg] == false) {
                    m_status[seg] = true;
                    if (fread(&m_array[SEG_SIZE * seg], SEG_SIZE, 1, fd) != 1) {
                        goto invalid;
                    }
                    ui->progressBar->setValue(ui->progressBar->value() + 1);
                }
                break;
        }

        fclose(fd);
    }
    m_segs.clear();
    if (m_config && ui->progressBar->value() == m_num) {
        ui->buttonBrowseSave->setEnabled(true);
    }
    return;

invalid:
    QMessageBox::critical(this, tr("Error"), tr("Invalid ROM segment\n") + m_segs.at(i));
    fclose(fd);
    return;
}

void RomSelection::saveDumpProgram() {
    FILE* file;
    QFileDialog dialog(this);

    dialog.setFileMode(QFileDialog::AnyFile);
    QString filename = dialog.getSaveFileName(this, tr("Save ROM Dumper Program"), m_dir.absolutePath(), tr("ROM Dumper (*.8xp)"));

    m_dir = dialog.directory();

    if (filename.isEmpty()) {
        return;
    }

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
    QString filename = dialog.getSaveFileName(this, tr("Save ROM"), m_dir.absolutePath(), tr("ROM Image (*.rom)"));

    m_dir = dialog.directory();

    if  (filename.isEmpty()) { return; }

    if (!filename.endsWith(QStringLiteral(".rom"), Qt::CaseInsensitive)) {
        filename += QStringLiteral(".rom");
    }

    saveRom = fopen_utf8(filename.toStdString().c_str(), "wb");

    if (saveRom) {
        fwrite(m_array, 1, ROM_SIZE, saveRom);
        fclose(saveRom);
        m_rom = filename;
        close();
    }
}

QString RomSelection::getRomPath() {
    return m_rom;
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

    m_segs.clear();

    if (mimeData->hasUrls()) {
        const QList<QUrl> urlList = mimeData->urls();
        foreach (const QUrl &url, urlList) {
            m_segs.append(url.toLocalFile());
        }
    }

    e->acceptProposedAction();
    parseROMSegments();
}

/*!

; rom dumping program
; assemble with fasmg

include 'include/ez80.inc'
include 'include/ti84pceg.inc'
include 'include/tiformat.inc'
format ti executable 'DUMP'

maxsize := 65512
certloc := $3b0000

dump:
	ld	hl,$10101
	call	ti.EnoughMem
	ret	c
	call	ti.ClrScrn
	ld	hl,name
	ld	bc,3
	call	create
	ex	de,hl
	ld	de,(ti.OSSize + 1)
	ld	(hl),de
	call	ti.Arc_Unarc
	ld	hl,num
	ld	(hl),'1'
	push	hl
	ld	hl,name
	ld	bc,maxsize
	call	create
	ld	hl,certloc
	ldir
	call	ti.Arc_Unarc
	pop	hl
	ld	(hl),'A'-1
loop:
	call	ti.HomeUp
	ld	hl,segstr
	call	ti.PutS
	ld	a,(num)
	inc	a
	ld	(num),a
	call	ti.PutC
	ld	a,'.'
	call	ti.PutC
	call	ti.PutC
	call	ti.PutC
	ld	hl,name
	ld	bc,maxsize
	push	bc
	call	create
addr:
	ld	hl,0
	push	hl
	ldir
	pop	hl
	pop	bc
	add	hl,bc
	ld	(addr + 1),hl
	push	hl
	call	ti.Arc_Unarc
	pop	hl
	ld	de,(ti.OSSize + 1)
	or	a,a
	sbc	hl,de
	jr	c,loop
	call	ti.HomeUp
	jp	ti.ClrScrn

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
	call	ti.Mov9ToOP1
	call	ti.ChkFindSym
	call	nc,ti.DelVarArc
	pop	hl
	call	ti.Mov9ToOP1
	pop	hl
	call	ti.CreateAppVar
	push	de
	call	ti.OP4ToOP1
	pop	de
	inc	de
	inc	de
	pop	bc
	ret

name:
	db	ti.AppVarObj, "ROMData"
num:
	db	"0"
segstr:
	db	"Dumping Segment ", 0


*/
