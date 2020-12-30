#include "visualizerwidget.h"

#include "cemucore.h"
#include "keypad/keypadwidget.h"
#include "util.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QAction>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

#define SETBITS(in, out, var) ((var) = static_cast<bool>(in) ? ((var) | (out)) : ((var) & ~(out)))

VisualizerWidgetList::VisualizerWidgetList()
    : mPrev{this},
      mNext{this}
{
}

VisualizerWidgetList::VisualizerWidgetList(VisualizerWidgetList *list)
    : mPrev{qExchange(list->mPrev, this)},
      mNext{list}
{
    mPrev->mNext = this;
}

VisualizerWidgetList::~VisualizerWidgetList()
{
    mPrev->mNext = mNext;
    mNext->mPrev = mPrev;
}

VisualizerWidget::VisualizerWidget(VisualizerWidgetList *list, const QString &config, QWidget *parent)
    : QWidget{parent},
      VisualizerWidgetList{list}
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

    if (config.isEmpty())
    {
        resetView();
    }
    else
    {
        setConfig(config);
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
    QLineEdit *baseEdit = new QLineEdit(Util::int2hex(mLcdConfig.mBaseAddr, 6));
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
    widthSpin->setValue(mLcdConfig.mWidth);

    heightSpin->setRange(0, 5000);
    heightSpin->setValue(mLcdConfig.mHeight);

    bepoChk->setChecked(mLcdConfig.mCtlReg & 0x400 ? true : false);
    beboChk->setChecked(mLcdConfig.mCtlReg & 0x200 ? true : false);
    bgrChk->setChecked(mLcdConfig.mCtlReg & 0x100 ? true : false);
    gridChk->setChecked(mLcdConfig.mGrid);

    bppCombo->addItem(QStringLiteral("1"));
    bppCombo->addItem(QStringLiteral("2"));
    bppCombo->addItem(QStringLiteral("4"));
    bppCombo->addItem(QStringLiteral("8"));
    bppCombo->addItem(QStringLiteral("16"));
    bppCombo->addItem(QStringLiteral("24"));
    bppCombo->addItem(QStringLiteral("16 (5:6:5)"));
    bppCombo->addItem(QStringLiteral("12 (4:4:4)"));

    bppCombo->setCurrentIndex((mLcdConfig.mCtlReg >> 1) & 7);

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

    connect(submitBtn, &QPushButton::clicked, [=]
    {
        mLcdConfig.mBaseAddr = static_cast<uint32_t>(Util::hex2int(baseEdit->text()));

        mLcdConfig.mCtlReg &= ~14u;
        mLcdConfig.mCtlReg |= static_cast<unsigned int>(bppCombo->currentIndex() << 1);

        mFps = fpsSpin->value();
        mScale = scaleSpin->value();
        mLcdConfig.mWidth = widthSpin->value();
        mLcdConfig.mHeight = heightSpin->value();
        mLcdConfig.mGrid = gridChk->isChecked();

        SETBITS(bepoChk->isChecked(), 0x400u, mLcdConfig.mCtlReg);
        SETBITS(beboChk->isChecked(), 0x200u, mLcdConfig.mCtlReg);
        SETBITS(bgrChk->isChecked(), 0x100u, mLcdConfig.mCtlReg);

        viewToString();

        dialog->close();
    });
    dialog->exec();
}

void VisualizerWidget::closeEvent(QCloseEvent *)
{
    parent()->deleteLater();
}

