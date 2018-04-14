#include "visualizerwidget.h"
#include "keypad/keypadwidget.h"
#include "utils.h"

#include <QtWidgets/QPushButton>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QAction>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QMenu>

VisualizerWidget::VisualizerWidget(QWidget *parent, const QString &config) : QWidget{parent} {
    QIcon iconLcd(QPixmap(QStringLiteral(":/icons/resources/icons/lcd.png")));
    QIcon iconDebug(QPixmap(QStringLiteral(":/icons/resources/icons/debugger.png")));
    QIcon iconInfo(QPixmap(QStringLiteral(":/icons/resources/icons/info.png")));

    m_group = new QGroupBox(this);
    QHBoxLayout *hlayout = new QHBoxLayout(m_group);

    m_config = new QLineEdit(m_group);
    m_config->setFocusPolicy(Qt::ClickFocus);
    m_btnLcd = new QToolButton(m_group);
    m_btnDebug = new QToolButton(m_group);
    m_btnInfo = new QToolButton(m_group);

    m_btnLcd->setIcon(iconLcd);
    m_btnDebug->setIcon(iconDebug);
    m_btnInfo->setIcon(iconInfo);

    hlayout->addWidget(m_config);
    hlayout->addWidget(m_btnLcd);
    hlayout->addWidget(m_btnDebug);
    hlayout->addWidget(m_btnInfo);

    m_view = new VisualizerDisplayWidget(this);
    m_view->setLayoutDirection(Qt::LeftToRight);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->addWidget(m_group);
    vlayout->addWidget(m_view);
    vlayout->addStretch(1);
    setLayout(vlayout);

    connect(m_config, &QLineEdit::returnPressed, this, &VisualizerWidget::stringToView);
    connect(m_btnDebug, &QPushButton::clicked, this, &VisualizerWidget::stringToView);
    connect(m_btnLcd, &QPushButton::clicked, this, &VisualizerWidget::showPresets);
    connect(m_btnInfo, &QPushButton::clicked, this, &VisualizerWidget::showHelp);

    if (config.isEmpty()) {
        setDefaultView();
    } else {
        m_config->setText(config);
        stringToView();
    }

    translate();
}

VisualizerWidget::~VisualizerWidget() { }

void VisualizerWidget::translate() {
    m_group->setTitle(tr("Settings"));
    m_btnLcd->setToolTip(tr("Reset to default"));
    m_btnDebug->setToolTip(tr("Apply changes"));
    m_btnInfo->setToolTip(tr("Help"));
}

void VisualizerWidget::showPresets() {
    QString preset_1 = tr("Default");
    QString preset_2 = tr("8bpp Buffer 1");
    QString preset_3 = tr("8bpp Buffer 2");
    QString preset_4 = tr("Palette");

    QMenu menu;
    menu.addAction(preset_1);
    menu.addAction(preset_2);
    menu.addAction(preset_3);
    menu.addAction(preset_4);

    QAction *item = menu.exec(mapToGlobal(m_btnLcd->pos()));
    if (item) {
        if (item->text() == preset_1) {
            setDefaultView();
        } else
        if (item->text() == preset_2) {
            m_config->setText("d40000,320x240,8bpp,bgr");
            stringToView();
        } else
        if (item->text() == preset_3) {
            m_config->setText("d52c00,320x240,8bpp,bgr");
            stringToView();
        } else
        if (item->text() == preset_4) {
            m_config->setText("e30200,64x8,8bpp,bgr,500%");
            stringToView();
        }
    }
}

void VisualizerWidget::showHelp() {
    QMessageBox::information(this, m_btnInfo->toolTip(),
                             tr("Use the format string to change the visual display. "
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

void VisualizerWidget::stringToView() {
    QStringList string = m_config->text().split(',');
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

void VisualizerWidget::setDefaultView() {
    m_width = LCD_WIDTH;
    m_height = LCD_HEIGHT;
    m_base = lcd.upbase;
    m_control = lcd.control;
    m_scale = 100.0;
    viewToString();
}

void VisualizerWidget::viewToString() {
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

    m_config->setText(m_setup.join(","));

    float s = m_scale / 100.0;
    float w = m_width * s;
    float h = m_height * s;

    uint32_t *data;
    uint32_t *data_end;

    lcd_setptrs(&data, &data_end, m_width, m_height, m_base, m_control, false);

    m_view->setFixedSize(w, h);
    m_view->setRefreshRate(m_rate);
    m_view->setConfig(m_height, m_width, m_base, m_control, data, data_end);
    adjustSize();

    emit configChanged();
}

QString VisualizerWidget::getConfig() {
    return m_config->text();
}
