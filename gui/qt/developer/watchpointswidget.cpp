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

#include "watchpointswidget.h"
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

const QString WatchpointsWidget::mEnabledText = tr("Yes");
const QString WatchpointsWidget::mDisabledText = tr("No");
const QString WatchpointsWidget::mRdText = tr("Read");
const QString WatchpointsWidget::mWrText = tr("Write");
const QString WatchpointsWidget::mRdWrText = tr("Read/Write");

WatchpointsWidget::WatchpointsWidget(const QList<Watchpoint> &watchpoints, DevWidget *parent)
    : DevWidget{parent},
      mWpNum{0}
{
    mTbl = new QTableWidget(0, 5);
    mTbl->setHorizontalHeaderLabels({tr("Enabled"), tr("Trigger"), tr("Address"), tr("Size"), tr("Name")});
    mTbl->horizontalHeader()->setStretchLastSection(true);
    mTbl->verticalHeader()->setVisible(false);
    mTbl->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    mTbl->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mTbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTbl->setDragDropMode(QAbstractItemView::InternalMove);
    mTbl->setDragDropOverwriteMode(false);
    mTbl->setDragEnabled(true);
    mTbl->setAcceptDrops(false);

    mBtnToggleSelected = new QPushButton(tr("Toggle selected"));
    mBtnRemoveSelected = new QPushButton(tr("Remove selected"));

    mNormalBackground = QTableWidgetItem().background();

    QPushButton *btnAddWatchpoint = new QPushButton(tr("Add watchpoint"));

    QHBoxLayout *hboxbtns = new QHBoxLayout;
    hboxbtns->addWidget(btnAddWatchpoint);
    hboxbtns->addStretch(1);
    hboxbtns->addWidget(mBtnToggleSelected);
    hboxbtns->addWidget(mBtnRemoveSelected);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(hboxbtns);
    vLayout->addWidget(mTbl);
    setLayout(vLayout);

    foreach (const Watchpoint &watchpoint, watchpoints)
    {
        addWatchpoint(watchpoint, false);
    }

    connect(btnAddWatchpoint, &QPushButton::clicked, [this]
    {
        Watchpoint watchpoint ={ false, Watchpoint::Mode::ReadWrite, 0, 1, QStringLiteral("wp") + QString::number(mWpNum) };
        addWatchpoint(watchpoint, true);
        mWpNum++;
    });

    connect(mBtnRemoveSelected, &QPushButton::clicked, this, &WatchpointsWidget::removeSelected);
    connect(mBtnToggleSelected, &QPushButton::clicked, this, &WatchpointsWidget::toggleSelected);
    connect(mTbl, &QTableWidget::itemChanged, this, &WatchpointsWidget::itemChanged);
    connect(mTbl, &QTableWidget::itemActivated, this, &WatchpointsWidget::itemActivated);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}


void WatchpointsWidget::addWatchpoint(const Watchpoint &watchpoint, bool edit)
{
    QComboBox *cmbEnabled = new QComboBox;
    cmbEnabled->addItems({ mEnabledText, mDisabledText });
    QComboBox *cmbMode = new QComboBox;
    cmbMode->addItems({ mRdText, mWrText, mRdWrText });

    QString addrStr = Util::int2hex(watchpoint.addr, Util::AddrByteWidth);
    QString sizeStr = QString::number(watchpoint.size);

    if (mTbl->rowCount() == 0)
    {
        mBtnToggleSelected->setEnabled(true);
        mBtnRemoveSelected->setEnabled(true);
    }

    QTableWidgetItem *addr = new QTableWidgetItem(addrStr);
    QTableWidgetItem *size = new QTableWidgetItem(sizeStr);
    QTableWidgetItem *name = new QTableWidgetItem(watchpoint.name);

    mTbl->blockSignals(true);
    mTbl->insertRow(0);
    mTbl->setCellWidget(0, Column::Enabled, cmbEnabled);
    mTbl->setCellWidget(0, Column::Mode, cmbMode);
    mTbl->setItem(0, Column::Address, addr);
    mTbl->setItem(0, Column::Size, size);
    mTbl->setItem(0, Column::Name, name);
    mTbl->blockSignals(false);

    if (edit)
    {
        mTbl->editItem(addr);
    }

    setWatchpointEnabled(0, watchpoint.enabled);
    setWatchpointMode(0, watchpoint.mode);
}

