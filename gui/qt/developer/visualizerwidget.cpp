#include "visualizerwidget.h"

#include "../corewrapper.h"
#include "../keypad/keypadwidget.h"
#include "../util.h"

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

VisualizerWidget::VisualizerWidget(CoreWindow *coreWindow, KDDockWidgets::DockWidgetBase *dock)
    : DockedWidget{dock ? dock : new KDDockWidgets::DockWidget{QStringLiteral("Visualizer #") + Util::randomString(6)},
                   QIcon(QStringLiteral(":/assets/icons/add_image.svg")),
                   coreWindow}
{
    mGroup = new QGroupBox(tr("Visualizer"));
    mGrpCfg = new QGroupBox(tr("Setup"));

    mConfigStr = new QLineEdit;
    mBtnLcd =  new QPushButton(QIcon(QStringLiteral(":/assets/icons/picture.svg")), tr("Presets"));
    mBtnConfig = new QPushButton(QIcon(QStringLiteral(":/assets/icons/support.svg")), tr("Setup"));
    mBtnApply = new QPushButton(QIcon(QStringLiteral(":/assets/icons/ok.svg")), tr("Apply"));

    mLcd = new VisualizerLcdWidget;

    QGridLayout *g1 = new QGridLayout;
    QGridLayout *g0 = new QGridLayout;
    QHBoxLayout *h0 = new QHBoxLayout;

    QLabel *baseLbl = new QLabel(tr("Base Address"));
    QLabel *fpsLbl = new QLabel(QStringLiteral("FPS"));
    QLabel *scaleLbl = new QLabel(tr("Scale"));
    QLabel *widthLbl = new QLabel(tr("Width"));
    QLabel *heightLbl = new QLabel(tr("Height"));
    QLabel *bppLbl = new QLabel(QStringLiteral("BPP"));

    mBaseEdit = new QLineEdit(Util::int2hex(mLcdConfig.mBaseAddr, 6));
    mFpsSpin = new QSpinBox;
    mScaleSpin = new QSpinBox;
    mWidthSpin = new QSpinBox;
    mHeightSpin = new QSpinBox;
    mBppCombo = new QComboBox;
    mBeboChk = new QCheckBox(QStringLiteral("bebo"));
    mBepoChk = new QCheckBox(QStringLiteral("bepo"));
    mBgrChk = new QCheckBox(QStringLiteral("bgr"));
    mGridChk = new QCheckBox(tr("grid"));

    mFpsSpin->setRange(0, 120);
    mFpsSpin->setValue(mFps);

    mScaleSpin->setRange(0, 5000);
    mScaleSpin->setValue(mScale);

    mWidthSpin->setRange(0, 5000);
    mWidthSpin->setValue(mLcdConfig.mWidth);

    mHeightSpin->setRange(0, 5000);
    mHeightSpin->setValue(mLcdConfig.mHeight);

    mBeboChk->setChecked(mLcdConfig.mCtlReg & 0x200 ? true : false);
    mBepoChk->setChecked(mLcdConfig.mCtlReg & 0x400 ? true : false);
    mBgrChk->setChecked(mLcdConfig.mCtlReg & 0x100 ? true : false);
    mGridChk->setChecked(mLcdConfig.mGrid);

    mBppCombo->addItem(QStringLiteral("1"));
    mBppCombo->addItem(QStringLiteral("2"));
    mBppCombo->addItem(QStringLiteral("4"));
    mBppCombo->addItem(QStringLiteral("8"));
    mBppCombo->addItem(QStringLiteral("16"));
    mBppCombo->addItem(QStringLiteral("24"));
    mBppCombo->addItem(QStringLiteral("16 (5:6:5)"));
    mBppCombo->addItem(QStringLiteral("12 (4:4:4)"));
    mBppCombo->setCurrentIndex((mLcdConfig.mCtlReg >> 1) & 7);

    g0->addWidget(baseLbl, 0, 0);
    g0->addWidget(mBaseEdit, 0, 1);
    g0->addWidget(fpsLbl, 1, 0);
    g0->addWidget(mFpsSpin, 1, 1);
    g0->addWidget(scaleLbl, 2, 0);
    g0->addWidget(mScaleSpin, 2, 1);
    g0->addWidget(widthLbl, 0, 2);
    g0->addWidget(mWidthSpin, 0, 3);
    g0->addWidget(heightLbl, 1, 2);
    g0->addWidget(mHeightSpin, 1, 3);
    g0->addWidget(bppLbl, 2, 2);
    g0->addWidget(mBppCombo, 2, 3);

    h0->addWidget(mBeboChk);
    h0->addWidget(mBepoChk);
    h0->addWidget(mBgrChk);
    h0->addWidget(mGridChk);
    h0->addWidget(mBtnApply);

    g1->addLayout(g0, 0, 0);
    g1->addLayout(h0, 1, 0);

    mGrpCfg->setLayout(g1);
    mGrpCfg->setVisible(false);

    QHBoxLayout *h2 = new QHBoxLayout;
    h2->addStretch();
    h2->addWidget(mGrpCfg);
    h2->addStretch();

    QHBoxLayout *h3 = new QHBoxLayout;
    h3->addWidget(mBtnLcd);
    h3->addStretch();
    h3->addWidget(mBtnConfig);

    QVBoxLayout *v0 = new QVBoxLayout;
    v0->addLayout(h3);
    v0->addLayout(h2);
    mGroup->setLayout(v0);

    QHBoxLayout *h4 = new QHBoxLayout;
    h4->addStretch();
    h4->addWidget(mLcd);
    h4->addStretch();

    QVBoxLayout *v1 = new QVBoxLayout;
    v1->addWidget(mGroup);
    v1->addLayout(h4);
    v1->addStretch();
    setLayout(v1);

    mBtnConfig->setCheckable(true);
    mBtnApply->setEnabled(false);

    resetView();

    connect(mBtnLcd, &QPushButton::clicked, this, &VisualizerWidget::showPresets);
    connect(mBtnConfig, &QPushButton::clicked, this, &VisualizerWidget::showConfig);
    connect(mBtnApply, &QPushButton::clicked, this, &VisualizerWidget::applyConfig);
    connect(&core(), &CoreWrapper::lcdFrame, this, &VisualizerWidget::lcdFrame);
    connect(mBppCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this]{ mBtnApply->setEnabled(true); });
    connect(mGridChk, &QCheckBox::clicked, [this]{ mBtnApply->setEnabled(true); });
    connect(mBgrChk, &QCheckBox::clicked, [this]{ mBtnApply->setEnabled(true); });
    connect(mBepoChk, &QCheckBox::clicked, [this]{ mBtnApply->setEnabled(true); });
    connect(mBeboChk, &QCheckBox::clicked, [this]{ mBtnApply->setEnabled(true); });
    connect(mHeightSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this]{ mBtnApply->setEnabled(true); });
    connect(mWidthSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this]{ mBtnApply->setEnabled(true); });
    connect(mScaleSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this]{ mBtnApply->setEnabled(true); });
    connect(mFpsSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this]{ mBtnApply->setEnabled(true); });
    connect(mBaseEdit, &QLineEdit::textChanged, [this]{ mBtnApply->setEnabled(true); });
}

