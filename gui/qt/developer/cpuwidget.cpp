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

#include "cpuwidget.h"

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSizePolicy>

CpuWidget::CpuWidget(QWidget *parent)
    : QWidget{parent}
{
#ifdef Q_OS_WIN
    QFont monospaceFont(QStringLiteral("Courier"), 10);
#else
    QFont monospaceFont(QStringLiteral("Monospace"), 10);
#endif

    QGroupBox *grpFlags = new QGroupBox(tr("Flags"));
    QGroupBox *grpReg = new QGroupBox(tr("Registers"));
    QGroupBox *grpRegPcSp = new QGroupBox(tr("PC/SP"));
    QGroupBox *grpMode = new QGroupBox(tr("Mode"));
    QGroupBox *grpState = new QGroupBox(tr("Mode/Interrupts"));

    QCheckBox *chkS = new QCheckBox(QStringLiteral("s"));
    QCheckBox *chkZ = new QCheckBox(QStringLiteral("z"));
    QCheckBox *chk5 = new QCheckBox(QStringLiteral("5"));
    QCheckBox *chkh = new QCheckBox(QStringLiteral("h"));
    QCheckBox *chk3 = new QCheckBox(QStringLiteral("3"));
    QCheckBox *chkPV = new QCheckBox(QStringLiteral("p/v"));
    QCheckBox *chkN = new QCheckBox(QStringLiteral("n"));
    QCheckBox *chkC = new QCheckBox(QStringLiteral("c"));

    QLabel *lblAF = new QLabel(QStringLiteral("af"));
    QLabel *lblBC = new QLabel(QStringLiteral("bc"));
    QLabel *lblDE = new QLabel(QStringLiteral("de"));
    QLabel *lblHL = new QLabel(QStringLiteral("hl"));
    QLabel *lblIX = new QLabel(QStringLiteral("ix"));
    QLabel *lblAFX = new QLabel(QStringLiteral("af\'"));
    QLabel *lblBCX = new QLabel(QStringLiteral("bc\'"));
    QLabel *lblDEX = new QLabel(QStringLiteral("de\'"));
    QLabel *lblHLX = new QLabel(QStringLiteral("hl\'"));
    QLabel *lblIY = new QLabel(QStringLiteral("iy "));
    QLabel *lblPC = new QLabel(QStringLiteral("pc "));
    QLabel *lblMB = new QLabel(QStringLiteral("mb "));
    QLabel *lblSPL = new QLabel(QStringLiteral("sps"));
    QLabel *lblSPS = new QLabel(QStringLiteral("spl"));
    QLabel *lblI = new QLabel(QStringLiteral("i "));
    QLabel *lblR = new QLabel(QStringLiteral("r "));
    QLabel *lblIM = new QLabel(QStringLiteral("im"));

    QLineEdit *edtAF = new QLineEdit(QStringLiteral("0000"));
    QLineEdit *edtBC = new QLineEdit(QStringLiteral("000000"));
    QLineEdit *edtDE = new QLineEdit(QStringLiteral("000000"));
    QLineEdit *edtHL = new QLineEdit(QStringLiteral("000000"));
    QLineEdit *edtIX = new QLineEdit(QStringLiteral("000000"));
    QLineEdit *edtAFX = new QLineEdit(QStringLiteral("0000"));
    QLineEdit *edtBCX = new QLineEdit(QStringLiteral("000000"));
    QLineEdit *edtDEX = new QLineEdit(QStringLiteral("000000"));
    QLineEdit *edtHLX = new QLineEdit(QStringLiteral("000000"));
    QLineEdit *edtIY = new QLineEdit(QStringLiteral("000000"));
    QLineEdit *edtPC = new QLineEdit(QStringLiteral("000000"));
    QLineEdit *edtMB = new QLineEdit(QStringLiteral("00"));
    QLineEdit *edtSPL = new QLineEdit(QStringLiteral("000000"));
    QLineEdit *edtSPS = new QLineEdit(QStringLiteral("0000"));
    QLineEdit *edtI = new QLineEdit(QStringLiteral("0000"));
    QLineEdit *edtR = new QLineEdit(QStringLiteral("00"));
    QLineEdit *edtIM = new QLineEdit(QStringLiteral("1"));

    lblAF->setFont(monospaceFont);
    lblBC->setFont(monospaceFont);
    lblDE->setFont(monospaceFont);
    lblHL->setFont(monospaceFont);
    lblIX->setFont(monospaceFont);
    lblAFX->setFont(monospaceFont);
    lblBCX->setFont(monospaceFont);
    lblDEX->setFont(monospaceFont);
    lblHLX->setFont(monospaceFont);
    lblIY->setFont(monospaceFont);
    lblPC->setFont(monospaceFont);
    lblMB->setFont(monospaceFont);
    lblSPL->setFont(monospaceFont);
    lblSPS->setFont(monospaceFont);
    lblI->setFont(monospaceFont);
    lblR->setFont(monospaceFont);
    lblIM->setFont(monospaceFont);

    edtAF->setFont(monospaceFont);
    edtBC->setFont(monospaceFont);
    edtDE->setFont(monospaceFont);
    edtHL->setFont(monospaceFont);
    edtIX->setFont(monospaceFont);
    edtAFX->setFont(monospaceFont);
    edtBCX->setFont(monospaceFont);
    edtDEX->setFont(monospaceFont);
    edtHLX->setFont(monospaceFont);
    edtIY->setFont(monospaceFont);
    edtPC->setFont(monospaceFont);
    edtMB->setFont(monospaceFont);
    edtSPL->setFont(monospaceFont);
    edtSPS->setFont(monospaceFont);
    edtI->setFont(monospaceFont);
    edtR->setFont(monospaceFont);
    edtIM->setFont(monospaceFont);

    QHBoxLayout *hboxFlags = new QHBoxLayout;
    hboxFlags->setSizeConstraint(QLayout::SetMinimumSize);
    hboxFlags->addStretch(1);
    hboxFlags->addWidget(chkS);
    hboxFlags->addStretch(1);
    hboxFlags->addWidget(chkZ);
    hboxFlags->addStretch(1);
    hboxFlags->addWidget(chk5);
    hboxFlags->addStretch(1);
    hboxFlags->addWidget(chkh);
    hboxFlags->addStretch(1);
    hboxFlags->addWidget(chk3);
    hboxFlags->addStretch(1);
    hboxFlags->addWidget(chkPV);
    hboxFlags->addStretch(1);
    hboxFlags->addWidget(chkN);
    hboxFlags->addStretch(1);
    hboxFlags->addWidget(chkC);
    hboxFlags->addStretch(1);
    grpFlags->setLayout(hboxFlags);

    QHBoxLayout *reglayout0 = new QHBoxLayout;
    reglayout0->addWidget(lblAF);
    reglayout0->addWidget(edtAF);

    QHBoxLayout *reglayout1 = new QHBoxLayout;
    reglayout1->addWidget(lblBC);
    reglayout1->addWidget(edtBC);

    QHBoxLayout *reglayout2 = new QHBoxLayout;
    reglayout2->addWidget(lblDE);
    reglayout2->addWidget(edtDE);

    QHBoxLayout *reglayout3 = new QHBoxLayout;
    reglayout3->addWidget(lblHL);
    reglayout3->addWidget(edtHL);

    QHBoxLayout *reglayout4 = new QHBoxLayout;
    reglayout4->addWidget(lblIX);
    reglayout4->addWidget(edtIX);

    QHBoxLayout *reglayout5 = new QHBoxLayout;
    reglayout5->addWidget(lblAFX);
    reglayout5->addWidget(edtAFX);

    QHBoxLayout *reglayout6 = new QHBoxLayout;
    reglayout6->addWidget(lblBCX);
    reglayout6->addWidget(edtBCX);

    QHBoxLayout *reglayout7 = new QHBoxLayout;
    reglayout7->addWidget(lblDEX);
    reglayout7->addWidget(edtDEX);

    QHBoxLayout *reglayout8 = new QHBoxLayout;
    reglayout8->addWidget(lblHLX);
    reglayout8->addWidget(edtHLX);

    QHBoxLayout *reglayout9 = new QHBoxLayout;
    reglayout9->addWidget(lblIY);
    reglayout9->addWidget(edtIY);

    QGridLayout *gridRegs = new QGridLayout;
    gridRegs->addLayout(reglayout0, 0, 0);
    gridRegs->addLayout(reglayout1, 1, 0);
    gridRegs->addLayout(reglayout2, 2, 0);
    gridRegs->addLayout(reglayout3, 3, 0);
    gridRegs->addLayout(reglayout4, 4, 0);
    gridRegs->addLayout(reglayout5, 0, 1);
    gridRegs->addLayout(reglayout6, 1, 1);
    gridRegs->addLayout(reglayout7, 2, 1);
    gridRegs->addLayout(reglayout8, 3, 1);
    gridRegs->addLayout(reglayout9, 4, 1);

    QHBoxLayout *reglayoutPc = new QHBoxLayout;
    reglayoutPc->addWidget(lblPC);
    reglayoutPc->addWidget(edtPC);

    QHBoxLayout *reglayoutMb = new QHBoxLayout;
    reglayoutMb->addWidget(lblMB);
    reglayoutMb->addWidget(edtMB);

    QHBoxLayout *reglayoutSpl = new QHBoxLayout;
    reglayoutSpl->addWidget(lblSPL);
    reglayoutSpl->addWidget(edtSPL);

    QHBoxLayout *reglayoutSps = new QHBoxLayout;
    reglayoutSps->addWidget(lblSPS);
    reglayoutSps->addWidget(edtSPS);

    QVBoxLayout *vboxRegs = new QVBoxLayout;
    vboxRegs->addLayout(reglayoutPc);
    vboxRegs->addLayout(reglayoutMb);
    vboxRegs->addLayout(reglayoutSpl);
    vboxRegs->addLayout(reglayoutSps);

    grpRegPcSp->setLayout(vboxRegs);

    QHBoxLayout *hboxRegs = new QHBoxLayout;
    hboxRegs->addLayout(gridRegs, Qt::AlignRight);
    hboxRegs->addWidget(grpRegPcSp, Qt::AlignLeft);
    grpReg->setLayout(hboxRegs);

    QCheckBox *chkHalt = new QCheckBox(QStringLiteral("Halted"));
    QCheckBox *chkAdl = new QCheckBox(QStringLiteral("ADL"));
    QCheckBox *chkMadl = new QCheckBox(QStringLiteral("MADL"));
    QCheckBox *chkIef2 = new QCheckBox(QStringLiteral("IEF2"));
    QCheckBox *chkIef1 = new QCheckBox(QStringLiteral("IEF1"));

    QVBoxLayout *vboxMode = new QVBoxLayout;
    vboxMode->addWidget(chkAdl);
    vboxMode->addWidget(chkMadl);
    grpMode->setLayout(vboxMode);

    QVBoxLayout *vboxIff = new QVBoxLayout;
    vboxIff->addWidget(chkIef1);
    vboxIff->addWidget(chkIef2);
    vboxIff->addWidget(chkHalt);

    QHBoxLayout *reglayoutI = new QHBoxLayout;
    reglayoutI->addWidget(lblI);
    reglayoutI->addWidget(edtI);

    QHBoxLayout *reglayoutR = new QHBoxLayout;
    reglayoutR->addWidget(lblR);
    reglayoutR->addWidget(edtR);

    QHBoxLayout *reglayoutIm = new QHBoxLayout;
    reglayoutIm->addWidget(lblIM);
    reglayoutIm->addWidget(edtIM);

    QVBoxLayout *vboxInt = new QVBoxLayout;
    vboxInt->addLayout(reglayoutI);
    vboxInt->addLayout(reglayoutR);
    vboxInt->addLayout(reglayoutIm);

    QHBoxLayout *hboxState = new QHBoxLayout;
    hboxState->addStretch(1);
    hboxState->addWidget(grpMode);
    hboxState->addLayout(vboxIff);
    hboxState->addLayout(vboxInt);
    hboxState->addStretch(1);
    grpState->setLayout(hboxState);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addStretch(1);
    vLayout->addWidget(grpFlags);
    vLayout->addWidget(grpReg);
    vLayout->addWidget(grpState);
    vLayout->addStretch(1);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addStretch(1);
    mainLayout->addLayout(vLayout);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
