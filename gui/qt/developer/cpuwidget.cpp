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

#include "../corewrapper.h"
#include "../util.h"
#include "widgets/highlighteditwidget.h"

#include <cemucore.h>

#include <kddockwidgets/DockWidget.h>

#include <QtCore/QEvent>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QToolTip>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSpinBox>

CpuWidget::CpuWidget(CoreWindow *coreWindow)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("CPU")},
                   QIcon(QStringLiteral(":/assets/icons/electronics.svg")),
                   coreWindow}
{
    mRegEventFilter = new CpuRegisterFilter(this);

    QGroupBox *grpFlags = new QGroupBox(tr("Flags"));
    QGroupBox *grpReg = new QGroupBox(tr("Registers"));
    QGroupBox *grpRegPcSp = new QGroupBox(tr("PC/SP"));
    QGroupBox *grpState = new QGroupBox(tr("Mode/Interrupts"));
    QGroupBox *grpStack = new QGroupBox(tr("Stack"));

    QLabel *lblAF = new QLabel(QStringLiteral("af"));
    QLabel *lblBC = new QLabel(QStringLiteral("bc"));
    QLabel *lblDE = new QLabel(QStringLiteral("de"));
    QLabel *lblHL = new QLabel(QStringLiteral("hl"));
    QLabel *lblIX = new QLabel(QStringLiteral("ix"));
    QLabel *lblAFX = new QLabel(QStringLiteral("af'"));
    QLabel *lblBCX = new QLabel(QStringLiteral("bc'"));
    QLabel *lblDEX = new QLabel(QStringLiteral("de'"));
    QLabel *lblHLX = new QLabel(QStringLiteral("hl'"));
    QLabel *lblIY = new QLabel(QStringLiteral("iy "));
    QLabel *lblPC = new QLabel(QStringLiteral("pc "));
    QLabel *lblMB = new QLabel(QStringLiteral("mb "));
    QLabel *lblSPL = new QLabel(QStringLiteral("sps"));
    QLabel *lblSPS = new QLabel(QStringLiteral("spl"));
    QLabel *lblI = new QLabel(QStringLiteral("i "));
    QLabel *lblR = new QLabel(QStringLiteral("r "));
    QLabel *lblIM = new QLabel(QStringLiteral("im"));

    mEdtStack = new QPlainTextEdit;
    mChkAdlStack = new QCheckBox(QStringLiteral("ADL"));

    mChkS = new QCheckBox(QStringLiteral("s"));
    mChkZ = new QCheckBox(QStringLiteral("z"));
    mChk5 = new QCheckBox(QStringLiteral("5"));
    mChkH = new QCheckBox(QStringLiteral("h"));
    mChk3 = new QCheckBox(QStringLiteral("3"));
    mChkP = new QCheckBox(QStringLiteral("p/v"));
    mChkN = new QCheckBox(QStringLiteral("n"));
    mChkC = new QCheckBox(QStringLiteral("c"));

    mChkHalt = new QCheckBox(QStringLiteral("Halted"));
    mChkAdl = new QCheckBox(QStringLiteral("ADL"));
    mChkMadl = new QCheckBox(QStringLiteral("MADL"));
    mChkIef2 = new QCheckBox(QStringLiteral("IEF2"));
    mChkIef1 = new QCheckBox(QStringLiteral("IEF1"));

    mEdtAF = new HighlightEditWidget(">HHHH");
    mEdtBC = new HighlightEditWidget(">HHHHHH");
    mEdtDE = new HighlightEditWidget(">HHHHHH");
    mEdtHL = new HighlightEditWidget(">HHHHHH");
    mEdtIX = new HighlightEditWidget(">HHHHHH");
    mEdtAFX = new HighlightEditWidget(">HHHH");
    mEdtBCX = new HighlightEditWidget(">HHHHHH");
    mEdtDEX = new HighlightEditWidget(">HHHHHH");
    mEdtHLX = new HighlightEditWidget(">HHHHHH");
    mEdtIY = new HighlightEditWidget(">HHHHHH");
    mEdtPC = new HighlightEditWidget(">HHHHHH");
    mEdtMB = new HighlightEditWidget(">HH");
    mEdtSPL = new HighlightEditWidget(">HHHHHH");
    mEdtSPS = new HighlightEditWidget(">HHHH");
    mEdtI = new HighlightEditWidget(">HHHH");
    mEdtR = new HighlightEditWidget(">HH");
    mSpnIM = new QSpinBox;

    mSpnIM->setMinimum(0);
    mSpnIM->setMaximum(2);

    lblAF->setFont(Util::monospaceFont());
    lblBC->setFont(Util::monospaceFont());
    lblDE->setFont(Util::monospaceFont());
    lblHL->setFont(Util::monospaceFont());
    lblIX->setFont(Util::monospaceFont());
    lblAFX->setFont(Util::monospaceFont());
    lblBCX->setFont(Util::monospaceFont());
    lblDEX->setFont(Util::monospaceFont());
    lblHLX->setFont(Util::monospaceFont());
    lblIY->setFont(Util::monospaceFont());
    lblPC->setFont(Util::monospaceFont());
    lblMB->setFont(Util::monospaceFont());
    lblSPL->setFont(Util::monospaceFont());
    lblSPS->setFont(Util::monospaceFont());
    lblI->setFont(Util::monospaceFont());
    lblR->setFont(Util::monospaceFont());
    lblIM->setFont(Util::monospaceFont());

    mEdtStack->setFont(Util::monospaceFont());
    mEdtStack->setReadOnly(true);

    mEdtAF->setObjectName(QStringLiteral("afReg"));
    mEdtBC->setObjectName(QStringLiteral("bcReg"));
    mEdtDE->setObjectName(QStringLiteral("deReg"));
    mEdtHL->setObjectName(QStringLiteral("hlReg"));
    mEdtIX->setObjectName(QStringLiteral("ixReg"));
    mEdtAFX->setObjectName(QStringLiteral("afxReg"));
    mEdtBCX->setObjectName(QStringLiteral("bcxReg"));
    mEdtDEX->setObjectName(QStringLiteral("dexReg"));
    mEdtHLX->setObjectName(QStringLiteral("hlxReg"));
    mEdtIY->setObjectName(QStringLiteral("iyReg"));

    mEdtAF->installEventFilter(mRegEventFilter);
    mEdtBC->installEventFilter(mRegEventFilter);
    mEdtDE->installEventFilter(mRegEventFilter);
    mEdtHL->installEventFilter(mRegEventFilter);
    mEdtIX->installEventFilter(mRegEventFilter);
    mEdtAFX->installEventFilter(mRegEventFilter);
    mEdtBCX->installEventFilter(mRegEventFilter);
    mEdtDEX->installEventFilter(mRegEventFilter);
    mEdtHLX->installEventFilter(mRegEventFilter);
    mEdtIY->installEventFilter(mRegEventFilter);

    QHBoxLayout *hboxFlags = new QHBoxLayout;
    hboxFlags->setSizeConstraint(QLayout::SetMinimumSize);
    hboxFlags->addStretch(1);
    hboxFlags->addWidget(mChkS);
    hboxFlags->addStretch(1);
    hboxFlags->addWidget(mChkZ);
    hboxFlags->addStretch(1);
    hboxFlags->addWidget(mChk5);
    hboxFlags->addStretch(1);
    hboxFlags->addWidget(mChkH);
    hboxFlags->addStretch(1);
    hboxFlags->addWidget(mChk3);
    hboxFlags->addStretch(1);
    hboxFlags->addWidget(mChkP);
    hboxFlags->addStretch(1);
    hboxFlags->addWidget(mChkN);
    hboxFlags->addStretch(1);
    hboxFlags->addWidget(mChkC);
    hboxFlags->addStretch(1);
    grpFlags->setLayout(hboxFlags);

    QHBoxLayout *reglayout0 = new QHBoxLayout;
    reglayout0->addWidget(lblAF);
    reglayout0->addWidget(mEdtAF);

    QHBoxLayout *reglayout1 = new QHBoxLayout;
    reglayout1->addWidget(lblBC);
    reglayout1->addWidget(mEdtBC);

    QHBoxLayout *reglayout2 = new QHBoxLayout;
    reglayout2->addWidget(lblDE);
    reglayout2->addWidget(mEdtDE);

    QHBoxLayout *reglayout3 = new QHBoxLayout;
    reglayout3->addWidget(lblHL);
    reglayout3->addWidget(mEdtHL);

    QHBoxLayout *reglayout4 = new QHBoxLayout;
    reglayout4->addWidget(lblIX);
    reglayout4->addWidget(mEdtIX);

    QHBoxLayout *reglayout5 = new QHBoxLayout;
    reglayout5->addWidget(lblAFX);
    reglayout5->addWidget(mEdtAFX);

    QHBoxLayout *reglayout6 = new QHBoxLayout;
    reglayout6->addWidget(lblBCX);
    reglayout6->addWidget(mEdtBCX);

    QHBoxLayout *reglayout7 = new QHBoxLayout;
    reglayout7->addWidget(lblDEX);
    reglayout7->addWidget(mEdtDEX);

    QHBoxLayout *reglayout8 = new QHBoxLayout;
    reglayout8->addWidget(lblHLX);
    reglayout8->addWidget(mEdtHLX);

    QHBoxLayout *reglayout9 = new QHBoxLayout;
    reglayout9->addWidget(lblIY);
    reglayout9->addWidget(mEdtIY);

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
    reglayoutPc->addWidget(mEdtPC);

    QHBoxLayout *reglayoutMb = new QHBoxLayout;
    reglayoutMb->addWidget(lblMB);
    reglayoutMb->addWidget(mEdtMB);

    QHBoxLayout *reglayoutSpl = new QHBoxLayout;
    reglayoutSpl->addWidget(lblSPL);
    reglayoutSpl->addWidget(mEdtSPL);

    QHBoxLayout *reglayoutSps = new QHBoxLayout;
    reglayoutSps->addWidget(lblSPS);
    reglayoutSps->addWidget(mEdtSPS);

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

    QGridLayout *gridIm = new QGridLayout;
    gridIm->addWidget(mChkAdl, 0, 0);
    gridIm->addWidget(mChkMadl, 1, 0);
    gridIm->addWidget(mChkIef1, 0, 1);
    gridIm->addWidget(mChkIef2, 1, 1);
    gridIm->addWidget(mChkHalt, 2, 1);

    QHBoxLayout *reglayoutI = new QHBoxLayout;
    reglayoutI->addWidget(lblI);
    reglayoutI->addWidget(mEdtI);

    QHBoxLayout *reglayoutR = new QHBoxLayout;
    reglayoutR->addWidget(lblR);
    reglayoutR->addWidget(mEdtR);

    QHBoxLayout *reglayoutIm = new QHBoxLayout;
    reglayoutIm->addWidget(lblIM);
    reglayoutIm->addWidget(mSpnIM, Qt::AlignLeft);

    QVBoxLayout *vboxInt = new QVBoxLayout;
    vboxInt->addLayout(reglayoutI);
    vboxInt->addLayout(reglayoutR);
    vboxInt->addLayout(reglayoutIm);

    QHBoxLayout *hboxState = new QHBoxLayout;
    hboxState->addLayout(gridIm);
    hboxState->addLayout(vboxInt);
    grpState->setLayout(hboxState);

    QVBoxLayout *vboxState0 = new QVBoxLayout;
    vboxState0->addWidget(grpState);
    vboxState0->addStretch();

    QVBoxLayout *vboxStack = new QVBoxLayout;
    vboxStack->addWidget(mChkAdlStack);
    vboxStack->addWidget(mEdtStack);
    grpStack->setLayout(vboxStack);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addStretch();
    hLayout->addLayout(vboxState0);
    hLayout->addWidget(grpStack);
    hLayout->addStretch();

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(grpFlags);
    vLayout->addWidget(grpReg);
    vLayout->addLayout(hLayout);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addStretch(1);
    mainLayout->addLayout(vLayout);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(mChkS, &QCheckBox::toggled, this, &CpuWidget::setFlags);
    connect(mChkZ, &QCheckBox::toggled, this, &CpuWidget::setFlags);
    connect(mChk5, &QCheckBox::toggled, this, &CpuWidget::setFlags);
    connect(mChkH, &QCheckBox::toggled, this, &CpuWidget::setFlags);
    connect(mChk3, &QCheckBox::toggled, this, &CpuWidget::setFlags);
    connect(mChkP, &QCheckBox::toggled, this, &CpuWidget::setFlags);
    connect(mChkN, &QCheckBox::toggled, this, &CpuWidget::setFlags);
    connect(mChkC, &QCheckBox::toggled, this, &CpuWidget::setFlags);
    connect(mEdtAF, &HighlightEditWidget::textChanged, [this](const QString &text)
    {
        uint8_t flags = Util::hex2int(text) & 0xFF;

        mChkS->blockSignals(true);
        mChkS->setChecked(flags & (1 << 7));
        mChkS->blockSignals(false);
        mChkZ->blockSignals(true);
        mChkZ->setChecked(flags & (1 << 6));
        mChkZ->blockSignals(false);
        mChk5->blockSignals(true);
        mChk5->setChecked(flags & (1 << 5));
        mChk5->blockSignals(false);
        mChkH->blockSignals(true);
        mChkH->setChecked(flags & (1 << 4));
        mChkH->blockSignals(false);
        mChk3->blockSignals(true);
        mChk3->setChecked(flags & (1 << 3));
        mChk3->blockSignals(false);
        mChkP->blockSignals(true);
        mChkP->setChecked(flags & (1 << 2));
        mChkP->blockSignals(false);
        mChkN->blockSignals(true);
        mChkN->setChecked(flags & (1 << 1));
        mChkN->blockSignals(false);
        mChkC->blockSignals(true);
        mChkC->setChecked(flags & (1 << 0));
        mChkC->blockSignals(false);
    });
}

