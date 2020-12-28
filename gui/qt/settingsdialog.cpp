/*
 * Copyright (c) 2015-2020 CE Programming.
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
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog{parent}
{
    mBtnBox = new QDialogButtonBox(QDialogButtonBox::Cancel |
                                   QDialogButtonBox::Save);

    SettingsGeneralTab *genTab = new SettingsGeneralTab;
    SettingsEmulationTab *emuTab = new SettingsEmulationTab;
    SettingsDeveloperTab *devTab = new SettingsDeveloperTab;

    mTabWidget = new QTabWidget;
    mTabWidget->addTab(genTab, tr("General"));
    mTabWidget->addTab(emuTab, tr("Emulation"));
    mTabWidget->addTab(devTab, tr("Developer"));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetMinimumSize);
    layout->addWidget(mTabWidget, Qt::AlignCenter);
    layout->addWidget(mBtnBox, Qt::AlignCenter);

    connect(genTab, &SettingsGeneralTab::changedKeypadColor, this, &SettingsDialog::changedKeypadColor);
    connect(mBtnBox, &QDialogButtonBox::accepted, genTab, &SettingsGeneralTab::saveSettings);
    connect(mBtnBox, &QDialogButtonBox::accepted, emuTab, &SettingsEmulationTab::saveSettings);
    connect(mBtnBox, &QDialogButtonBox::accepted, devTab, &SettingsDeveloperTab::saveSettings);
    connect(mBtnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(mBtnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    setLayout(layout);
    setWindowTitle(tr("Preferences"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

SettingsGeneralTab::SettingsGeneralTab(QWidget *parent)
    : QWidget(parent)
{
    mChkAutoUpdate = new QCheckBox(tr("Automatically check for updates"));
    mChkPortable = new QCheckBox(tr("Portable mode"));

    QGroupBox *grpConfig = new QGroupBox(tr("Configuration"));
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(mChkAutoUpdate);
    vbox->addWidget(mChkPortable, Qt::AlignRight);
    vbox->addStretch(1);
    grpConfig->setLayout(vbox);

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

    QGroupBox *grpKeys = new QGroupBox(tr("Key Bindings"));
    QGridLayout *gbox = new QGridLayout;
    gbox->addWidget(btn0, 0, 0);
    gbox->addWidget(btn1, 0, 1);
    gbox->addWidget(btn2, 0, 2);
    gbox->addWidget(btn3, 1, 0);
    gbox->addWidget(btn4, 1, 1);
    gbox->addWidget(btn5, 1, 2);
    grpKeys->setLayout(gbox);

    QGroupBox *grpKeyColor = new QGroupBox(tr("Keypad Color"));
    QPushButton *btnColor = new QPushButton(tr("Change Keypad Color"));

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget(btnColor);
    grpKeyColor->setLayout(hbox);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(grpConfig);
    mainLayout->addWidget(grpKeys);
    mainLayout->addWidget(grpKeyColor);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    mKeypadColor = Settings::intOption(Settings::KeypadColor);
    connect(btnColor, &QPushButton::clicked, this, [this]
    {
        KeyColorDialog dialog(mKeypadColor);
        connect(&dialog, &KeyColorDialog::changedColor, this, &SettingsGeneralTab::changedKeypadColor);

        if (dialog.exec())
        {
            mKeypadColor = dialog.getColor();
        }
        emit changedKeypadColor(mKeypadColor);
    });

    int keymap = Settings::intOption(Settings::KeyMap);
    if (keymap < 0 || keymap > 5)
    {
        keymap = 1;
        Settings::setIntOption(Settings::KeyMap, keymap);
    }
    mKeybind->buttons().at(keymap)->setChecked(true);

    mChkAutoUpdate->setChecked(Settings::boolOption(Settings::AutoUpdate));
    mChkPortable->setChecked(Settings::boolOption(Settings::PortableMode));
}

void SettingsGeneralTab::saveSettings()
{
    Settings::setIntOption(Settings::KeypadColor, mKeypadColor);
    Settings::setIntOption(Settings::KeyMap, mKeybind->checkedId());
    Settings::setBoolOption(Settings::AutoUpdate, mChkAutoUpdate->isChecked());
    Settings::setBoolOption(Settings::PortableMode, mChkPortable->isChecked());
}

SettingsEmulationTab::SettingsEmulationTab(QWidget *parent)
    : QWidget(parent)
{
    mChkThrottle = new QCheckBox(tr("Throttle"));
    mSpnSpeed = new QSpinBox;

    mSpnSpeed->setMinimum(0);
    mSpnSpeed->setMaximum(5000);
    mSpnSpeed->setSuffix(QStringLiteral("%"));

    QGroupBox *grpSpeed = new QGroupBox(tr("Speed"));
    QHBoxLayout *hbox = new QHBoxLayout;
    QVBoxLayout *vbox = new QVBoxLayout;
    hbox->addWidget(mChkThrottle);
    hbox->addWidget(mSpnSpeed);
    vbox->addLayout(hbox);
    vbox->addStretch(1);
    grpSpeed->setLayout(vbox);

    mChkFrameSkip = new QCheckBox(tr("Frame skip"));
    mSpnFrameSkip = new QSpinBox;
    mChkLcdSpi = new QCheckBox(tr("Emulate LCD SPI"));

    mSpnFrameSkip->setMinimum(0);
    mSpnFrameSkip->setMaximum(99);
    mSpnFrameSkip->setEnabled(false);

    QGroupBox *grpDisplay = new QGroupBox(tr("Display"));
    QGridLayout *gbox = new QGridLayout;
    gbox->addWidget(mChkFrameSkip, 0, 0);
    gbox->addWidget(mSpnFrameSkip, 0, 1);
    gbox->addWidget(mChkLcdSpi, 1, 0);
    grpDisplay->setLayout(gbox);

    mChkPreI = new QCheckBox(tr("Emulate Pre-Revision I (IM 2)"));

    QGroupBox *grpEmu = new QGroupBox(tr("Emulation"));
    QVBoxLayout *vbo = new QVBoxLayout;
    vbo->addWidget(mChkPreI);
    vbo->addStretch(1);
    grpEmu->setLayout(vbo);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(grpSpeed);
    mainLayout->addWidget(grpDisplay);
    mainLayout->addWidget(grpEmu);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    connect(mChkThrottle, &QCheckBox::stateChanged, this, [this](int state)
    {
        mSpnSpeed->setEnabled(state == Qt::Checked);
    });

    connect(mChkFrameSkip, &QCheckBox::stateChanged, this, [this](int state)
    {
        mSpnFrameSkip->setEnabled(state == Qt::Checked);
    });

    int speed = Settings::intOption(Settings::EmuSpeed);
    if (speed > 5000 || speed < 0)
    {
        speed = 100;
        Settings::setIntOption(Settings::EmuSpeed, speed);
    }
    mSpnSpeed->setValue(speed);

    int frameskip = Settings::intOption(Settings::EmuFrameSkipRate);
    if (frameskip < 0 || frameskip > 99)
    {
        frameskip = 0;
        Settings::setIntOption(Settings::EmuFrameSkipRate, frameskip);
    }
    mSpnFrameSkip->setValue(frameskip);

    mChkThrottle->setChecked(Settings::boolOption(Settings::EmuThrottle));
    mChkFrameSkip->setChecked(Settings::boolOption(Settings::EmuFrameSkip));
    mChkLcdSpi->setChecked(Settings::boolOption(Settings::EmuLcdSpi));
    mChkPreI->setChecked(Settings::boolOption(Settings::EmuPreI));
}

void SettingsEmulationTab::saveSettings()
{
    Settings::setIntOption(Settings::EmuSpeed, mSpnSpeed->value());
    Settings::setIntOption(Settings::EmuFrameSkipRate, mSpnFrameSkip->value());
    Settings::setBoolOption(Settings::EmuFrameSkip, mChkFrameSkip->isChecked());
    Settings::setBoolOption(Settings::EmuLcdSpi, mChkLcdSpi->isChecked());
    Settings::setBoolOption(Settings::EmuPreI, mChkPreI->isChecked());
}

SettingsDeveloperTab::SettingsDeveloperTab(QWidget *parent)
    : QWidget(parent)
{
    mChkSoftCmds = new QCheckBox(tr("Enable soft commands"));
    mChkTiOs = new QCheckBox(tr("Assume ROM has TI-OS"));
    mChkResetNmi = new QCheckBox(tr("Open debugger on reset or NMI"));

    QGroupBox *grpConfig = new QGroupBox(tr("Configuration"));
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(mChkSoftCmds);
    vbox->addWidget(mChkTiOs);
    vbox->addWidget(mChkResetNmi);
    vbox->addStretch(1);
    grpConfig->setLayout(vbox);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(grpConfig);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    mChkSoftCmds->setChecked(Settings::boolOption(Settings::DevSoftCmds));
    mChkTiOs->setChecked(Settings::boolOption(Settings::DevTIOS));
    mChkResetNmi->setChecked(Settings::boolOption(Settings::DevOpenDebug));
}

void SettingsDeveloperTab::saveSettings()
{
    Settings::setBoolOption(Settings::DevSoftCmds, mChkSoftCmds->isChecked());
    Settings::setBoolOption(Settings::DevTIOS, mChkTiOs->isChecked());
    Settings::setBoolOption(Settings::DevOpenDebug, mChkResetNmi->isChecked());
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

    mColors->addButton(btnBlack, KeypadWidget::Color::Black);
    mColors->addButton(btnWhite, KeypadWidget::Color::White);
    mColors->addButton(btnTrueBlue, KeypadWidget::Color::TrueBlue);
    mColors->addButton(btnDenim, KeypadWidget::Color::Denim);
    mColors->addButton(btnSilver, KeypadWidget::Color::Silver);
    mColors->addButton(btnPink, KeypadWidget::Color::Pink);
    mColors->addButton(btnPlum, KeypadWidget::Color::Plum);
    mColors->addButton(btnRed, KeypadWidget::Color::Red);
    mColors->addButton(btnLightning, KeypadWidget::Color::Lightning);
    mColors->addButton(btnGold, KeypadWidget::Color::Gold);
    mColors->addButton(btnSpaceGrey, KeypadWidget::Color::SpaceGrey);
    mColors->addButton(btnCoral, KeypadWidget::Color::Coral);
    mColors->addButton(btnMint, KeypadWidget::Color::Mint);
    mColors->addButton(btnRoseGold, KeypadWidget::Color::RoseGold);
    mColors->addButton(btnCrystalClear, KeypadWidget::Color::CrystalClear);
    mColors->addButton(btnMatteBlack, KeypadWidget::Color::MatteBlack);
    mColors->addButton(btnTangentTeal, KeypadWidget::Color::TangentTeal);
    mColors->addButton(btnTotallyTeal, KeypadWidget::Color::TotallyTeal);

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

    connect(mColors, &QButtonGroup::idClicked, this, &KeyColorDialog::changedColor);
}

int KeyColorDialog::getColor()
{
    return mColors->checkedId();
}
