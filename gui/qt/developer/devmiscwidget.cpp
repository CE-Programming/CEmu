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

#include "devmiscwidget.h"

#include "../util.h"
#include "widgets/highlighteditwidget.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSpinBox>

DevMiscWidget::DevMiscWidget(CoreWindow *coreWindow)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("Miscellaneous")},
                   QIcon(QStringLiteral(":/assets/icons/charge_battery.svg")),
                   coreWindow}
{
    QGroupBox *grpLcd = new QGroupBox(QStringLiteral("LCD"));
    QGroupBox *grpLcdCtl = new QGroupBox(tr("Control"));
    QGroupBox *grpLcdReg = new QGroupBox(tr("Registers"));
    QGroupBox *grpBat = new QGroupBox(tr("Battery"));
    QGroupBox *grpBacklight = new QGroupBox(tr("Backlight"));

    QLabel *lblLcdBase = new QLabel(tr("Base"));
    QLabel *lblLcdCurr = new QLabel(tr("Current"));
    QLabel *lblBatLevel = new QLabel(tr("Battery Level"));
    QLabel *lblBacklightLevel = new QLabel(tr("Backlight Level"));

    lblLcdBase->setFont(Util::monospaceFont());
    lblLcdCurr->setFont(Util::monospaceFont());

    mChkLcdPwr = new QCheckBox(QStringLiteral("pwr"));
    mChkLcdBgr = new QCheckBox(QStringLiteral("bgr"));
    mChkLcdBepo = new QCheckBox(QStringLiteral("bepo"));
    mChkLcdBebo = new QCheckBox(QStringLiteral("bebo"));
    mChkBacklightEnable = new QCheckBox(QStringLiteral("Enabled"));
    mChkBatCharge = new QCheckBox(QStringLiteral("Charging"));
    mSpnBatLevel = new QSpinBox;
    mSpnBacklightLevel = new QSpinBox;

    mEdtLcdBase = new HighlightEditWidget(">HHHHHH");
    mEdtLcdCurr = new HighlightEditWidget(">HHHHHH");

    mSpnBatLevel->setMinimum(0);
    mSpnBatLevel->setMaximum(5);

    mSpnBacklightLevel->setMinimum(0);
    mSpnBacklightLevel->setMaximum(255);

    QGridLayout *gridLcdCtl = new QGridLayout;
    gridLcdCtl->addWidget(mChkLcdPwr, 0, 0);
    gridLcdCtl->addWidget(mChkLcdBgr, 0, 1);
    gridLcdCtl->addWidget(mChkLcdBepo, 1, 0);
    gridLcdCtl->addWidget(mChkLcdBebo, 1, 1);
    grpLcdCtl->setLayout(gridLcdCtl);

    QGridLayout *gridLcdReg = new QGridLayout;
    gridLcdReg->addWidget(lblLcdBase, 0, 0);
    gridLcdReg->addWidget(lblLcdCurr, 1, 0);
    gridLcdReg->addWidget(mEdtLcdBase, 0, 1);
    gridLcdReg->addWidget(mEdtLcdCurr, 1, 1);
    grpLcdReg->setLayout(gridLcdReg);

    QHBoxLayout *hboxLcd = new QHBoxLayout;
    hboxLcd->addWidget(grpLcdCtl);
    hboxLcd->addWidget(grpLcdReg);
    grpLcd->setLayout(hboxLcd);

    QHBoxLayout *hboxBat = new QHBoxLayout;
    hboxBat->addWidget(mChkBatCharge);
    hboxBat->addStretch();
    hboxBat->addWidget(lblBatLevel);
    hboxBat->addWidget(mSpnBatLevel);
    grpBat->setLayout(hboxBat);

    QHBoxLayout *hboxBacklight = new QHBoxLayout;
    hboxBacklight->addWidget(mChkBacklightEnable);
    hboxBacklight->addStretch();
    hboxBacklight->addWidget(lblBacklightLevel);
    hboxBacklight->addWidget(mSpnBacklightLevel);
    grpBacklight->setLayout(hboxBacklight);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addStretch();
    vLayout->addWidget(grpLcd);
    vLayout->addWidget(grpBacklight);
    vLayout->addWidget(grpBat);
    vLayout->addStretch();

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addStretch();
    hLayout->addLayout(vLayout);
    hLayout->addStretch();
    setLayout(hLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void DevMiscWidget::saveState()
{

}

void DevMiscWidget::loadState()
{

}
