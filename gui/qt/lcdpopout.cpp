#include "lcdpopout.h"
#include "ui_lcdpopout.h"

LCDPopout::LCDPopout(QtKeypadBridge *keypadBridge, QWidget *p) : QDialog(p),ui(new Ui::LCDPopout) {
    ui->setupUi(this);

    setFixedSize(width(), height());
    setWindowTitle("LCD Popout");

    connect(ui->lineAddress, &QLineEdit::returnPressed, this, &LCDPopout::changeAddress);
    connect(ui->lineBPP, &QLineEdit::returnPressed, this, &LCDPopout::changeBPP);
    connect(ui->checkBEBO, &QCheckBox::toggled, this, &LCDPopout::changeChecked);
    connect(ui->checkBEPO, &QCheckBox::toggled, this, &LCDPopout::changeChecked);
    connect(ui->checkBGR, &QCheckBox::toggled, this, &LCDPopout::changeChecked);

    ui->lcdWidget->installEventFilter(keypadBridge);

    lcdState = lcd;
    ui->lcdWidget->setLCD(&lcdState);
    ui->lcdWidget->setFocus();

    QString tmp;

    switch((lcdState.control>>1)&7) {
        case 0:
            tmp = "01"; break;
        case 1:
            tmp = "02"; break;
        case 2:
            tmp = "04"; break;
        case 3:
            tmp = "08"; break;
        case 4:
            tmp = "16"; break;
        case 5:
            tmp = "24"; break;
        case 6:
            tmp = "16"; break;
        case 7:
            tmp = "12"; break;
    }

    ui->lineBPP->setText(tmp);

    ui->checkBEPO->setChecked(lcdState.control & 0x400);
    ui->checkBEBO->setChecked(lcdState.control & 0x200);
    ui->checkBGR->setChecked(lcdState.control & 0x100);

    ui->lineAddress->setText(QString::number(lcdState.upcurr, 16).rightJustified(6, '0', true).toUpper());
}

LCDPopout::~LCDPopout() {
    delete ui;
}

void LCDPopout::changeAddress() {
    uint32_t line = static_cast<uint32_t>(std::stoi(ui->lineAddress->text().toStdString(), nullptr, 16));
    if (line < 0xD00000) {
        ui->lineAddress->setText("D00000");
        line = 0xD00000;
    }
    lcdState.upcurr = line;
    lcdState.upbase = line;
    ui->lcdWidget->setFocus();
}

void LCDPopout::changeBPP() {
    uint8_t bpp = 0;

    lcdState.control &= ~14;
    switch(ui->lineBPP->text().toInt()) {
        case 1:
            bpp = 0; break;
        case 2:
            bpp = 1; break;
        case 4:
            bpp = 2; break;
        case 8:
            bpp = 3; break;
        case 24:
            bpp = 5; break;
        case 16:
            bpp = 6; break;
        case 12:
            bpp = 7; break;
        default:
            ui->lineBPP->setText("16");
            bpp = 6; break;
    }
    lcdState.control |= bpp<<1;
    ui->lcdWidget->setFocus();
}

void LCDPopout::changeChecked() {
    if (ui->checkBEPO->isChecked()) {
        lcdState.control |= 0x400;
    } else {
        lcdState.control &= ~0x400;
    }
    if (ui->checkBEBO->isChecked()) {
        lcdState.control |= 0x200;
    } else {
        lcdState.control &= ~0x200;
    }
    if (ui->checkBGR->isChecked()) {
        lcdState.control |= 0x100;
    } else {
        lcdState.control &= ~0x100;
    }
    ui->lcdWidget->setFocus();
}

