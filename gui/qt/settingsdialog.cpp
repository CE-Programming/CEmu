/*
 * Copyright (c) 2015-2021 CE Programming.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "settingsdialog.h"
#include "settings.h"

#include "keypad/keymap.h"
#include "keypad/keypadwidget.h"

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog{parent},
      mReset{Settings::Reset::None}
{
    mBtnBox = new QDialogButtonBox(QDialogButtonBox::Cancel |
                                   QDialogButtonBox::Save);

    SettingsGeneralTab *genTab = new SettingsGeneralTab;
    SettingsResetTab *resetTab = new SettingsResetTab;

    mTabWidget = new QTabWidget;
    mTabWidget->addTab(genTab, tr("General"));
    mTabWidget->addTab(resetTab, tr("Reset"));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetMinimumSize);
    layout->addWidget(mTabWidget, Qt::AlignCenter);
    layout->addWidget(mBtnBox, Qt::AlignCenter);

    connect(genTab, &SettingsGeneralTab::changeLanguage, [this]{ mReset = Settings::Reset::Langauge; });
    connect(genTab, &SettingsGeneralTab::changePortable, [this]{ mReset = Settings::Reset::Portable; });
    connect(genTab, &SettingsGeneralTab::changedKeypadColor, this, &SettingsDialog::changedKeypadColor);
    connect(resetTab, &SettingsResetTab::reset, [this](int reset){ mReset = reset; accept(); });
    connect(mBtnBox, &QDialogButtonBox::accepted, genTab, &SettingsGeneralTab::saveSettings);
    connect(mBtnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(mBtnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    setLayout(layout);
    setWindowTitle(tr("Preferences"));
    setWindowIcon(QIcon(QStringLiteral(":/assets/icons/support.svg")));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

SettingsGeneralTab::SettingsGeneralTab(QWidget *parent)
    : QWidget(parent)
{
    mChkAutoUpdate = new QCheckBox(tr("Check for updates on start"));
    mChkPortable = new QCheckBox(tr("Portable mode"));

    mCmbLang = new QComboBox;
    mCmbLang->addItems({ tr("English"), tr("French"), tr("Spanish") });

    QGroupBox *grpLang = new QGroupBox(tr("Language"));
    QVBoxLayout *vboxLang = new QVBoxLayout;
    vboxLang->addWidget(mCmbLang);
    grpLang->setLayout(vboxLang);

    QGroupBox *grpConfig = new QGroupBox(tr("Configuration"));
    QVBoxLayout *vboxConfig = new QVBoxLayout;
    vboxConfig->addWidget(mChkAutoUpdate);
    vboxConfig->addWidget(mChkPortable);
    vboxConfig->addStretch(1);
    grpConfig->setLayout(vboxConfig);

    mKeybind = new QButtonGroup(this);

    QRadioButton *btn0 = new QRadioButton(tr("Natural"));
    QRadioButton *btn1 = new QRadioButton(tr("CEmu"));
    QRadioButton *btn2 = new QRadioButton(tr("TilEm"));
    QRadioButton *btn3 = new QRadioButton(tr("Wabbitemu"));
    QRadioButton *btn4 = new QRadioButton(tr("jsTIfied"));
    QRadioButton *btn5 = new QRadioButton(tr("Custom"));

    mKeybind->addButton(btn0, Keymap::Natural);
    mKeybind->addButton(btn1, Keymap::CEmu);
    mKeybind->addButton(btn2, Keymap::TilEm);
    mKeybind->addButton(btn3, Keymap::WabbitEmu);
    mKeybind->addButton(btn4, Keymap::JsTIfied);
    mKeybind->addButton(btn5, Keymap::Custom);

    QGroupBox *grpKeys = new QGroupBox(tr("Key bindings"));
    QGridLayout *gbox = new QGridLayout;
    gbox->addWidget(btn0, 0, 0);
    gbox->addWidget(btn1, 0, 1);
    gbox->addWidget(btn2, 0, 2);
    gbox->addWidget(btn3, 1, 0);
    gbox->addWidget(btn4, 1, 1);
    gbox->addWidget(btn5, 1, 2);
    grpKeys->setLayout(gbox);

    QGroupBox *grpKeyColor = new QGroupBox(tr("Keypad color"));
    QPushButton *btnColor = new QPushButton(tr("Change Keypad Color"));
    QHBoxLayout *hboxColor = new QHBoxLayout;
    hboxColor->addWidget(btnColor);
    grpKeyColor->setLayout(hboxColor);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(grpLang);
    mainLayout->addWidget(grpConfig);
    mainLayout->addWidget(grpKeys);
    mainLayout->addWidget(grpKeyColor);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    mPortable = Settings::boolOption(Settings::PortableMode);
    if (mPortable)
    {
        mChkPortable->setEnabled(!Settings::hasPortableDir());
    }
    else
    {
        mChkPortable->setEnabled(Settings::canBePortable());
    }

    int keymap = Settings::intOption(Settings::KeyMap);
    if (keymap < 0 || keymap > 5)
    {
        keymap = 1;
        Settings::setIntOption(Settings::KeyMap, keymap);
    }
    mKeybind->buttons().at(keymap)->setChecked(true);

    mLang = Settings::intOption(Settings::Language);
    if (mLang < 0 || mLang >= Settings::Lang::NumberOfLangs)
    {
        mLang = Settings::Lang::English;
    }
    mCmbLang->setCurrentIndex(mLang);

    mKeypadColor = Settings::intOption(Settings::KeypadColor);
    connect(btnColor, &QPushButton::clicked, [this]
    {
        KeyColorDialog dialog(mKeypadColor);
        connect(&dialog, &KeyColorDialog::changedColor, this, &SettingsGeneralTab::changedKeypadColor);
        if (dialog.exec())
        {
            mKeypadColor = dialog.getColor();
        }
        emit changedKeypadColor(mKeypadColor);
    });

    mChkAutoUpdate->setChecked(Settings::boolOption(Settings::AutoUpdate));
    mChkPortable->setChecked(Settings::boolOption(Settings::PortableMode));
}

void SettingsGeneralTab::saveSettings()
{
    Settings::setIntOption(Settings::Language, mCmbLang->currentIndex());
    Settings::setIntOption(Settings::KeyMap, mKeybind->checkedId());
    Settings::setBoolOption(Settings::AutoUpdate, mChkAutoUpdate->isChecked());
    Settings::setIntOption(Settings::KeypadColor, mKeypadColor);

    if (mPortable != mChkPortable->isChecked())
    {
        Settings::setPortable(mChkPortable->isChecked());
        emit changePortable();
    }

    if (mLang != mCmbLang->currentIndex())
    {
        emit changeLanguage();
    }
}

SettingsResetTab::SettingsResetTab(QWidget *parent)
    : QWidget(parent)
{
    QGroupBox *grpReset = new QGroupBox(tr("Reset"));

    QPushButton *btnResetDefaults = new QPushButton(tr("Reset Defaults"));
    QPushButton *btnResetGui = new QPushButton(tr("Reset GUI Docks"));
    QPushButton *btnResetAll = new QPushButton(tr("Reset All"));

    QVBoxLayout *vboxReset = new QVBoxLayout;
    vboxReset->addWidget(btnResetDefaults);
    vboxReset->addWidget(btnResetGui);
    vboxReset->addWidget(btnResetAll);
    grpReset->setLayout(vboxReset);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(grpReset);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    connect(btnResetDefaults, &QPushButton::clicked, [this]{ emit reset(Settings::Reset::Defaults); });
    connect(btnResetGui, &QPushButton::clicked, [this]{ emit reset(Settings::Reset::Gui); });
    connect(btnResetAll, &QPushButton::clicked, [this]{ emit reset(Settings::Reset::All); });
}

KeyColorDialog::KeyColorDialog(int color, QWidget *parent)
    : QDialog{parent}
{
    mColors = new QButtonGroup(this);

    QRadioButton *btnBlack = new QRadioButton(QStringLiteral("Black"));
    QRadioButton *btnWhite = new QRadioButton(QStringLiteral("White"));
    QRadioButton *btnTrueBlue = new QRadioButton(QStringLiteral("True Blue"));
    QRadioButton *btnDenim = new QRadioButton(QStringLiteral("Denim"));
    QRadioButton *btnSilver = new QRadioButton(QStringLiteral("Silver"));
    QRadioButton *btnPink = new QRadioButton(QStringLiteral("Pink"));
    QRadioButton *btnPlum = new QRadioButton(QStringLiteral("Plum"));
    QRadioButton *btnRed = new QRadioButton(QStringLiteral("Red"));
    QRadioButton *btnLightning = new QRadioButton(QStringLiteral("Lightning"));
    QRadioButton *btnGold = new QRadioButton(QStringLiteral("Gold"));
    QRadioButton *btnSpaceGrey = new QRadioButton(QStringLiteral("Space Grey"));
    QRadioButton *btnCoral = new QRadioButton(QStringLiteral("Coral"));
    QRadioButton *btnMint = new QRadioButton(QStringLiteral("Mint"));
    QRadioButton *btnRoseGold = new QRadioButton(QStringLiteral("Rose Gold"));
    QRadioButton *btnCrystalClear = new QRadioButton(QStringLiteral("Crystal Clear"));
    QRadioButton *btnMatteBlack = new QRadioButton(QStringLiteral("Matte Black"));
    QRadioButton *btnTangentTeal = new QRadioButton(QStringLiteral("Tangent Teal"));
    QRadioButton *btnTotallyTeal = new QRadioButton(QStringLiteral("Totally Teal"));

    QPalette palette(btnBlack->palette());

    palette.setColor(QPalette::Text, {255, 255, 255, 255});

    palette.setColor(QPalette::Base, QColor::fromRgb(0x191919));
    btnBlack->setPalette(palette);
    palette.setColor(QPalette::Base, QColor::fromRgb(0xe8e8e8));
    btnWhite->setPalette(palette);
    palette.setColor(QPalette::Base, QColor::fromRgb(0x385E9D));
    btnTrueBlue->setPalette(palette);
    palette.setColor(QPalette::Base, QColor::fromRgb(0x003C71));
    btnDenim->setPalette(palette);
    palette.setColor(QPalette::Base, QColor::fromRgb(0x7C878E));
    btnSilver->setPalette(palette);
    palette.setColor(QPalette::Base, QColor::fromRgb(0xDF1995));
    btnPink->setPalette(palette);
    palette.setColor(QPalette::Base, QColor::fromRgb(0x830065));
    btnPlum->setPalette(palette);
    palette.setColor(QPalette::Base, QColor::fromRgb(0xAB2328));
    btnRed->setPalette(palette);
    palette.setColor(QPalette::Base, QColor::fromRgb(0x0077C8));
    btnLightning->setPalette(palette);
    palette.setColor(QPalette::Base, QColor::fromRgb(0xD8D3B6));
    btnGold->setPalette(palette);
    palette.setColor(QPalette::Base, QColor::fromRgb(0xDBDBDB));
    btnSpaceGrey->setPalette(palette);
    palette.setColor(QPalette::Base, QColor::fromRgb(0xFD6D99));
    btnCoral->setPalette(palette);
    palette.setColor(QPalette::Base, QColor::fromRgb(0xD2EBE8));
    btnMint->setPalette(palette);
    palette.setColor(QPalette::Base, QColor::fromRgb(0xAF867C));
    btnRoseGold->setPalette(palette);
    palette.setColor(QPalette::Base, QColor::fromRgb(0xACA7AE));
    btnCrystalClear->setPalette(palette);
    palette.setColor(QPalette::Base, QColor::fromRgb(0x0F0F0F));
    btnMatteBlack->setPalette(palette);
    palette.setColor(QPalette::Base, QColor::fromRgb(0x005062));
    btnTangentTeal->setPalette(palette);
    palette.setColor(QPalette::Base, QColor::fromRgb(0x108798));
    btnTotallyTeal->setPalette(palette);

    QGroupBox *grpColors = new QGroupBox(tr("Calculator Colors"));

    QGridLayout *gbox = new QGridLayout;
    gbox->addWidget(btnBlack, 0, 0);
    gbox->addWidget(btnWhite, 0, 1);
    gbox->addWidget(btnTrueBlue, 0, 2);
    gbox->addWidget(btnDenim, 1, 0);
    gbox->addWidget(btnSilver, 1, 1);
    gbox->addWidget(btnPink, 1, 2);
    gbox->addWidget(btnPlum, 2, 0);
    gbox->addWidget(btnRed, 2, 1);
    gbox->addWidget(btnLightning, 2, 2);
    gbox->addWidget(btnGold, 3, 0);
    gbox->addWidget(btnSpaceGrey, 3, 1);
    gbox->addWidget(btnCoral, 3, 2);
    gbox->addWidget(btnMint, 4, 0);
    gbox->addWidget(btnRoseGold, 4, 1);
    gbox->addWidget(btnCrystalClear, 4, 2);
    gbox->addWidget(btnMatteBlack, 5, 0);
    gbox->addWidget(btnTangentTeal, 5, 1);
    gbox->addWidget(btnTotallyTeal, 5, 2);

    mColors->addButton(btnBlack, int(KeypadWidget::Color::Black));
    mColors->addButton(btnWhite, int(KeypadWidget::Color::White));
    mColors->addButton(btnTrueBlue, int(KeypadWidget::Color::TrueBlue));
    mColors->addButton(btnDenim, int(KeypadWidget::Color::Denim));
    mColors->addButton(btnSilver, int(KeypadWidget::Color::Silver));
    mColors->addButton(btnPink, int(KeypadWidget::Color::Pink));
    mColors->addButton(btnPlum, int(KeypadWidget::Color::Plum));
    mColors->addButton(btnRed, int(KeypadWidget::Color::Red));
    mColors->addButton(btnLightning, int(KeypadWidget::Color::Lightning));
    mColors->addButton(btnGold, int(KeypadWidget::Color::Gold));
    mColors->addButton(btnSpaceGrey, int(KeypadWidget::Color::SpaceGrey));
    mColors->addButton(btnCoral, int(KeypadWidget::Color::Coral));
    mColors->addButton(btnMint, int(KeypadWidget::Color::Mint));
    mColors->addButton(btnRoseGold, int(KeypadWidget::Color::RoseGold));
    mColors->addButton(btnCrystalClear, int(KeypadWidget::Color::CrystalClear));
    mColors->addButton(btnMatteBlack, int(KeypadWidget::Color::MatteBlack));
    mColors->addButton(btnTangentTeal, int(KeypadWidget::Color::TangentTeal));
    mColors->addButton(btnTotallyTeal, int(KeypadWidget::Color::TotallyTeal));

    mColors->setExclusive(true);
    mColors->buttons().at(color)->setChecked(true);

    grpColors->setLayout(gbox);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Cancel |
                                                    QDialogButtonBox::Save);

    connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(grpColors);
    mainLayout->addWidget(btnBox);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    connect(mColors,
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
            QOverload<int>::of(&QButtonGroup::buttonClicked),
#else
            &QButtonGroup::idClicked,
#endif
            this, &KeyColorDialog::changedColor);
}

int KeyColorDialog::getColor()
{
    return mColors->checkedId();
}
