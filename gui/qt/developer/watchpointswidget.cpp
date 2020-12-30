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
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QSizePolicy>

const QString WatchpointsWidget::mDisabledText = tr("Disabled");
const QString WatchpointsWidget::mRdText = tr("Read");
const QString WatchpointsWidget::mWrText = tr("Write");
const QString WatchpointsWidget::mRdWrText = tr("Read/Write");

WatchpointsWidget::WatchpointsWidget(const QList<Watchpoint> &watchpoints, DevWidget *parent)
    : DevWidget{parent},
      mWpNum{0}
{
    mTbl = new QTableWidget(0, 4);
    mTbl->setHorizontalHeaderLabels({tr("Trigger"), tr("Address"), tr("Size"), tr("Name")});
    mTbl->horizontalHeader()->setStretchLastSection(true);
    mTbl->verticalHeader()->setVisible(false);
    mTbl->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mTbl->setSelectionBehavior(QAbstractItemView::SelectRows);

    mBtnMarkDisabled = new QPushButton(tr("Mark disabled"));
    mBtnMarkRead = new QPushButton(tr("Mark read"));
    mBtnMarkWrite = new QPushButton(tr("Mark write"));
    mBtnMarkReadWrite = new QPushButton(tr("Mark read/write"));

    QPushButton *btnAddWatchpoint = new QPushButton(tr("Add watchpoint"));
    QPushButton *btnRemoveSelected = new QPushButton(tr("Remove selected"));

    mNormalBackground = QTableWidgetItem().background();

    QHBoxLayout *hboxAddRemove = new QHBoxLayout;
    hboxAddRemove->addWidget(btnAddWatchpoint);
    hboxAddRemove->addStretch(1);
    hboxAddRemove->addWidget(btnRemoveSelected);

    QHBoxLayout *hboxBtns = new QHBoxLayout;
    hboxBtns->addWidget(mBtnMarkDisabled);
    hboxBtns->addWidget(mBtnMarkRead);
    hboxBtns->addWidget(mBtnMarkWrite);
    hboxBtns->addWidget(mBtnMarkReadWrite);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(hboxAddRemove);
    vLayout->addLayout(hboxBtns);
    vLayout->addWidget(mTbl);
    setLayout(vLayout);

    foreach (const Watchpoint &watchpoint, watchpoints)
    {
        addWatchpoint(watchpoint, false);
    }

    connect(btnAddWatchpoint, &QPushButton::clicked, [this]
    {
        Watchpoint watchpoint = { Watchpoint::Mode::Disabled, 0, 1, QStringLiteral("wp") + QString::number(mWpNum) };
        addWatchpoint(watchpoint, true);
        mWpNum++;
    });

    connect(mBtnMarkDisabled, &QPushButton::clicked, [this]{ toggleSelected(Watchpoint::Mode::Disabled); });
    connect(mBtnMarkRead, &QPushButton::clicked, [this]{ toggleSelected(Watchpoint::Mode::Read); });
    connect(mBtnMarkWrite, &QPushButton::clicked, [this]{ toggleSelected(Watchpoint::Mode::Write); });
    connect(mBtnMarkReadWrite, &QPushButton::clicked, [this]{ toggleSelected(Watchpoint::Mode::ReadWrite); });
    connect(mTbl, &QTableWidget::itemChanged, this, &WatchpointsWidget::itemChanged);
    connect(mTbl, &QTableWidget::itemActivated, this, &WatchpointsWidget::itemActivated);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}


void WatchpointsWidget::addWatchpoint(const Watchpoint &watchpoint, bool edit)
{
    QString addrStr = Util::int2hex(watchpoint.addr, Util::AddrByteWidth);
    QString sizeStr = QString::number(watchpoint.size);

    if (mTbl->rowCount() == 0)
    {
        mBtnMarkDisabled->setEnabled(true);
        mBtnMarkRead->setEnabled(true);
        mBtnMarkWrite->setEnabled(true);
        mBtnMarkReadWrite->setEnabled(true);
    }

    QTableWidgetItem *mode = new QTableWidgetItem;
    QTableWidgetItem *addr = new QTableWidgetItem(addrStr);
    QTableWidgetItem *size = new QTableWidgetItem(sizeStr);
    QTableWidgetItem *name = new QTableWidgetItem(watchpoint.name);

    mTbl->blockSignals(true);
    mTbl->insertRow(0);
    mTbl->setItem(0, 0, mode);
    mTbl->setItem(0, 1, addr);
    mTbl->setItem(0, 2, size);
    mTbl->setItem(0, 3, name);
    mTbl->blockSignals(false);

    if (edit)
    {
        mTbl->editItem(addr);
    }

    setWatchpoint(0, Watchpoint::Mode::Disabled);
}

void WatchpointsWidget::setWatchpoint(int row, int mode)
{
    switch (mode)
    {
        default:
            abort();
        case Watchpoint::Mode::Disabled:
            mTbl->item(row, Column::Mode)->setText(mDisabledText);
            break;
        case Watchpoint::Mode::Read:
            mTbl->item(row, Column::Mode)->setText(mRdText);
            break;
        case Watchpoint::Mode::Write:
            mTbl->item(row, Column::Mode)->setText(mWrText);
            break;
        case Watchpoint::Mode::ReadWrite:
            mTbl->item(row, Column::Mode)->setText(mRdWrText);
            break;
    }
}

void WatchpointsWidget::toggleSelected(int mode)
{
    Q_ASSERT(mTbl->rowCount() != 0);

    for (int i = mTbl->rowCount() - 1; i >= 0; --i)
    {
        QTableWidgetItem *item = mTbl->item(i, Column::Address);
        if (item->isSelected())
        {
            if (Util::isHexAddress(item->text()))
            {
                setWatchpoint(i, mode);

            }
            else
            {
                setWatchpoint(i, Watchpoint::Mode::Disabled);
            }
        }
    }
}

void WatchpointsWidget::removeSelected()
{
    Q_ASSERT(mTbl->rowCount() != 0);

    for (int i = mTbl->rowCount() - 1; i >= 0; --i)
    {
        if (mTbl->item(i, 0)->isSelected())
        {
            mTbl->removeRow(i);
        }
    }

    if (mTbl->rowCount() == 0)
    {
        mBtnMarkDisabled->setEnabled(false);
        mBtnMarkRead->setEnabled(false);
        mBtnMarkWrite->setEnabled(false);
        mBtnMarkReadWrite->setEnabled(false);
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
                    setWatchpoint(row, Watchpoint::ReadWrite);
                }
            }
            else
            {
                setWatchpoint(row, Watchpoint::Disabled);
                item->setBackground(QBrush(QColor(Qt::red).lighter()));
            }
            break;
        case Column::Size:
            if (Util::isDecString(item->text(), 1, 16777215))
            {
                item->setBackground(mNormalBackground);

                if (Util::isHexAddress(mTbl->item(row, Column::Address)->text()))
                {
                    setWatchpoint(row, Watchpoint::ReadWrite);
                }
            }
            else
            {
                setWatchpoint(row, Watchpoint::Disabled);
                item->setBackground(QBrush(QColor(Qt::red).lighter()));
            }
            break;
    }
}
