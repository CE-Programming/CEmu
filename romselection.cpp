#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include "romselection.h"
#include "ui_romselection.h"

#include "core/os/os.h"

std::string romImagePath;

static const size_t rom_size = 1024*1024*4;
static const size_t rom_segment_size = 0xFFE9;
static int num_rom_segments = 11;
static const uint8_t dumper_program[214] = {
    0x2A, 0x2A, 0x54, 0x49, 0x38, 0x33, 0x46, 0x2A, 0x1A, 0x0A, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x9D, 0x00, 0x0D, 0x00, 0x8C, 0x00, 0x06,
    0x44, 0x55, 0x4D, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8C, 0x00,
    0x8A, 0x00, 0xEF, 0x7B, 0xCD, 0x14, 0x08, 0x02, 0xCD, 0x28, 0x08, 0x02,
    0x21, 0xF8, 0xA8, 0xD1, 0xCD, 0xC0, 0x07, 0x02, 0x3A, 0xF7, 0xA8, 0xD1,
    0x3C, 0x32, 0xF7, 0xA8, 0xD1, 0xCD, 0xB8, 0x07, 0x02, 0x21, 0xEF, 0xA8,
    0xD1, 0xCD, 0x20, 0x03, 0x02, 0xCD, 0x0C, 0x05, 0x02, 0xD4, 0x34, 0x14,
    0x02, 0x21, 0xEF, 0xA8, 0xD1, 0xCD, 0x20, 0x03, 0x02, 0x21, 0xE9, 0xFF,
    0x00, 0xE5, 0xE5, 0xCD, 0x30, 0x13, 0x02, 0xC1, 0x13, 0x13, 0x2A, 0xEC,
    0xA8, 0xD1, 0xE5, 0xED, 0xB0, 0xE1, 0xD1, 0x19, 0x22, 0xEC, 0xA8, 0xD1,
    0xCD, 0xC8, 0x02, 0x02, 0xCD, 0x48, 0x14, 0x02, 0x2A, 0xEC, 0xA8, 0xD1,
    0xED, 0x5B, 0x05, 0x01, 0x02, 0xB7, 0xED, 0x52, 0x38, 0x9E, 0xCD, 0x14,
    0x08, 0x02, 0xC9, 0x00, 0x00, 0x00, 0x15, 0x52, 0x4F, 0x4D, 0x44, 0x61,
    0x74, 0x61, 0x40, 0x44, 0x75, 0x6D, 0x70, 0x69, 0x6E, 0x67, 0x20, 0x53,
    0x65, 0x67, 0x6D, 0x65, 0x6E, 0x74, 0x20, 0x00, 0x67, 0x3D
};

#define BIGENDI(data) ((data>>8)&0xF)|((data&0xF)<<8)

RomSelection::RomSelection(QWidget *p) : QDialog(p), ui(new Ui::RomSelection) {
    ui->setupUi(this);
    setWindowModality(Qt::ApplicationModal);
    setWindowFlags(Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint );

    connect(ui->rompath, SIGNAL(textChanged(QString)), this, SLOT(checkInput(QString)));
    ui->stackedWidget->setCurrentIndex(0);
    ui->progressBar->setMaximum(num_rom_segments);
}

bool fileExists(const QString &path) {
    QFileInfo checkFile(path);
    return (checkFile.exists() && checkFile.isFile());
}

RomSelection::~RomSelection() {
    if (rom_array != nullptr) { free(rom_array); }
    delete ui;
}

bool RomSelection::flash_open(const char *filename) {
    size_t s;

    FILE* rom_read = fopen_utf8(filename, "r+b");

    if (!rom_read) {
        return 0;
    }
    fseek(rom_read, 0, SEEK_END);
    s = ftell(rom_read);

    fclose(rom_read);

    return (s == 4*1024*1024);
}

void RomSelection::checkInput(const QString &path) {
    ui->next->setEnabled(fileExists(path));
}

void RomSelection::on_create_sel_clicked() {
    this->ui->rompath->setEnabled(false);
    this->ui->browse->setEnabled(false);
    this->ui->next->setEnabled(true);
    this->ui->next->setText(QString("Continue"));
}

void RomSelection::on_cancel_clicked() {
    romImagePath.clear();
    close();
}

void RomSelection::on_browse_sel_clicked() {
    ui->rompath->setEnabled(true);
    ui->browse->setEnabled(true);
    ui->next->setText(QString("Finish"));
    checkInput(ui->rompath->text());     // make sure the text is still valid
}

void RomSelection::on_browse_clicked() {
  ui->rompath->setText(
      QFileDialog::getOpenFileName(this, tr("Open ROM file"),"",
      tr("Known Types (*.rom *.sav);;ROM Image (*.rom);;Saved Image (*.sav);;All Files (*.*)")));
}

void RomSelection::on_next_clicked() {
    if (ui->browse_sel->isChecked()) {
        if (flash_open(ui->rompath->text().toLatin1()) == false) {
            QMessageBox::critical(this, trUtf8("Invalid ROM image"), trUtf8("You have selected an invalid ROM image."));
            return;
        }
        romImagePath = ui->rompath->text().toLatin1().toStdString();
        close();
    } else {
        ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex()+1);
    }
}

