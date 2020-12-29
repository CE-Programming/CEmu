#include "visualizerwidget.h"
#include "keypad/keypadwidget.h"
#include "util.h"

#include "../../core/lcd.h"

#include <QtWidgets/QPushButton>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QAction>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QMenu>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QLineEdit>

#define SETBITS(in, out, var) ((var) = static_cast<bool>(in) ? ((var) | (out)) : ((var) & ~(out)))

VisualizerWidget::VisualizerWidget(const QString &config, QWidget *parent)
    : QWidget{parent}
{
    mGroup = new QGroupBox(tr("Settings"));

    mConfigStr = new QLineEdit;
    mBtnLcd = new QToolButton;
    mBtnRefresh = new QToolButton;
    mBtnConfig = new QToolButton;

    mLcd = new VisualizerLcdWidget;

    mBtnLcd->setToolTip(tr("Preset Configurations"));
    mBtnConfig->setToolTip(tr("Change Configuration"));
    mBtnRefresh->setToolTip(tr("Apply changes"));

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(mConfigStr);
    hLayout->addWidget(mBtnRefresh);
    hLayout->addWidget(mBtnLcd);
    hLayout->addWidget(mBtnConfig);
    mGroup->setLayout(hLayout);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addStretch(1);
    vLayout->addWidget(mGroup);
    vLayout->addWidget(mLcd);
    vLayout->addStretch(1);
    setLayout(vLayout);

    connect(mConfigStr, &QLineEdit::returnPressed, this, &VisualizerWidget::stringToView);
    connect(mBtnRefresh, &QPushButton::clicked, this, &VisualizerWidget::stringToView);
    connect(mBtnLcd, &QPushButton::clicked, this, &VisualizerWidget::showPresets);
    connect(mBtnConfig, &QPushButton::clicked, this, &VisualizerWidget::showConfig);

    if (!config.isEmpty())
    {
        mConfigStr->setText(config);
        stringToView();
    }
}

void VisualizerWidget::showPresets()
{
    const QString preset_1 = tr("Current LCD State");
    const QString preset_2 = tr("8bpp Buffer 1");
    const QString preset_3 = tr("8bpp Buffer 2");
    const QString preset_4 = tr("Palette View");

    QMenu menu;
    menu.addAction(preset_1);
    menu.addAction(preset_2);
    menu.addAction(preset_3);
    menu.addAction(preset_4);

    QAction *item = menu.exec(mapToGlobal(mBtnLcd->pos()));
    if (item)
    {
        if (item->text() == preset_1)
        {
            resetView();
        }
        else if (item->text() == preset_2)
        {
            mConfigStr->setText("d40000,320x240,8bpp,bgr");
            stringToView();
        }
        else if (item->text() == preset_3)
        {
            mConfigStr->setText("d52c00,320x240,8bpp,bgr");
            stringToView();
        }
        else if (item->text() == preset_4)
        {
            mConfigStr->setText("e30200,32x8,1555bpp,bgr,1000%");
            stringToView();
        }
    }
}

void VisualizerWidget::showConfig()
{
    QDialog *dialog = new QDialog;

    QGridLayout *mlayout = new QGridLayout(dialog);
    QGridLayout *glayout = new QGridLayout;
    QHBoxLayout *hlayout = new QHBoxLayout;

    QLabel *baseLbl = new QLabel(tr("Base Address"));
    QLineEdit *baseEdit = new QLineEdit(Util::int2hex(mBaseAddr, 6));
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
    fpsSpin->setValue(mFps);

    scaleSpin->setRange(0, 5000);
    scaleSpin->setValue(mScale);

    widthSpin->setRange(0, 5000);
    widthSpin->setValue(mWidth);

    heightSpin->setRange(0, 5000);
    heightSpin->setValue(mHeight);

    bepoChk->setChecked(mCtlReg & 0x400 ? true : false);
    beboChk->setChecked(mCtlReg & 0x200 ? true : false);
    bgrChk->setChecked(mCtlReg & 0x100 ? true : false);
    gridChk->setChecked(mGrid);

    bppCombo->addItem(QStringLiteral("1"));
    bppCombo->addItem(QStringLiteral("2"));
    bppCombo->addItem(QStringLiteral("4"));
    bppCombo->addItem(QStringLiteral("8"));
    bppCombo->addItem(QStringLiteral("16"));
    bppCombo->addItem(QStringLiteral("24"));
    bppCombo->addItem(QStringLiteral("16 (5:6:5)"));
    bppCombo->addItem(QStringLiteral("12 (4:4:4)"));

    bppCombo->setCurrentIndex((mCtlReg >> 1) & 7);

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

        mBaseAddr = static_cast<uint32_t>(Util::hex2int(baseEdit->text()));

        mCtlReg &= ~14u;
        mCtlReg |= static_cast<unsigned int>(bppCombo->currentIndex() << 1);

        mFps = fpsSpin->value();
        mScale = scaleSpin->value();
        mWidth = widthSpin->value();
        mHeight = heightSpin->value();
        mGrid = gridChk->isChecked();

        SETBITS(bepoChk->isChecked(), 0x400u, mCtlReg);
        SETBITS(beboChk->isChecked(), 0x200u, mCtlReg);
        SETBITS(bgrChk->isChecked(), 0x100u, mCtlReg);

        viewToString();

        dialog->close();
    });
    dialog->exec();
}

