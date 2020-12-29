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

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSpinBox>

DevMiscWidget::DevMiscWidget(QWidget *parent)
    : QWidget{parent}
{
#ifdef Q_OS_WIN
    QFont monospaceFont(QStringLiteral("Courier"), 10);
#else
    QFont monospaceFont(QStringLiteral("Monospace"), 10);
#endif

    QGroupBox *grpLcd = new QGroupBox(QStringLiteral("LCD"));
    QGroupBox *grpLcdCtl = new QGroupBox(tr("Control"));
    QGroupBox *grpLcdReg = new QGroupBox(tr("Registers"));
    QGroupBox *grpBat = new QGroupBox(tr("Battery"));

    QCheckBox *chkLcdPwr = new QCheckBox(QStringLiteral("pwr"));
    QCheckBox *chkLcdBgr = new QCheckBox(QStringLiteral("bgr"));
    QCheckBox *chkLcdBepo = new QCheckBox(QStringLiteral("bepo"));
    QCheckBox *chkLcdBebo = new QCheckBox(QStringLiteral("bebo"));

    QCheckBox *chkBatCharge = new QCheckBox(QStringLiteral("Charging"));
    QSpinBox *spnBatLevel = new QSpinBox;

    QLabel *lblLcdBase = new QLabel(tr("Base"));
    QLabel *lblLcdCurr = new QLabel(tr("Current"));
    QLabel *lblLcdBright = new QLabel(tr("Brightness"));
    QLabel *lblBatLevel = new QLabel(tr("Level"));

    QLineEdit *edtlcdBase = new QLineEdit(QStringLiteral("000000"));
    QLineEdit *edtlcdCurr = new QLineEdit(QStringLiteral("000000"));

    lblLcdBase->setFont(monospaceFont);
    lblLcdCurr->setFont(monospaceFont);
    lblLcdBright->setFont(monospaceFont);
    lblBatLevel->setFont(monospaceFont);

    edtlcdBase->setFont(monospaceFont);
    edtlcdCurr->setFont(monospaceFont);

    spnBatLevel->setMinimum(0);
    spnBatLevel->setMaximum(5);

    QGridLayout *gridLcdCtl = new QGridLayout;
    gridLcdCtl->addWidget(chkLcdPwr, 0, 0);
    gridLcdCtl->addWidget(chkLcdBgr, 0, 1);
    gridLcdCtl->addWidget(chkLcdBepo, 1, 0);
    gridLcdCtl->addWidget(chkLcdBebo, 1, 1);
    grpLcdCtl->setLayout(gridLcdCtl);

    QGridLayout *gridLcdReg = new QGridLayout;
    gridLcdReg->addWidget(lblLcdBase, 0, 0);
    gridLcdReg->addWidget(lblLcdCurr, 1, 0);
    gridLcdReg->addWidget(edtlcdBase, 0, 1);
    gridLcdReg->addWidget(edtlcdCurr, 1, 1);
    grpLcdReg->setLayout(gridLcdReg);

    QHBoxLayout *hboxLcd = new QHBoxLayout;
    hboxLcd->addWidget(grpLcdCtl);
    hboxLcd->addWidget(grpLcdReg);
    grpLcd->setLayout(hboxLcd);

    QHBoxLayout *hboxBat = new QHBoxLayout;
    hboxBat->addWidget(chkBatCharge);
    hboxBat->addStretch(1);
    hboxBat->addWidget(lblBatLevel);
    hboxBat->addWidget(spnBatLevel);
    grpBat->setLayout(hboxBat);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addStretch(1);
    vLayout->addWidget(grpLcd);
    vLayout->addWidget(grpBat);
    vLayout->addStretch(1);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addStretch(1);
    hLayout->addLayout(vLayout);
    hLayout->addStretch(1);
    setLayout(hLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