void RomSelection::on_mergeButton_clicked() {
    FILE* read_segment;
    QFileDialog dialog(this);
    QStringList fileNames;

    /* allocate 65Kb for each chunck */
    uint8_t tmp_buf[10];
    int tmpint;

    dialog.setDirectory(QDir::homePath());
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilter(trUtf8("ROMData (*.8xv)"));
    if (dialog.exec()) {
        fileNames = dialog.selectedFiles();
    } else {
        return;
    }

    for (int i = 0; i < 20; i++) {
        segment_filled[i] = false;
    }

    /* As of right now, there are only 11 data segements that need to be loaded. */
    /* Luckily if more are needed, this code can handle it. */
    /* Data segments go from A-K, so let's be sure we load them all, and they are valid */
    for (int i = 0; i < fileNames.size(); i++) {
        read_segment = fopen_utf8(fileNames.at(i).toStdString().c_str(), "rb");
        if (read_segment != NULL) {
            /* make sure the name is right... */
            fseek(read_segment,0x3C,0);
            fread(&tmp_buf,1,8,read_segment);

            fseek(read_segment,0x48,0);

            if (tmp_buf[0] == 'R' && tmp_buf[1] == 'O' && tmp_buf[2] == 'M' &&
               tmp_buf[3] == 'D' && tmp_buf[4] == 'a' && tmp_buf[5] == 't' && tmp_buf[6] == 'a') {

                fread(&tmp_buf,1,2,read_segment);
                if (tmp_buf[0] == 0xE9 && tmp_buf[1] == 0xFF) {

                /* First one is 'A' */
                tmpint = tmp_buf[7]-'A';
                if (segment_filled[tmpint] == false) {
                    segment_filled[tmpint] = true;

                    fread(rom_array+(tmpint*rom_segment_size),1,0xFFE9,read_segment);
                    if (tmpint > num_rom_segments) {
                        ui->progressBar->setMaximum(tmpint);
                        num_rom_segments = tmpint;
                    }
                    ui->progressBar->setValue(ui->progressBar->value()+1);
                 }
              } else {
                    QMessageBox::warning(this, tr("Invalid"), tr("Invalid ROM segment."));
              }
          } else {
                QMessageBox::warning(this, tr("Invalid"), tr("Invalid ROM segment name."));
          }
      }
      fclose(read_segment);
    }
    if (ui->progressBar->value() == num_rom_segments) {
        ui->hiddenLabel_1->setVisible(true);
        ui->hiddenLabel_2->setVisible(true);
        ui->romsaveBrowse->setVisible(true);
    }
}

void RomSelection::on_dumpButton_clicked() {
    FILE* save_program;
    QString filename = QFileDialog::getSaveFileName(this, tr("Save ROM Dumper Program"), QString(), tr("ROM Dumper (*.8xp)"));
    if (filename.isEmpty()) {
        return;
    }

    save_program = fopen_utf8(filename.toStdString().c_str(), "w+b");

    if(save_program) {
        fwrite(dumper_program, 1, 214, save_program);
    }
    fclose(save_program);
}

void RomSelection::on_nextButton_2_clicked() {
    /* initialize the rom data */
    rom_array = (uint8_t*)malloc(rom_size);
    memset(rom_array, 0xFF, rom_size);
    ui->hiddenLabel_1->setVisible(false);
    ui->hiddenLabel_2->setVisible(false);
    ui->romsaveBrowse->setVisible(false);
    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex()+1);
}

void RomSelection::on_romsaveBrowse_clicked() {
    FILE* save_rom;
    QString filename = QFileDialog::getSaveFileName(this, tr("Save ROM Image"), QString(), tr("ROM Image (*.rom)"));
    if  (filename.isEmpty()) {
        return;
    }

    romImagePath = filename.toStdString();

    save_rom = fopen_utf8(romImagePath.c_str(), "w+b");

    if (save_rom) {
        /* make sure the only thing in the rom is the boot+os */
        int os_end = (rom_array[0x20105]) | (rom_array[0x20106]<<8) | (rom_array[0x20107]<<16);
        memset(&rom_array[os_end],0xFF,rom_size-os_end-1);

        /* Set the specifed flag */
        rom_array[0x7E] = 0xFF;
        if (ui->toggleFlag->isChecked()) { rom_array[0x7E] = 0xFE; }
        if (ui->toggleFlash->isChecked()) { rom_array[0x7E] = 0x80; }

        fwrite(rom_array, 1, rom_size, save_rom);
    }
    fclose(save_rom);

    close();
}

/* ROM Dumper program */

/*
#include "ti84pce.inc"

.assume ADL=1
.db $EF,$7B
.org usermem

 call _clrscrn
_dump_loop:
 call _homeup
 ld hl,_dump_string
 call _puts
 ld a,(_data_num)
 inc a
 ld (_data_num),a
 call _putc
 ld hl,_data_name
 call _mov9toop1
 call _chkfindsym
 call nc,_delvararc
 ld hl,_data_name
 call _mov9toop1
 ld hl,$FFFF-22
_notend:
 push hl
  push hl
   call _createappvar
  pop bc
  inc de
  inc de
  ld hl,(_currentaddr)
  push hl
   ldir
  pop hl
 pop de
 add hl,de
 ld (_currentaddr),hl
 call _op4toop1
 call _arc_unarc
 ld hl,(_currentaddr)
 ld de,(_ossize+1)
 or a,a
 sbc hl,de
 jr c,_dump_loop
 call _clrscrn
 ret

_currentaddr:
 .dl 0

_data_name:
 .db appvarobj,"ROMData"
_data_num:
 .db 'A'-1

_dump_string:
 .db "Dumping Segment ",0
*/
