#include "memoryvisualizer.h"
#include "ui_memoryvisualizer.h"

#include <QtWidgets/QPushButton>
#include <QtWidgets/QMessageBox>
#include <QAction>

#include "utils.h"
#include "keypad/keypadwidget.h"

MemoryVisualizer::MemoryVisualizer(QWidget *p) : QDialog(p), ui(new Ui::MemoryVisualizer) {
    ui->setupUi(this);

    connect(ui->edit, &QLineEdit::returnPressed, this, &MemoryVisualizer::stringToView);
    connect(ui->buttonSubmit, &QPushButton::clicked, this, &MemoryVisualizer::stringToView);
    connect(ui->buttonDefault, &QPushButton::clicked, this, &MemoryVisualizer::setDefaultView);
    connect(ui->buttonHelp, &QPushButton::clicked, this, &MemoryVisualizer::showHelp);

    setDefaultView();
}

MemoryVisualizer::~MemoryVisualizer() {
    delete ui;
}

void MemoryVisualizer::showHelp() {
    QMessageBox::information(this, tr("Help"), tr("Use the format string to change the visual display. "
                                                  "Press the screen button to grab the current LCD state. Examples:\n\n"
                                                  " 'd40000'\t6 hex digits specify start address\n"
                                                  " '320x240'\tSpecify width and height of data\n"
                                                  " '16bpp'\tChange the bits per pixel\n"
                                                  " 'bebo'\tSet BEBO LCD bit\n"
                                                  " 'bepo'\tSet BEPO LCD bit\n"
                                                  " 'bgr'\tSet BGR LCD bit\n"
                                                  " '200%'\tChange scale of displayed image\n"
                                                  " '40fps'\tChange refresh rate of displayed image"));
}

void MemoryVisualizer::keyPressEvent(QKeyEvent *e) {
    if(e->key() != Qt::Key_Escape) {
        QDialog::keyPressEvent(e);
    }
    e->accept();
}

void MemoryVisualizer::stringToView() {
    QStringList string = ui->edit->text().split(',');
    QRegExp hex_reg("^[0-9A-F]{6}$", Qt::CaseInsensitive);
    QRegExp bpp_reg("^\\d{1,6}bpp$", Qt::CaseInsensitive);
    QRegExp fps_reg("^\\d+fps$", Qt::CaseInsensitive);

    rate = 30; scale = 100;

    set_reset(false, 0x400, control);
    set_reset(false, 0x200, control);
    set_reset(false, 0x100, control);

    foreach (QString str, string) {
        str = str.toLower();
        if (!str.compare(QLatin1Literal("bepo"), Qt::CaseInsensitive)) {
            set_reset(true, 0x400, control);
        }
        if (!str.compare(QLatin1Literal("bebo"), Qt::CaseInsensitive)) {
            set_reset(true, 0x200, control);
        }
        if (!str.compare(QLatin1Literal("bgr"), Qt::CaseInsensitive)) {
            set_reset(true, 0x100, control);
        }
        if (str.contains('x')) {
            QStringList wh = str.split('x');
            if (wh.size() == 2) {
                width = wh.at(0).toUInt();
                height = wh.at(1).toUInt();
            }
        }
        if (str.endsWith('%')) {
            str.remove('%');
            scale = str.toInt();
        }
        if (str.length() == 8 && str.at(0) == '0' && str.at(1) == 'x') {
            str.remove(0, 2);
        }
        if (str.length() == 7 && str.at(0) == '$') {
            str.remove(0, 1);
        }
        if (hex_reg.exactMatch(str)) {
            upbase = str.toUInt(Q_NULLPTR, 16);
        }
        if (bpp_reg.exactMatch(str)) {
            str.chop(3);
            uint8_t bpp;
            switch(str.toUInt()) {
                case 1: bpp = 0; break;
                case 2: bpp = 1; break;
                case 4: bpp = 2; break;
                case 8: bpp = 3; break;
                case 161555: bpp = 4; break;
                case 24: bpp = 5; break;
                case 16: bpp = 6; break;
                case 12: bpp = 7; break;
                default: bpp = 255; break;
            }
            if (bpp != 255) {
                control &= ~14;
                control |= bpp << 1;
            }
        }
        if (fps_reg.exactMatch(str)) {
            str.chop(3);
            rate = str.toUInt();
            if (rate < 1 || rate > 120) { rate = 30; }
        }
    }

    viewToString();
}

void MemoryVisualizer::setDefaultView() {
    width = LCD_WIDTH;
    height = LCD_HEIGHT;
    upbase = lcd.upbase;
    control = lcd.control;
    viewToString();
}

void MemoryVisualizer::viewToString() {
    QString bpp;

    switch((control >> 1) & 7) {
        case 0: bpp = "1"; break;
        case 1: bpp = "2"; break;
        case 2: bpp = "4"; break;
        case 3: bpp = "8"; break;
        case 4: bpp = "161555"; break;
        case 5: bpp = "24"; break;
        case 6: bpp = "16"; break;
        case 7: bpp = "12"; break;
        default: break;
    }

    setup.clear();
    setup.append(int2hex(upbase, 6).toLower());
    setup.append(QString::number(width) + "x" + QString::number(height));
    setup.append(bpp + QLatin1Literal("bpp"));
    if (control & 0x400) { setup.append(QLatin1Literal("bepo")); }
    if (control & 0x200) { setup.append(QLatin1Literal("bebo")); }
    if (control & 0x100) { setup.append(QLatin1Literal("bgr")); }
    if (scale != 100) { setup.append(QString::number(scale)+QLatin1Literal("%")); }
    if (rate != 30) { setup.append(QString::number(rate)+QLatin1Literal("fps")); }

    ui->edit->setText(setup.join(","));

    float s = scale / 100.0;
    float w = width * s;
    float h = height * s;

    uint32_t *data;
    uint32_t *data_end;

    lcd_setptrs(&data, &data_end, width, height, upbase, control, false);

    ui->view->setFixedSize(w, h);
    ui->view->setRefreshRate(rate);
    ui->view->setConfig(height, width, upbase, control, data, data_end);
    adjustSize();
}
