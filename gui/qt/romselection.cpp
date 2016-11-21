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

static const uint8_t dumper_program[288] = {
    0x2A, 0x2A, 0x54, 0x49, 0x38, 0x33, 0x46, 0x2A, 0x1A, 0x0A, 0x00, 0x46,
    0x69, 0x6C, 0x65, 0x20, 0x67, 0x65, 0x6E, 0x65, 0x72, 0x61, 0x74, 0x65,
    0x64, 0x20, 0x62, 0x79, 0x20, 0x57, 0x61, 0x62, 0x62, 0x69, 0x74, 0x53,
    0x69, 0x67, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xE7, 0x00, 0x0D, 0x00, 0xD6, 0x00, 0x06,
    0x44, 0x55, 0x4D, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD6, 0x00,
    0xD4, 0x00, 0xEF, 0x7B, 0xCD, 0x14, 0x08, 0x02, 0x21, 0x39, 0xA9, 0xD1,
    0xCD, 0x20, 0x03, 0x02, 0xCD, 0x0C, 0x05, 0x02, 0xD4, 0x34, 0x14, 0x02,
    0x21, 0x39, 0xA9, 0xD1, 0xCD, 0x20, 0x03, 0x02, 0x21, 0x03, 0x00, 0x00,
    0xCD, 0x30, 0x13, 0x02, 0x13, 0x13, 0xEB, 0xED, 0x5B, 0x05, 0x01, 0x02,
    0xED, 0x1F, 0xCD, 0xC8, 0x02, 0x02, 0xCD, 0x48, 0x14, 0x02, 0x3E, 0x40,
    0x32, 0x41, 0xA9, 0xD1, 0xCD, 0x28, 0x08, 0x02, 0x21, 0x42, 0xA9, 0xD1,
    0xCD, 0xC0, 0x07, 0x02, 0x3A, 0x41, 0xA9, 0xD1, 0x3C, 0x32, 0x41, 0xA9,
    0xD1, 0xCD, 0xB8, 0x07, 0x02, 0x3E, 0x2E, 0xCD, 0xB8, 0x07, 0x02, 0x3E,
    0x2E, 0xCD, 0xB8, 0x07, 0x02, 0x3E, 0x2E, 0xCD, 0xB8, 0x07, 0x02, 0x21,
    0x39, 0xA9, 0xD1, 0xCD, 0x20, 0x03, 0x02, 0xCD, 0x0C, 0x05, 0x02, 0xD4,
    0x34, 0x14, 0x02, 0x21, 0x39, 0xA9, 0xD1, 0xCD, 0x20, 0x03, 0x02, 0x21,
    0xE9, 0xFF, 0x00, 0xE5, 0xE5, 0xCD, 0x30, 0x13, 0x02, 0xC1, 0x13, 0x13,
    0x21, 0x00, 0x00, 0x00, 0xE5, 0xED, 0xB0, 0xE1, 0xD1, 0x19, 0x22, 0x0E,
    0xA9, 0xD1, 0xCD, 0xC8, 0x02, 0x02, 0xCD, 0x48, 0x14, 0x02, 0x2A, 0x0E,
    0xA9, 0xD1, 0xED, 0x5B, 0x05, 0x01, 0x02, 0xB7, 0xED, 0x52, 0x38, 0x8C,
    0xCD, 0x28, 0x08, 0x02, 0xC3, 0x14, 0x08, 0x02, 0x15, 0x52, 0x4F, 0x4D,
    0x44, 0x61, 0x74, 0x61, 0x30, 0x44, 0x75, 0x6D, 0x70, 0x69, 0x6E, 0x67,
    0x20, 0x53, 0x65, 0x67, 0x6D, 0x65, 0x6E, 0x74, 0x20, 0x00, 0xE8, 0x4D
};