void VisualizerWidget::showPresets()
{
    const QString preset1 = tr("Current LCD State");
    const QString preset2 = tr("8bpp Buffer 1");
    const QString preset3 = tr("8bpp Buffer 2");
    const QString preset4 = tr("Palette View");

    QMenu menu;
    menu.addAction(preset1);
    menu.addAction(preset2);
    menu.addAction(preset3);
    menu.addAction(preset4);

    QAction *item = menu.exec(mBtnLcd->mapToGlobal({0, mBtnLcd->height() + 1}));
    if (item)
    {
        if (item->text() == preset1)
        {
            resetView();
        }
        else if (item->text() == preset2)
        {
            mConfigStr->setText("d40000,320x240,8bpp,bgr");
            stringToView();
        }
        else if (item->text() == preset3)
        {
            mConfigStr->setText("d52c00,320x240,8bpp,bgr");
            stringToView();
        }
        else if (item->text() == preset4)
        {
            mConfigStr->setText("e30200,32x8,1555bpp,bgr,1000%");
            stringToView();
        }
    }
}

void VisualizerWidget::showConfig()
{
    mGrpCfg->setVisible(mBtnConfig->isChecked());
}

void VisualizerWidget::applyConfig()
{
    mLcdConfig.mBaseAddr = static_cast<uint32_t>(Util::hex2int(mBaseEdit->text()));
    mLcdConfig.mCtlReg &= ~14u;
    mLcdConfig.mCtlReg |= static_cast<unsigned int>(mBppCombo->currentIndex() << 1);
    mFps = mFpsSpin->value();
    mScale = mScaleSpin->value();
    mLcdConfig.mWidth = mWidthSpin->value();
    mLcdConfig.mHeight = mHeightSpin->value();
    mLcdConfig.mGrid = mGridChk->isChecked();
    SETBITS(mBeboChk->isChecked(), 0x200u, mLcdConfig.mCtlReg);
    SETBITS(mBepoChk->isChecked(), 0x400u, mLcdConfig.mCtlReg);
    SETBITS(mBgrChk->isChecked(), 0x100u, mLcdConfig.mCtlReg);
    viewToString();

    mBtnApply->setEnabled(false);
}

void VisualizerWidget::closeEvent(QCloseEvent *)
{
    if (auto *p = parent())
    {
        p->deleteLater();
    }
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
            mLcdConfig.mBaseAddr = str.toUInt(nullptr, 16);
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

void VisualizerWidget::lcdFrame()
{
}

void VisualizerWidget::resetView()
{
    mScale = 100.0;

    mLcdConfig.mWidth = 320;
    mLcdConfig.mHeight = 240;
    mLcdConfig.mBaseAddr = core().get(cemucore::CEMUCORE_PROP_PORT, 0x4010);
    mLcdConfig.mCtlReg = core().get(cemucore::CEMUCORE_PROP_PORT, 0x4018);
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

    emit configChanged(dock()->uniqueName(), getConfig());
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
