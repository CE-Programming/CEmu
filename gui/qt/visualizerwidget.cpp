#include "visualizerwidget.h"
#include "keypad/keypadwidget.h"
#include "utils.h"

#include <QtWidgets/QPushButton>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QMenu>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QComboBox>
#include <QAction> /* Different module in Qt5 vs Qt6 */

VisualizerWidget::VisualizerWidget(QWidget *parent, const QString &config) : QWidget{parent} {
    QIcon iconRefresh(QPixmap(QStringLiteral(":/icons/resources/icons/refresh.png")));
    QIcon iconLcd(QPixmap(QStringLiteral(":/icons/resources/icons/lcd.png")));
    QIcon iconInfo(QPixmap(QStringLiteral(":/icons/resources/icons/misc.png")));

    m_group = new QGroupBox(this);
    QHBoxLayout *hlayout = new QHBoxLayout(m_group);

    m_config = new QLineEdit(m_group);
    m_config->setFocusPolicy(Qt::ClickFocus);
    m_btnLcd = new QToolButton(m_group);
    m_btnRefresh = new QToolButton(m_group);
    m_btnConfig = new QToolButton(m_group);

    m_btnLcd->setIcon(iconLcd);
    m_btnRefresh->setIcon(iconRefresh);
    m_btnConfig->setIcon(iconInfo);

    hlayout->addWidget(m_config);
    hlayout->addWidget(m_btnRefresh);
    hlayout->addWidget(m_btnLcd);
    hlayout->addWidget(m_btnConfig);

    m_view = new VisualizerDisplayWidget(this);
    m_view->setLayoutDirection(Qt::LeftToRight);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->addWidget(m_group);
    vlayout->addWidget(m_view);
    vlayout->addStretch(1);
    setLayout(vlayout);

    connect(m_config, &QLineEdit::returnPressed, this, &VisualizerWidget::stringToView);
    connect(m_btnRefresh, &QPushButton::clicked, this, &VisualizerWidget::stringToView);
    connect(m_btnLcd, &QPushButton::clicked, this, &VisualizerWidget::showPresets);
    connect(m_btnConfig, &QPushButton::clicked, this, &VisualizerWidget::showConfig);

    if (config.isEmpty()) {
        resetView();
    } else {
        m_config->setText(config);
        stringToView();
    }

    translate();
}

VisualizerWidget::~VisualizerWidget() = default;

void VisualizerWidget::translate() {
    m_group->setTitle(tr("Settings"));
    m_btnLcd->setToolTip(tr("Preset Configurations"));
    m_btnRefresh->setToolTip(tr("Apply changes"));
    m_btnConfig->setToolTip(tr("Change Configuration"));
}

void VisualizerWidget::showPresets() {
    QMenu menu;
    QAction *preset_1 = menu.addAction(tr("Current LCD State"));
    QAction *preset_2 = menu.addAction(tr("8bpp Buffer 1"));
    QAction *preset_3 = menu.addAction(tr("8bpp Buffer 2"));
    QAction *preset_4 = menu.addAction(tr("Palette View"));

    QAction *item = menu.exec(mapToGlobal(m_btnLcd->pos()));
    if (item == preset_1) {
        resetView();
    } else if (item == preset_2) {
        m_config->setText("d40000,320x240,8bpp,bgr");
        stringToView();
    } else if (item == preset_3) {
        m_config->setText("d52c00,320x240,8bpp,bgr");
        stringToView();
    } else if (item == preset_4) {
        m_config->setText("e30200,32x8,1555bpp,bgr,1000%");
        stringToView();
    }
}

