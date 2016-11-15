#include "utils.h"
#include "lcdpopout.h"
#include "ui_lcdpopout.h"

LCDPopout::LCDPopout(QtKeypadBridge *keypadBridge, QWidget *p) : QDialog(p), ui(new Ui::LCDPopout) {
    ui->setupUi(this);

    setFixedSize(width(), height());
    setWindowTitle("LCD Popout");

    connect(ui->lineAddress, &QLineEdit::returnPressed, this, &LCDPopout::changeAddress);
    connect(ui->lineBPP, &QLineEdit::returnPressed, this, &LCDPopout::changeBPP);
    connect(ui->checkBEBO, &QCheckBox::toggled, this, &LCDPopout::changeChecked);
    connect(ui->checkBEPO, &QCheckBox::toggled, this, &LCDPopout::changeChecked);
    connect(ui->checkBGR, &QCheckBox::toggled, this, &LCDPopout::changeChecked);

    ui->lcdWidget->installEventFilter(keypadBridge);

    ui->lcdWidget->setLCD(&(lcdState = lcd));
    ui->lcdWidget->setFocus();

    ui->lineBPP->setText(bpp2Str((lcdState.control >> 1) & 7));

    ui->checkBEPO->setChecked(lcdState.control & 0x400);
    ui->checkBEBO->setChecked(lcdState.control & 0x200);
    ui->checkBGR->setChecked(lcdState.control & 0x100);

    ui->lineAddress->setText(int2hex(lcdState.upcurr, 6));
}

LCDPopout::~LCDPopout() {
    delete ui;
}

void LCDPopout::closeEvent(QCloseEvent *e) {
    // someone should shoot me for using this
    delete this;
    e->accept();
}

void LCDPopout::changeAddress() {
    uint32_t line = static_cast<uint32_t>(hex2int(ui->lineAddress->text()));
    lcdState.upcurr = line;
    lcdState.upbase = line;
    ui->lcdWidget->setFocus();
}

void LCDPopout::changeBPP() {
    uint8_t bpp = 0;

    lcdState.control &= ~14;
    bpp = int2Bpp(ui->lineBPP->text().toInt());

    if (bpp == 6) { ui->lineBPP->setText("16"); }
    lcdState.control |= bpp << 1;
    ui->lcdWidget->setFocus();
}

void LCDPopout::changeChecked() {
    set_reset(ui->checkBEPO->isChecked(), 0x400, lcdState.control);
    set_reset(ui->checkBEBO->isChecked(), 0x200, lcdState.control);
    set_reset(ui->checkBGR->isChecked(), 0x100, lcdState.control);
    ui->lcdWidget->setFocus();
}