RomSelection::RomSelection(QWidget *p) : QDialog(p), ui(new Ui::RomSelection) {
    ui->setupUi(this);

    setFixedSize(width(), height());
    setWindowModality(Qt::ApplicationModal);
    setWindowFlags(Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint );

    connect(ui->buttonCancel, &QPushButton::clicked, this, &RomSelection::close);
    connect(ui->rompath, &QLineEdit::textChanged, this, &RomSelection::checkInput);
    connect(ui->buttonNext, &QPushButton::clicked, this, &RomSelection::nextPageOne);
    connect(ui->buttonNext2, &QPushButton::clicked, this, &RomSelection::nextPageTwo);
    connect(ui->buttonDump, &QPushButton::clicked, this, &RomSelection::saveDumpProgram);
    connect(ui->buttonBrowseROM, &QPushButton::clicked, this, &RomSelection::browseForROM);
    connect(ui->buttonBrowseSave, &QPushButton::clicked, this, &RomSelection::saveROMImage);
    connect(ui->radioBrowse, &QRadioButton::clicked, this, &RomSelection::openROMImageSelected);
    connect(ui->buttonOpenSegments, &QPushButton::clicked, this, &RomSelection::openROMSegments);
    connect(ui->radioCreate, &QRadioButton::clicked, this, &RomSelection::createROMImageSelected);
    connect(ui->buttonBack1, &QRadioButton::clicked, this, &RomSelection::backPage);
    connect(ui->buttonBack2, &QRadioButton::clicked, this, &RomSelection::backPage);

    ui->stackedWidget->setCurrentIndex(0);

    ui->versionLabel->setText(ui->versionLabel->text() + QStringLiteral(CEMU_VERSION));
}

RomSelection::~RomSelection() {
    free(romArray);
    delete ui;
}

bool RomSelection::checkImageSize(const char *filename) {
    size_t s;

    FILE* romfile = fopen_utf8(filename, "r+b");

    if (!romfile) {
        return false;
    }

    fseek(romfile, 0, SEEK_END);
    s = ftell(romfile);

    fclose(romfile);

    return s == ROM_SIZE;
}

void RomSelection::checkInput(const QString &path) {
    ui->buttonNext->setEnabled(fileExists(path.toStdString()));
}

void RomSelection::createROMImageSelected() {
    this->ui->rompath->setEnabled(false);
    this->ui->buttonBrowseROM->setEnabled(false);
    this->ui->buttonNext->setEnabled(true);
    this->ui->buttonNext->setText(tr("Continue"));
}

void RomSelection::openROMImageSelected() {
    ui->rompath->setEnabled(true);
    ui->buttonBrowseROM->setEnabled(true);
    ui->buttonNext->setText(tr("Finish"));
    checkInput(ui->rompath->text());
}

void RomSelection::browseForROM() {
  ui->rompath->setText(
      QFileDialog::getOpenFileName(this, tr("Open ROM file"),"",
      tr("Known Types (*.rom *.sav);;ROM Image (*.rom);;Saved Image (*.sav);;All Files (*.*)")));
}

void RomSelection::nextPageOne() {
    if (ui->radioBrowse->isChecked()) {
        if (checkImageSize(ui->rompath->text().toStdString().c_str()) == false) {
            QMessageBox::critical(this, tr("Invalid ROM image"), tr("You have selected an invalid ROM image."));
            return;
        }
        rom = ui->rompath->text();
        close();
    } else {
        ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex() + 1);
    }
}

void RomSelection::nextPageTwo() {
    romArray = reinterpret_cast<uint8_t*>(malloc(ROM_SIZE));
    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex() + 1);
}

void RomSelection::backPage() {
    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex() - 1);
}