void CpuWidget::setFlags()
{
    uint8_t flags = 0;

    flags |= mChkS->isChecked() ? 1 << 7 : 0;
    flags |= mChkZ->isChecked() ? 1 << 6 : 0;
    flags |= mChk5->isChecked() ? 1 << 5 : 0;
    flags |= mChkH->isChecked() ? 1 << 4 : 0;
    flags |= mChk3->isChecked() ? 1 << 3 : 0;
    flags |= mChkP->isChecked() ? 1 << 2 : 0;
    flags |= mChkN->isChecked() ? 1 << 1 : 0;
    flags |= mChkC->isChecked() ? 1 << 0 : 0;

    mEdtAF->blockSignals(true);
    mEdtAF->setInt((mEdtAF->getInt() & 0xFF00) | flags, 4);
    mEdtAF->blockSignals(false);
}

bool CpuRegisterFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseMove)
    {
        QString name = obj->objectName();
        HighlightEditWidget *widget = static_cast<HighlightEditWidget *>(obj);

        if (!widget->isEnabled())
        {
            return QObject::eventFilter(obj, event);
        }

        unsigned int num  = Util::hex2int(widget->text());
        unsigned int num0 = num & 255;
        unsigned int num1 = (num >> 8) & 255;
        unsigned int num2 = (num >> 16) & 255;

        QString t;
        QString val  = QString::number(num);
        QString val0 = QString::number(num0);
        QString val1 = QString::number(num1);
        QString val2 = QString::number(num2);

        if (num  > 0x7FFFFF)
        {
            val += QStringLiteral("\n\t") + QString::number(static_cast<int32_t>(num | 0xFF000000u));
        }
        if (num0 > 0x7F)
        {
            val0 += QStringLiteral("\t") + QString::number(static_cast<int8_t>(num0));
        }
        if (num1 > 0x7F)
        {
            val1 += QStringLiteral("\t") + QString::number(static_cast<int8_t>(num1));
        }
        if (num2 > 0x7F)
        {
            val2 += QStringLiteral("\t") + QString::number(static_cast<int8_t>(num2));
        }

        if (name == QStringLiteral("afReg"))
        {
            t = QStringLiteral("a:\t") + val1 +
                QStringLiteral("\nf:\t") + val0;
        }
        else if (name == QStringLiteral("hlReg"))
        {
            t = QStringLiteral("hl:\t") + val +
                QStringLiteral("\nu:\t") + val2 +
                QStringLiteral("\nh:\t") + val1 +
                QStringLiteral("\nl:\t") + val0;
        }
        else if (name == QStringLiteral("deReg"))
        {
            t = QStringLiteral("de:\t") + val +
                QStringLiteral("\nu:\t") + val2 +
                QStringLiteral("\nd:\t") + val1 +
                QStringLiteral("\ne:\t") + val0;
        }
        else if (name == QStringLiteral("bcReg"))
        {
            t = QStringLiteral("bc:\t") + val +
                QStringLiteral("\nu:\t") + val2 +
                QStringLiteral("\nb:\t") + val1 +
                QStringLiteral("\nc:\t") + val0;
        }
        else if (name == QStringLiteral("ixReg"))
        {
            t = QStringLiteral("ix:\t") + val +
                QStringLiteral("\nixh:\t") + val1 +
                QStringLiteral("\nixl:\t") + val0;
        }
        else if (name == QStringLiteral("iyReg"))
        {
            t = QStringLiteral("iy:\t") + val +
                QStringLiteral("\niyh:\t") + val1 +
                QStringLiteral("\niyl:\t") + val0;
        }
        else if (name == QStringLiteral("afxReg"))
        {
            t = QStringLiteral("a':\t") + val1 +
                QStringLiteral("\nf':\t") + val0;
        }
        else if (name == QStringLiteral("hlxReg"))
        {
            t = QStringLiteral("hl':\t") + val +
                QStringLiteral("\nu':\t") + val2 +
                QStringLiteral("\nh':\t") + val1 +
                QStringLiteral("\nl':\t") + val0;
        }
        else if (name == QStringLiteral("dexReg"))
        {
            t = QStringLiteral("de':\t") + val +
                QStringLiteral("\nu':\t") + val2 +
                QStringLiteral("\nd':\t") + val1 +
                QStringLiteral("\ne':\t") + val0;
        }
        else if (name == QStringLiteral("bcxReg"))
        {
            t = QStringLiteral("bc':\t") + val +
                QStringLiteral("\nu':\t") + val2 +
                QStringLiteral("\nb':\t") + val1 +
                QStringLiteral("\nc':\t") + val0;
        }
        else
        {
            return QObject::eventFilter(obj, event);
        }

        QToolTip::showText(widget->mapToGlobal({0, 5}), t, widget);
    }

    return QObject::eventFilter(obj, event);
}