void VisualizerWidget::showConfig() {
    QDialog *dialog = new QDialog;

    QGridLayout *mlayout = new QGridLayout(dialog);
    QGridLayout *glayout = new QGridLayout;
    QHBoxLayout *hlayout = new QHBoxLayout;

    QLabel *baseLbl = new QLabel(tr("Base Address"));
    QLineEdit *baseEdit = new QLineEdit(int2hex(m_base, 6));
    QLabel *fpsLbl = new QLabel(QStringLiteral("FPS"));
    QSpinBox *fpsSpin = new QSpinBox;
    QLabel *scaleLbl = new QLabel(tr("Scale"));
    QSpinBox *scaleSpin = new QSpinBox;
    QLabel *widthLbl = new QLabel(tr("Width"));
    QSpinBox *widthSpin = new QSpinBox;
    QLabel *heightLbl = new QLabel(tr("Height"));
    QSpinBox *heightSpin = new QSpinBox;
    QLabel *bppLbl = new QLabel(QStringLiteral("BPP"));
    QComboBox *bppCombo = new QComboBox;
    QCheckBox *beboChk = new QCheckBox(QStringLiteral("BEBO"));
    QCheckBox *bepoChk = new QCheckBox(QStringLiteral("BEPO"));
    QCheckBox *bgrChk = new QCheckBox(QStringLiteral("BGR"));
    QCheckBox *gridChk = new QCheckBox(tr("Grid"));
    QPushButton *submitBtn = new QPushButton(tr("Submit"));

    fpsSpin->setRange(0, 120);
    fpsSpin->setValue(m_fps);

    scaleSpin->setRange(0, 5000);
    scaleSpin->setValue(m_scale);

    widthSpin->setRange(0, 5000);
    widthSpin->setValue(m_width);

    heightSpin->setRange(0, 5000);
    heightSpin->setValue(m_height);

    bepoChk->setChecked(m_control & 0x400 ? true : false);
    beboChk->setChecked(m_control & 0x200 ? true : false);
    bgrChk->setChecked(m_control & 0x100 ? true : false);
    gridChk->setChecked(m_grid);

    bppCombo->addItem(QStringLiteral("1"));
    bppCombo->addItem(QStringLiteral("2"));
    bppCombo->addItem(QStringLiteral("4"));
    bppCombo->addItem(QStringLiteral("8"));
    bppCombo->addItem(QStringLiteral("16"));
    bppCombo->addItem(QStringLiteral("24"));
    bppCombo->addItem(QStringLiteral("16 (5:6:5)"));
    bppCombo->addItem(QStringLiteral("12 (4:4:4)"));

    bppCombo->setCurrentIndex((m_control >> 1) & 7);

    glayout->addWidget(baseLbl, 0, 0);
    glayout->addWidget(baseEdit, 0, 1);
    glayout->addWidget(fpsLbl, 1, 0);
    glayout->addWidget(fpsSpin, 1, 1);
    glayout->addWidget(scaleLbl, 2, 0);
    glayout->addWidget(scaleSpin, 2, 1);
    glayout->addWidget(widthLbl, 0, 2);
    glayout->addWidget(widthSpin, 0, 3);
    glayout->addWidget(heightLbl, 1, 2);
    glayout->addWidget(heightSpin, 1, 3);
    glayout->addWidget(bppLbl, 2, 2);
    glayout->addWidget(bppCombo, 2, 3);

    hlayout->addWidget(beboChk);
    hlayout->addWidget(bepoChk);
    hlayout->addWidget(bgrChk);
    hlayout->addWidget(gridChk);
    hlayout->addWidget(submitBtn);

    mlayout->addLayout(glayout, 0, 0);
    mlayout->addLayout(hlayout, 1, 0);

    dialog->setLayout(mlayout);

    connect(submitBtn, &QPushButton::clicked, [this, dialog, baseEdit,
            fpsSpin, scaleSpin, widthSpin, heightSpin,
            bppCombo, beboChk, bepoChk, bgrChk, gridChk]{

        m_base = static_cast<uint32_t>(hex2int(baseEdit->text()));

        m_control &= ~14u;
        m_control |= static_cast<unsigned int>(bppCombo->currentIndex() << 1);

        m_fps = fpsSpin->value();
        m_scale = scaleSpin->value();
        m_width = widthSpin->value();
        m_height = heightSpin->value();
        m_grid = gridChk->isChecked();

        set_reset(bepoChk->isChecked(), 0x400u, m_control);
        set_reset(beboChk->isChecked(), 0x200u, m_control);
        set_reset(bgrChk->isChecked(), 0x100u, m_control);

        viewToString();

        dialog->close();
    });
    dialog->exec();
}