void RomSelection::openROMSegments() {
    FILE* seg;
    QFileDialog dialog(this);
    QStringList fileNames;

    uint8_t buf[10];
    uint16_t u16 = 0;

    bool config = false;
    int i, segint;

    dialog.setDirectory(currentDir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilter(QStringLiteral("ROMData (*.8xv)"));

    if (!dialog.exec()) {
        return;
    }

    fileNames = dialog.selectedFiles();
    ui->progressBar->setEnabled(false);

    for (i = 0; i < fileNames.size(); i++) {
        seg = fopen_utf8(fileNames.at(i).toStdString().c_str(), "rb");
        if (!seg)                                             goto _err;
        if (fseek(seg, 0x3C, SEEK_SET))                       goto _err;
        if (fread(buf, 1, 8, seg) != 8)                       goto _err;

        if (memcmp(buf, "ROMData", 7)) {
            goto _err;
        } else {
            if (buf[7] == '0') {
                if (fseek(seg, 0x4A, 0))                       goto _err;
                if (fread(buf, 1, 3, seg) != 3)                goto _err;

                imageSize = static_cast<uint32_t>(buf[0] << 0) |
                            static_cast<uint32_t>(buf[1] << 8) |
                            static_cast<uint32_t>(buf[2] << 16);

                numROMSegments = (imageSize/SEG_SIZE) + 1;
                ui->progressBar->setMaximum(numROMSegments);
                ui->progressBar->setEnabled((config = true));
            } else {
                if (fseek(seg,0x48,0))                          goto _err;
                if (fread(&u16, sizeof(u16), 1,seg) != 1)   goto _err;
                if (SEG_SIZE == u16) {
                    segint = buf[7] - 'A';
                    if (segmentFilledStatus[segint] == false) {
                        segmentFilledStatus[segint] = true;
                        if (fread(romArray + (SEG_SIZE * segint), SEG_SIZE, 1, seg) != 1) {
                            goto _err;
                        }
                        ui->progressBar->setValue(ui->progressBar->value() + 1);
                    }
                }
            }
        }

        fclose(seg);
    }
    if (ui->progressBar->value() == numROMSegments && config) {
        ui->buttonBrowseSave->setEnabled(true);
    }
    return;

_err:
    QMessageBox::warning(this, tr("Invalid"), tr("Invalid ROM segment: ") + fileNames.at(i));
    fclose(seg);
    return;
}

void RomSelection::saveDumpProgram() {
    FILE* save_program;
    QString filename = QFileDialog::getSaveFileName(this, tr("Save ROM Dumper Program"), QString(), tr("ROM Dumper (*.8xp)"));
    if (filename.isEmpty()) {
        return;
    }

    save_program = fopen_utf8(filename.toStdString().c_str(), "w+b");

    if (save_program) {
        fwrite(dumper_program, 1, sizeof(dumper_program), save_program);
        fclose(save_program);
    }
}

void RomSelection::saveROMImage() {
    FILE* saveRom;
    QString filename = QFileDialog::getSaveFileName(this, tr("Save ROM Image"), QString(), tr("ROM Image (*.rom)"));
    if  (filename.isEmpty()) {
        return;
    }

    rom = filename;
    saveRom = fopen_utf8(rom.toStdString().c_str(), "w+b");

    if (saveRom) {
        /* Make sure the only thing in the rom is the boot+os */
        memset(&romArray[imageSize], 0xFF, ROM_SIZE - imageSize - 1);

        fwrite(romArray, 1, ROM_SIZE, saveRom);

        fclose(saveRom);
    }

    close();
}

QString RomSelection::romPath() {
    return rom;
}


/*!
 * ROM dumping program
 *
#include "ti84pce.inc"

.assume ADL=1
.db $EF,$7B
.org usermem

    call	_clrscrn
    ld	hl,_data_name
    call	_mov9toop1
    call	_chkfindsym
    call	nc,_delvararc
    ld	hl,_data_name
    call	_mov9toop1
    ld	hl,3
    call	_createappvar
    inc	de
    inc	de
    ex	de,hl
    ld	de,(_ossize+1)
    ld	(hl),de
    call	_op4toop1
    call	_arc_unarc
    ld	a,'A'-1
    ld	(_data_num),a
_dump_loop:
    call	_homeup
    ld	hl,_dump_string
    call	_puts
    ld	a,(_data_num)
    inc	a
    ld	(_data_num),a
    call	_putc
    ld	a,'.'
    call	_putc
    ld	a,'.'
    call	_putc
    ld	a,'.'
    call	_putc
    ld	hl,_data_name
    call	_mov9toop1
    call	_chkfindsym
    call	nc,_delvararc
    ld	hl,_data_name
    call	_mov9toop1
    ld	hl,$FFFF-22
_notend:
    push	hl
    push	hl
    call	_createappvar
    pop	bc
    inc	de
    inc	de
_currentaddr =$+1
    ld	hl,0
    push	hl
    ldir
    pop	hl
    pop	de
    add	hl,de
    ld	(_currentaddr),hl
    call	_op4toop1
    call	_arc_unarc
    ld	hl,(_currentaddr)
    ld	de,(_ossize+1)
    or	a,a
    sbc	hl,de
    jr	c,_dump_loop
    call	_homeup
    jp	_clrscrn


_data_name:
 .db appvarobj,"ROMData"
_data_num:
 .db '0'

_dump_string:
 .db "Dumping Segment ",0
 *
 */