void CpuWidget::loadFromCore(const CoreWrapper &core)
{
    mEdtI->setInt(core.get(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_I), 4);
    mEdtR->setInt(core.get(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_R), 2);
    mEdtMB->setInt(core.get(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_MB), 2);
    mEdtAF->setInt(core.get(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_AF), 4);
    mEdtBC->setInt(core.get(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_UBC), 6);
    mEdtDE->setInt(core.get(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_UDE), 6);
    mEdtHL->setInt(core.get(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_UHL), 6);
    mEdtIX->setInt(core.get(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_UIX), 6);
    mEdtIY->setInt(core.get(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_UIY), 6);
    mEdtPC->setInt(core.get(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_PC), 6);
    mEdtSPS->setInt(core.get(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_SPS), 4);
    mEdtSPL->setInt(core.get(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_SPL), 6);
    mEdtAFX->setInt(core.get(cemucore::CEMUCORE_PROP_REG_SHADOW, cemucore::CEMUCORE_REG_AF), 4);
    mEdtBCX->setInt(core.get(cemucore::CEMUCORE_PROP_REG_SHADOW, cemucore::CEMUCORE_REG_UBC), 6);
    mEdtDEX->setInt(core.get(cemucore::CEMUCORE_PROP_REG_SHADOW, cemucore::CEMUCORE_REG_UDE), 6);
    mEdtHLX->setInt(core.get(cemucore::CEMUCORE_PROP_REG_SHADOW, cemucore::CEMUCORE_REG_UHL), 6);
}