bool WatchpointsWidget::getWatchpointEnabled(int row)
{
    QComboBox *cmbEnabled = static_cast<QComboBox *>(mTbl->cellWidget(row, Column::Enabled));
    return cmbEnabled->currentIndex() == 0;
}

void WatchpointsWidget::setWatchpointEnabled(int row, bool enabled)
{
    QComboBox *cmbEnabled = static_cast<QComboBox *>(mTbl->cellWidget(row, Column::Enabled));
    cmbEnabled->setCurrentIndex(enabled ? 0 : 1);
}

void WatchpointsWidget::setWatchpointMode(int row, int mode)
{
    QComboBox *cmbMode = static_cast<QComboBox *>(mTbl->cellWidget(row, Column::Mode));

    switch (mode)
    {
        default:
            abort();
        case Watchpoint::Mode::Read:
            cmbMode->setCurrentIndex(0);
            break;
        case Watchpoint::Mode::Write:
            cmbMode->setCurrentIndex(1);
            break;
        case Watchpoint::Mode::ReadWrite:
            cmbMode->setCurrentIndex(2);
            break;
    }
}

void WatchpointsWidget::toggleSelected()
{
    Q_ASSERT(mTbl->rowCount() != 0);

    for (int i = mTbl->rowCount() - 1; i >= 0; --i)
    {
        QTableWidgetItem *item = mTbl->item(i, Column::Address);
        if (item->isSelected())
        {
            if (Util::isHexAddress(item->text()))
            {
                setWatchpointEnabled(i, !getWatchpointEnabled(i));
            }
            else
            {
                setWatchpointEnabled(i, false);
            }
        }
    }
}

void WatchpointsWidget::removeSelected()
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
        mBtnRemoveSelected->setEnabled(false);
        mBtnToggleSelected->setEnabled(false);
    }
}

void WatchpointsWidget::itemActivated(QTableWidgetItem *item)
{
    switch (item->column())
    {
        default:
            break;
        case Column::Address:
            mPrevAddr = item->text();
            break;
        case Column::Size:
            mPrevSize = item->text();
            break;
    }
}

void WatchpointsWidget::itemChanged(QTableWidgetItem *item)
{
    int row = item->row();

    switch (item->column())
    {
        default:
            break;
        case Column::Address:
            if (Util::isHexAddress(item->text()))
            {
                item->setBackground(mNormalBackground);
                blockSignals(true);
                item->setText(Util::int2hex(Util::hex2int(item->text()), Util::AddrByteWidth));
                blockSignals(false);

                if (Util::isDecString(mTbl->item(row, Column::Size)->text(), 1, 16777215))
                {
                    mTbl->cellWidget(row, Column::Enabled)->setEnabled(true);
                    setWatchpointEnabled(row, true);
                }
            }
            else
            {
                mTbl->cellWidget(row, Column::Enabled)->setEnabled(false);
                setWatchpointEnabled(row, false);
                item->setBackground(QBrush(QColor(Qt::red).lighter()));
            }
            break;
        case Column::Size:
            if (Util::isDecString(item->text(), 1, 16777215))
            {
                item->setBackground(mNormalBackground);

                if (Util::isHexAddress(mTbl->item(row, Column::Address)->text()))
                {
                    mTbl->cellWidget(row, Column::Enabled)->setEnabled(true);
                    setWatchpointEnabled(row, true);
                }
            }
            else
            {
                mTbl->cellWidget(row, Column::Enabled)->setEnabled(false);
                setWatchpointEnabled(row, false);
                item->setBackground(QBrush(QColor(Qt::red).lighter()));
            }
            break;
    }
}