void VisualizerWidget::stringToView()
{
    QStringList string = mConfigStr->text().split(',');
    QRegExp hex_reg("^[0-9A-F]{6}$", Qt::CaseInsensitive);
    QRegExp bpp_reg("^\\d{1,6}bpp$", Qt::CaseInsensitive);
    QRegExp fps_reg("^\\d+fps$", Qt::CaseInsensitive);

    mFps = 30;
    mScale = 100;
    mGrid = false;

    SETBITS(false, 0x400u, mCtlReg);
    SETBITS(false, 0x200u, mCtlReg);
    SETBITS(false, 0x100u, mCtlReg);

    foreach (QString str, string)
    {
        str = str.toLower();
        if (!str.compare(QLatin1String("grid"), Qt::CaseInsensitive))
        {
            mGrid = true;
        }
        if (!str.compare(QLatin1String("bepo"), Qt::CaseInsensitive))
        {
            SETBITS(true, 0x400u, mCtlReg);
        }
        if (!str.compare(QLatin1String("bebo"), Qt::CaseInsensitive))
        {
            SETBITS(true, 0x200u, mCtlReg);
        }
        if (!str.compare(QLatin1String("bgr"), Qt::CaseInsensitive))
        {
            SETBITS(true, 0x100u, mCtlReg);
        }
        if (str.contains('x'))
        {
            QStringList wh = str.split('x');
            if (wh.size() == 2) {
                mWidth = wh.at(0).toInt();
                mHeight = wh.at(1).toInt();
            }
        }
        if (str.endsWith('%'))
        {
            str.remove('%');
            mScale = str.toInt();
            if (mScale > 5000)
            {
                mScale = 5000;
            }
        }
        if (str.length() == 8 && str.at(0) == '0' && str.at(1) == 'x')
        {
            str.remove(0, 2);
        }
        if (str.length() == 7 && str.at(0) == '$')
        {
            str.remove(0, 1);
        }
        if (hex_reg.exactMatch(str))
        {
            mBaseAddr = str.toUInt(Q_NULLPTR, 16);
        }
        if (bpp_reg.exactMatch(str))
        {
            str.chop(3);
            uint8_t bpp;
            switch (str.toUInt())
            {
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
            if (bpp != 255)
            {
                mCtlReg &= ~14u;
                mCtlReg |= static_cast<unsigned int>(bpp << 1);
            }
        }
        if (fps_reg.exactMatch(str))
        {
            str.chop(3);
            mFps = str.toInt();
            if (mFps < 1 || mFps > 120) { mFps = 30; }
        }
    }

    viewToString();
}

void VisualizerWidget::resetView()
{
    mWidth = LCD_WIDTH;
    mHeight = LCD_HEIGHT;
    mBaseAddr = lcd.upbase;
    mCtlReg = lcd.control;
    mScale = 100.0;
    mGrid = false;
    viewToString();
}

void VisualizerWidget::forceUpdate()
{
    stringToView();
}

void VisualizerWidget::viewToString()
{
    QString bpp;
    float bppstep = 1.0f;

    switch ((mCtlReg >> 1) & 7)
    {
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

    mSetup.clear();
    mSetup.append(Util::int2hex(mBaseAddr, 6).toUpper());
    mSetup.append(QString::number(mWidth) + QStringLiteral("x") + QString::number(mHeight));
    mSetup.append(bpp + QStringLiteral("bpp"));
    if (mCtlReg & 0x400) { mSetup.append(QStringLiteral("bepo")); }
    if (mCtlReg & 0x200) { mSetup.append(QStringLiteral("bebo")); }
    if (mCtlReg & 0x100) { mSetup.append(QStringLiteral("bgr")); }
    if (mScale != 100) { mSetup.append(QString::number(mScale) + QStringLiteral("%")); }
    if (mFps != 30) { mSetup.append(QString::number(mFps) + QStringLiteral("fps")); }
    if (mGrid == true) { mSetup.append(QStringLiteral("grid")); }

    mConfigStr->setText(mSetup.join(","));

    float s = mScale / 100.0f;
    float w = mWidth * s;
    float h = mHeight * s;

    uint32_t *data;
    uint32_t *data_end;

    emu_set_lcd_ptrs(&data, &data_end, mWidth, mHeight, mBaseAddr, mCtlReg, false);

    mLcd->setFixedSize(static_cast<int>(w), static_cast<int>(h));
    mLcd->setRefreshRate(mFps);
    mLcd->setConfig(bppstep, mWidth, mHeight, mBaseAddr, mCtlReg, mGrid, data, data_end);
    adjustSize();

    emit configChanged();
}

QString VisualizerWidget::getConfig()
{
    return mConfigStr->text();
}