void CpuWidget::storeToCore(CoreWrapper &core) const
{
    core.set(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_I, mEdtI->getInt());
    core.set(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_R, mEdtR->getInt());
    core.set(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_MB, mEdtMB->getInt());
    core.set(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_AF, mEdtR->getInt());
    core.set(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_UBC, mEdtR->getInt());
    core.set(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_UDE, mEdtR->getInt());
    core.set(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_UHL, mEdtR->getInt());
    core.set(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_UIX, mEdtR->getInt());
    core.set(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_UIY, mEdtR->getInt());
    core.set(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_PC, mEdtR->getInt());
    core.set(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_SPS, mEdtR->getInt());
    core.set(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_SPL, mEdtR->getInt());
    core.set(cemucore::CEMUCORE_PROP_REG_SHADOW, cemucore::CEMUCORE_REG_AF, mEdtAFX->getInt());
    core.set(cemucore::CEMUCORE_PROP_REG_SHADOW, cemucore::CEMUCORE_REG_UBC, mEdtBCX->getInt());
    core.set(cemucore::CEMUCORE_PROP_REG_SHADOW, cemucore::CEMUCORE_REG_UDE, mEdtDEX->getInt());
    core.set(cemucore::CEMUCORE_PROP_REG_SHADOW, cemucore::CEMUCORE_REG_UHL, mEdtHLX->getInt());
}