void VisualizerWidget::stringToView()
{
    QStringList string = mConfigStr->text().split(',');
    QRegExp hex_reg("^[0-9A-F]{6}$", Qt::CaseInsensitive);
    QRegExp bpp_reg("^\\d{1,6}bpp$", Qt::CaseInsensitive);
    QRegExp fps_reg("^\\d+fps$", Qt::CaseInsensitive);

    mFps = 30;
    mScale = 100;
    mLcdConfig.mGrid = false;

    SETBITS(false, 0x400u, mLcdConfig.mCtlReg);
    SETBITS(false, 0x200u, mLcdConfig.mCtlReg);
    SETBITS(false, 0x100u, mLcdConfig.mCtlReg);

    foreach (QString str, string)
    {
        str = str.toLower();
        if (!str.compare(QLatin1String("grid"), Qt::CaseInsensitive))
        {
            mLcdConfig.mGrid = true;
        }
        if (!str.compare(QLatin1String("bepo"), Qt::CaseInsensitive))
        {
            SETBITS(true, 0x400u, mLcdConfig.mCtlReg);
        }
        if (!str.compare(QLatin1String("bebo"), Qt::CaseInsensitive))
        {
            SETBITS(true, 0x200u, mLcdConfig.mCtlReg);
        }
        if (!str.compare(QLatin1String("bgr"), Qt::CaseInsensitive))
        {
            SETBITS(true, 0x100u, mLcdConfig.mCtlReg);
        }
        if (str.contains('x'))
        {
            QStringList wh = str.split('x');
            if (wh.size() == 2) {
                mLcdConfig.mWidth = wh.at(0).toInt();
                mLcdConfig.mHeight = wh.at(1).toInt();
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
            mLcdConfig.mBaseAddr = str.toUInt(Q_NULLPTR, 16);
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
                mLcdConfig.mCtlReg &= ~14u;
                mLcdConfig.mCtlReg |= static_cast<unsigned int>(bpp << 1);
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
    mScale = 100.0;

    mLcdConfig.mWidth = LCD_WIDTH;
    mLcdConfig.mHeight = LCD_HEIGHT;
    mLcdConfig.mBaseAddr = lcd.upbase;
    mLcdConfig.mCtlReg = lcd.control;
    mLcdConfig.mGrid = false;
    viewToString();
}

void VisualizerWidget::forceUpdate()
{
    stringToView();
}

void VisualizerWidget::viewToString()
{
    QString bpp;
    mLcdConfig.mBppStep = 1.0f;

    switch ((mLcdConfig.mCtlReg >> 1) & 7)
    {
        case 0: bpp = QStringLiteral("1"); mLcdConfig.mBppStep = 1.0f/0.125f; break;
        case 1: bpp = QStringLiteral("2"); mLcdConfig.mBppStep = 1.0f/0.25f; break;
        case 2: bpp = QStringLiteral("4"); mLcdConfig.mBppStep = 1.0f/0.5f; break;
        case 3: bpp = QStringLiteral("8"); mLcdConfig.mBppStep = 1.0f/1.0f; break;
        case 4: bpp = QStringLiteral("1555"); mLcdConfig.mBppStep = 1.0f/2.0f; break;
        case 5: bpp = QStringLiteral("888"); mLcdConfig.mBppStep = 1.0f/3.0f; break;
        case 6: bpp = QStringLiteral("565"); mLcdConfig.mBppStep = 1.0f/2.0f; break;
        case 7: bpp = QStringLiteral("444"); mLcdConfig.mBppStep = 1.0f/1.5f; break;
        default: break;
    }

    mSetup.clear();
    mSetup.append(Util::int2hex(mLcdConfig.mBaseAddr, 6).toUpper());
    mSetup.append(QString::number(mLcdConfig.mWidth) + QStringLiteral("x") + QString::number(mLcdConfig.mHeight));
    mSetup.append(bpp + QStringLiteral("bpp"));
    if (mLcdConfig.mCtlReg & 0x400) { mSetup.append(QStringLiteral("bepo")); }
    if (mLcdConfig.mCtlReg & 0x200) { mSetup.append(QStringLiteral("bebo")); }
    if (mLcdConfig.mCtlReg & 0x100) { mSetup.append(QStringLiteral("bgr")); }
    if (mLcdConfig.mGrid == true) { mSetup.append(QStringLiteral("grid")); }
    if (mScale != 100) { mSetup.append(QString::number(mScale) + QStringLiteral("%")); }
    if (mFps != 30) { mSetup.append(QString::number(mFps) + QStringLiteral("fps")); }

    mConfigStr->setText(mSetup.join(","));

    float s = mScale / 100.0f;
    float w = mLcdConfig.mWidth * s;
    float h = mLcdConfig.mHeight * s;

    mLcd->setFixedSize(static_cast<int>(w), static_cast<int>(h));
    mLcd->setRefreshRate(mFps);
    mLcd->setConfig(mLcdConfig);
    adjustSize();

    emit configChanged(static_cast<KDDockWidgets::DockWidget *>(parent())->uniqueName(), getConfig());
}

void VisualizerWidget::setConfig(const QString &config)
{
    mConfigStr->setText(config);
    stringToView();
}

QString VisualizerWidget::getConfig() const
{
    return mConfigStr->text();
}