void VisualizerWidget::stringToView() {
    QStringList string = m_config->text().split(',');
    QRegularExpression hex_reg("^[0-9A-F]{6}$", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression bpp_reg("^\\d{1,6}bpp$", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression fps_reg("^\\d+fps$", QRegularExpression::CaseInsensitiveOption);

    m_fps = 30;
    m_scale = 100;
    m_grid = false;

    set_reset(false, 0x400u, m_control);
    set_reset(false, 0x200u, m_control);
    set_reset(false, 0x100u, m_control);

    foreach (QString str, string) {
        str = str.toLower();
        if (!str.compare(QLatin1String("grid"), Qt::CaseInsensitive)) {
            m_grid = true;
        }
        if (!str.compare(QLatin1String("bepo"), Qt::CaseInsensitive)) {
            set_reset(true, 0x400u, m_control);
        }
        if (!str.compare(QLatin1String("bebo"), Qt::CaseInsensitive)) {
            set_reset(true, 0x200u, m_control);
        }
        if (!str.compare(QLatin1String("bgr"), Qt::CaseInsensitive)) {
            set_reset(true, 0x100u, m_control);
        }
        if (str.contains('x')) {
            QStringList wh = str.split('x');
            if (wh.size() == 2) {
                m_width = wh.at(0).toInt();
                m_height = wh.at(1).toInt();
            }
        }
        if (str.endsWith('%')) {
            str.remove('%');
            m_scale = str.toInt();
            if (m_scale > 5000) {
                m_scale = 5000;
            }
        }
        if (str.length() == 8 && str.at(0) == '0' && str.at(1) == 'x') {
            str.remove(0, 2);
        }
        if (str.length() == 7 && str.at(0) == '$') {
            str.remove(0, 1);
        }
        if (hex_reg.match(str).hasMatch()) {
            m_base = str.toUInt(Q_NULLPTR, 16);
        }
        if (bpp_reg.match(str).hasMatch()) {
            str.chop(3);
            uint8_t bpp;
            switch (str.toUInt()) {
                case 1: bpp = 0; break;
                case 2: bpp = 1; break;
                case 4: bpp = 2; break;
                case 8: bpp = 3; break;
                case 161555: case 1555: bpp = 4; break;
                case 24: case 888: bpp = 5; break;
                case 16: case 565: bpp = 6; break;
                case 12: case 444: bpp = 7; break;
                default: bpp = 255; break;
            }
            if (bpp != 255) {
                m_control &= ~14u;
                m_control |= static_cast<unsigned int>(bpp << 1);
            }
        }
        if (fps_reg.match(str).hasMatch()) {
            str.chop(3);
            m_fps = str.toInt();
            if (m_fps < 1 || m_fps > 120) { m_fps = 30; }
        }
    }

    viewToString();
}

void VisualizerWidget::resetView() {
    m_width = LCD_WIDTH;
    m_height = LCD_HEIGHT;
    m_base = lcd.upbase;
    m_control = lcd.control;
    m_scale = 100.0;
    m_grid = false;
    viewToString();
}

void VisualizerWidget::forceUpdate() {
    stringToView();
}

void VisualizerWidget::viewToString() {
    QString bpp;
    float bppstep = 1.0f;

    switch ((m_control >> 1) & 7) {
        case 0: bpp = QStringLiteral("1"); bppstep = 1.0f/0.125f; break;
        case 1: bpp = QStringLiteral("2"); bppstep = 1.0f/0.25f; break;
        case 2: bpp = QStringLiteral("4"); bppstep = 1.0f/0.5f; break;
        case 3: bpp = QStringLiteral("8"); bppstep = 1.0f/1.0f; break;
        case 4: bpp = QStringLiteral("1555"); bppstep = 1.0f/2.0f; break;
        case 5: bpp = QStringLiteral("888"); bppstep = 1.0f/3.0f; break;
        case 6: bpp = QStringLiteral("565"); bppstep = 1.0f/2.0f; break;
        case 7: bpp = QStringLiteral("444"); bppstep = 1.0f/1.5f; break;
        default: break;
    }

    m_setup.clear();
    m_setup.append(int2hex(m_base, 6).toUpper());
    m_setup.append(QString::number(m_width) + QStringLiteral("x") + QString::number(m_height));
    m_setup.append(bpp + QStringLiteral("bpp"));
    if (m_control & 0x400) { m_setup.append(QStringLiteral("bepo")); }
    if (m_control & 0x200) { m_setup.append(QStringLiteral("bebo")); }
    if (m_control & 0x100) { m_setup.append(QStringLiteral("bgr")); }
    if (m_scale != 100) { m_setup.append(QString::number(m_scale) + QStringLiteral("%")); }
    if (m_fps != 30) { m_setup.append(QString::number(m_fps) + QStringLiteral("fps")); }
    if (m_grid == true) { m_setup.append(QStringLiteral("grid")); }

    m_config->setText(m_setup.join(","));

    float s = m_scale / 100.0f;
    float w = m_width * s;
    float h = m_height * s;

    uint32_t *data;
    uint32_t *data_end;

    emu_set_lcd_ptrs(&data, &data_end, m_width, m_height, m_base, m_control, false);

    m_view->setFixedSize(static_cast<int>(w), static_cast<int>(h));
    m_view->setRefreshRate(m_fps);
    m_view->setConfig(bppstep, m_width, m_height, m_base, m_control, m_grid, data, data_end);
    adjustSize();

    emit configChanged();
}

QString VisualizerWidget::getConfig() {
    return m_config->text();
}
