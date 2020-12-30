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

#include "breakpointswidget.h"
#include "util.h"

#include <QtGui/QColor>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QSizePolicy>

const QString BreakpointsWidget::mEnabledText = tr("Yes");
const QString BreakpointsWidget::mDisabledText = tr("No");

BreakpointsWidget::BreakpointsWidget(const QList<Breakpoint> &breakpoints, DevWidget *parent)
    : DevWidget{parent},
      mBpNum{0}
{
    mTbl = new QTableWidget(0, 3);
    mTbl->setHorizontalHeaderLabels({tr("Enabled"), tr("Address"), tr("Name")});
    mTbl->horizontalHeader()->setStretchLastSection(true);
    mTbl->verticalHeader()->setVisible(false);
    mTbl->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    mTbl->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mTbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTbl->setDragDropMode(QAbstractItemView::InternalMove);
    mTbl->setDragDropOverwriteMode(false);
    mTbl->setDragEnabled(true);
    mTbl->setAcceptDrops(true);
    mTbl->setDefaultDropAction(Qt::IgnoreAction);
    mTbl->setSortingEnabled(false);
    mTbl->setDropIndicatorShown(true);

    mBtnToggleSelected = new QPushButton(tr("Toggle selected"));
    mBtnRemoveSelected = new QPushButton(tr("Remove selected"));

    QPushButton *btnAddBreakpoint = new QPushButton(tr("Add breakpoint"));

    mNormalBackground = QTableWidgetItem().background();

    QHBoxLayout *hboxBtns = new QHBoxLayout;
    hboxBtns->addWidget(btnAddBreakpoint);
    hboxBtns->addStretch(1);
    hboxBtns->addWidget(mBtnToggleSelected);
    hboxBtns->addWidget(mBtnRemoveSelected);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(hboxBtns);
    vLayout->addWidget(mTbl);
    setLayout(vLayout);

    foreach (const Breakpoint &breakpoint, breakpoints)
    {
        addBreakpoint(breakpoint, false);
    }

    connect(btnAddBreakpoint, &QPushButton::clicked, [this]
    {
        Breakpoint breakpoint = { false, 0, QStringLiteral("bp") + QString::number(mBpNum) };
        addBreakpoint(breakpoint, true);
        mBpNum++;
    });

    connect(mBtnToggleSelected, &QPushButton::clicked, this, &BreakpointsWidget::toggleSelected);
    connect(mBtnRemoveSelected, &QPushButton::clicked, this, &BreakpointsWidget::removeSelected);
    connect(mTbl, &QTableWidget::itemChanged, this, &BreakpointsWidget::itemChanged);
    connect(mTbl, &QTableWidget::itemActivated, this, &BreakpointsWidget::itemActivated);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void BreakpointsWidget::addBreakpoint(const Breakpoint &breakpoint, bool edit)
{
    QComboBox *cmbEnabled = new QComboBox;
    cmbEnabled->addItems({ mEnabledText, mDisabledText });

    QString addrStr = Util::int2hex(breakpoint.addr, Util::AddrByteWidth);

    if (mTbl->rowCount() == 0)
    {
        mBtnToggleSelected->setEnabled(true);
        mBtnRemoveSelected->setEnabled(true);
    }

    QTableWidgetItem *addr = new QTableWidgetItem(addrStr);
    QTableWidgetItem *name = new QTableWidgetItem(breakpoint.name);

    mTbl->blockSignals(true);
    mTbl->insertRow(0);
    mTbl->setCellWidget(0, Column::Enabled, cmbEnabled);
    mTbl->setItem(0, Column::Address, addr);
    mTbl->setItem(0, Column::Name, name);
    mTbl->blockSignals(false);

    if (edit)
    {
        mTbl->editItem(addr);
    }

    setBreakpoint(0, false);
}

void BreakpointsWidget::setBreakpoint(int row, bool enable)
{
    QComboBox *cmbEnabled = static_cast<QComboBox *>(mTbl->cellWidget(row, Column::Enabled));

    cmbEnabled->setCurrentIndex(enable ? 0 : 1);
}

void BreakpointsWidget::toggleSelected()
{
    Q_ASSERT(mTbl->rowCount() != 0);

    for (int i = mTbl->rowCount() - 1; i >= 0; --i)
    {
        QTableWidgetItem *item = mTbl->item(i, Column::Address);
        if (item->isSelected())
        {
            if (Util::isHexAddress(item->text()))
            {
                QComboBox *cmbEnabled = static_cast<QComboBox *>(mTbl->cellWidget(i, Column::Enabled));
                setBreakpoint(i, !(cmbEnabled->currentIndex() == 0));
            }
            else
            {
                setBreakpoint(i, false);
            }
        }
    }
}

void BreakpointsWidget::removeSelected()
{
    Q_ASSERT(mTbl->rowCount() != 0);

    for (int i = mTbl->rowCount() - 1; i >= 0; --i)
    {
        if (mTbl->item(i, Column::Address)->isSelected())
        {
            mTbl->removeRow(i);
        }
    }

    if (mTbl->rowCount() == 0)
    {
        mBtnToggleSelected->setEnabled(false);
        mBtnRemoveSelected->setEnabled(false);
    }
}

void BreakpointsWidget::itemActivated(QTableWidgetItem *item)
{
    switch (item->column())
    {
        default:
            break;
        case Column::Address:
            mPrevAddr = item->text();
            break;
    }
}

void BreakpointsWidget::itemChanged(QTableWidgetItem *item)
{
    int row = item->row();

    switch (item->column())
    {
        default:
            break;

        case Column::Address:
            if (Util::isHexAddress(item->text()))
            {
                setBreakpoint(row, true);
                item->setBackground(mNormalBackground);
                blockSignals(true);
                mTbl->cellWidget(row, Column::Enabled)->setEnabled(true);
                item->setText(Util::int2hex(Util::hex2int(item->text()), Util::AddrByteWidth));
                blockSignals(false);
            }
            else
            {
                setBreakpoint(row, false);
                item->setBackground(QBrush(QColor(Qt::red).lighter()));
                mTbl->cellWidget(row, Column::Enabled)->setEnabled(false);
            }
            break;

        case Column::Name:
            break;
    }
}
