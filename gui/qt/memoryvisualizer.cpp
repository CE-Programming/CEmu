#include "memoryvisualizer.h"
#include "ui_memoryvisualizer.h"
#include "keypad/keypadwidget.h"
#include "utils.h"

#include <QtWidgets/QPushButton>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QAction>

MemoryVisualizer::MemoryVisualizer(QWidget *parent) : QDialog{parent}, ui(new Ui::MemoryVisualizer) {
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

    m_rate = 30; m_scale = 100;

    set_reset(false, 0x400, m_control);
    set_reset(false, 0x200, m_control);
    set_reset(false, 0x100, m_control);

    foreach (QString str, string) {
        str = str.toLower();
        if (!str.compare(QLatin1Literal("bepo"), Qt::CaseInsensitive)) {
            set_reset(true, 0x400, m_control);
        }
        if (!str.compare(QLatin1Literal("bebo"), Qt::CaseInsensitive)) {
            set_reset(true, 0x200, m_control);
        }
        if (!str.compare(QLatin1Literal("bgr"), Qt::CaseInsensitive)) {
            set_reset(true, 0x100, m_control);
        }
        if (str.contains('x')) {
            QStringList wh = str.split('x');
            if (wh.size() == 2) {
                m_width = wh.at(0).toUInt();
                m_height = wh.at(1).toUInt();
            }
        }
        if (str.endsWith('%')) {
            str.remove('%');
            m_scale = str.toInt();
        }
        if (str.length() == 8 && str.at(0) == '0' && str.at(1) == 'x') {
            str.remove(0, 2);
        }
        if (str.length() == 7 && str.at(0) == '$') {
            str.remove(0, 1);
        }
        if (hex_reg.exactMatch(str)) {
            m_base = str.toUInt(Q_NULLPTR, 16);
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
                m_control &= ~14;
                m_control |= bpp << 1;
            }
        }
        if (fps_reg.exactMatch(str)) {
            str.chop(3);
            m_rate = str.toUInt();
            if (m_rate < 1 || m_rate > 120) { m_rate = 30; }
        }
    }

    viewToString();
}

void MemoryVisualizer::setDefaultView() {
    m_width = LCD_WIDTH;
    m_height = LCD_HEIGHT;
    m_base = lcd.upbase;
    m_control = lcd.control;
    viewToString();
}

void MemoryVisualizer::viewToString() {
    QString bpp;

    switch((m_control >> 1) & 7) {
        case 0: bpp = QStringLiteral("1"); break;
        case 1: bpp = QStringLiteral("2"); break;
        case 2: bpp = QStringLiteral("4"); break;
        case 3: bpp = QStringLiteral("8"); break;
        case 4: bpp = QStringLiteral("161555"); break;
        case 5: bpp = QStringLiteral("24"); break;
        case 6: bpp = QStringLiteral("16"); break;
        case 7: bpp = QStringLiteral("12"); break;
        default: break;
    }

    m_setup.clear();
    m_setup.append(int2hex(m_base, 6).toLower());
    m_setup.append(QString::number(m_width) + QStringLiteral("x") + QString::number(m_height));
    m_setup.append(bpp + QStringLiteral("bpp"));
    if (m_control & 0x400) { m_setup.append(QStringLiteral("bepo")); }
    if (m_control & 0x200) { m_setup.append(QStringLiteral("bebo")); }
    if (m_control & 0x100) { m_setup.append(QStringLiteral("bgr")); }
    if (m_scale != 100) { m_setup.append(QString::number(m_scale) + QStringLiteral("%")); }
    if (m_rate != 30) { m_setup.append(QString::number(m_rate) + QStringLiteral("fps")); }

    ui->edit->setText(m_setup.join(QStringLiteral(",")));

    float s = m_scale / 100.0;
    float w = m_width * s;
    float h = m_height * s;

    uint32_t *data;
    uint32_t *data_end;

    lcd_setptrs(&data, &data_end, m_width, m_height, m_base, m_control, false);

    ui->view->setFixedSize(w, h);
    ui->view->setRefreshRate(m_rate);
    ui->view->setConfig(m_height, m_width, m_base, m_control, data, data_end);
    adjustSize();
}
