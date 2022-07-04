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

#include "watchpointswidget.h"

#include "../corewrapper.h"
#include "../tablewidget.h"
#include "../util.h"

#include <kddockwidgets/DockWidget.h>

#include <QtGui/QColor>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QToolButton>

const uint32_t WatchpointsWidget::sMaxSize = (2 << 24);
const uint32_t WatchpointsWidget::sMaxAddr = (2 << 24) - 1;

WatchpointsWidget::WatchpointsWidget(CoreWindow *coreWindow, const QList<Watchpoint> &watchpoints)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("Watchpoints")},
                   QIcon(QStringLiteral(":/assets/icons/flash_on.svg")),
                   coreWindow},
      mWpNum{0},
      mDefaultMode{Watchpoint::Mode::R | Watchpoint::Mode::W}
{
    mTbl = new TableWidget(0, 7);
    mTbl->setHorizontalHeaderLabels({tr("E"), tr("R"), tr("W"), tr("X"), tr("Address"), tr("Size"), tr("Name")});
    mTbl->horizontalHeader()->setStretchLastSection(true);
    mTbl->horizontalHeader()->setDefaultSectionSize(QFontMetrics(Util::monospaceFont()).maxWidth() * 10);
    mTbl->horizontalHeader()->setMinimumSectionSize(mTbl->verticalHeader()->defaultSectionSize());
    mTbl->horizontalHeader()->setSectionResizeMode(Column::Enabled, QHeaderView::ResizeToContents);
    mTbl->horizontalHeader()->setSectionResizeMode(Column::Read, QHeaderView::ResizeToContents);
    mTbl->horizontalHeader()->setSectionResizeMode(Column::Write, QHeaderView::ResizeToContents);
    mTbl->horizontalHeader()->setSectionResizeMode(Column::Execute, QHeaderView::ResizeToContents);
    mTbl->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    mTbl->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mTbl->setSelectionBehavior(QAbstractItemView::SelectRows);

    mBtnRemoveSelected = new QPushButton(QIcon(QStringLiteral(":/assets/icons/cross.svg")), tr("Remove selected"));
    mBtnRemoveSelected->setEnabled(false);

    mNormalBackground = QTableWidgetItem().background();

    QPushButton *btnAddWatchpoint = new QPushButton(QIcon(QStringLiteral(":/assets/icons/plus.svg")), tr("Add watchpoint"));
    QComboBox *cmbDefaultMode = new QComboBox;
    cmbDefaultMode->addItems({ tr("Default: R"), tr("Default: W"), tr("Default: RW"), tr("Default: X") });
    cmbDefaultMode->setCurrentIndex(2);

    QHBoxLayout *hboxbtns = new QHBoxLayout;
    hboxbtns->addWidget(btnAddWatchpoint);
    hboxbtns->addWidget(cmbDefaultMode);
    hboxbtns->addStretch();
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
        Watchpoint watchpoint = { mDefaultMode, 0, 1, QStringLiteral("wp") + QString::number(mWpNum++) };
        addWatchpoint(watchpoint, true);
    });

    connect(cmbDefaultMode, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int index)
    {
        switch (index)
        {
            default:
            case 0:
                mDefaultMode = Watchpoint::Mode::R;
                break;
            case 1:
                mDefaultMode = Watchpoint::Mode::W;
                break;
            case 2:
                mDefaultMode = Watchpoint::Mode::R | Watchpoint::Mode::W;
                break;
            case 3:
                mDefaultMode = Watchpoint::Mode::X;
                break;
        }
    });

    connect(mBtnRemoveSelected, &QPushButton::clicked, this, &WatchpointsWidget::removeSelected);
    connect(mTbl, &TableWidget::deletePressed, this, &WatchpointsWidget::removeSelected);
    connect(mTbl, &QTableWidget::itemChanged, this, &WatchpointsWidget::itemChanged);
    connect(mTbl, &QTableWidget::itemPressed, this, &WatchpointsWidget::itemPressed);
    connect(mTbl, &QTableWidget::itemSelectionChanged, [this]
    {
        mBtnRemoveSelected->setEnabled(mTbl->selectedItems().count() >= 1);
    });

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void WatchpointsWidget::addWatchpoint(const Watchpoint &watchpoint, bool edit)
{
    int id = core().get(cemucore::CEMUCORE_PROP_DEBUG_WATCH, -1);
    if (id == -1)
    {
        return;
    }

    QString addrStr = Util::int2hex(watchpoint.addr, Util::addrByteWidth);
    QString sizeStr = QString::number(watchpoint.size);

    QTableWidgetItem *e = new QTableWidgetItem;
    QTableWidgetItem *r = new QTableWidgetItem;
    QTableWidgetItem *w = new QTableWidgetItem;
    QTableWidgetItem *x = new QTableWidgetItem;
    QTableWidgetItem *addr = new QTableWidgetItem(addrStr);
    QTableWidgetItem *size = new QTableWidgetItem(sizeStr);
    QTableWidgetItem *name = new QTableWidgetItem(watchpoint.name);

    e->setData(Role::Id, id);

    e->setFlags(e->flags() & ~Qt::ItemIsEditable);
    r->setFlags(r->flags() & ~Qt::ItemIsEditable);
    w->setFlags(w->flags() & ~Qt::ItemIsEditable);
    x->setFlags(x->flags() & ~Qt::ItemIsEditable);

    e->setTextAlignment(Qt::AlignCenter);
    r->setTextAlignment(Qt::AlignCenter);
    w->setTextAlignment(Qt::AlignCenter);
    x->setTextAlignment(Qt::AlignCenter);

    e->setFont(Util::monospaceFont());
    r->setFont(Util::monospaceFont());
    w->setFont(Util::monospaceFont());
    x->setFont(Util::monospaceFont());
    addr->setFont(Util::monospaceFont());
    size->setFont(Util::monospaceFont());
    name->setFont(Util::monospaceFont());

    mTbl->blockSignals(true);
    mTbl->insertRow(0);
    mTbl->setItem(0, Column::Enabled, e);
    mTbl->setItem(0, Column::Read, w);
    mTbl->setItem(0, Column::Write, r);
    mTbl->setItem(0, Column::Execute, x);
    mTbl->setItem(0, Column::Address, addr);
    mTbl->setItem(0, Column::Size, size);
    mTbl->setItem(0, Column::Name, name);
    mTbl->setVerticalHeaderItem(0, new QTableWidgetItem(QIcon(QStringLiteral(":/assets/icons/updown.svg")), QString()));
    mTbl->blockSignals(false);

    if (edit)
    {
        mTbl->editItem(addr);
    }

    setWatchpointMode(0, watchpoint.mode);

    enableDebugWidgets(false);
}

int WatchpointsWidget::getWatchpointMode(int row)
{
    return mTbl->item(row, Column::Enabled)->data(Role::Mode).toInt();
}

void WatchpointsWidget::setWatchpointMode(int row, int mode)
{
    const QString space = QStringLiteral(" ");
    int id = mTbl->item(row, Column::Enabled)->data(Role::Id).toInt();

    QString addrStr = mTbl->item(row, Column::Address)->text();
    if (Util::isHexAddress(addrStr))
    {
        core().set(cemucore::CEMUCORE_PROP_DEBUG_WATCH_ADDR, id, Util::hex2int(addrStr));
    }

    QString sizeStr = mTbl->item(row, Column::Size)->text();
    if (Util::isDecString(sizeStr, 1, sMaxSize))
    {
        core().set(cemucore::CEMUCORE_PROP_DEBUG_WATCH_SIZE, id, sizeStr.toUInt());
    }

    QFlags<cemucore::debug_flags> flags;
    flags |= cemucore::CEMUCORE_DEBUG_WATCH_MEMORY;
    flags |= cemucore::CEMUCORE_DEBUG_WATCH_ANY;

    if (mode & Watchpoint::Mode::E)
    {
        mTbl->item(row, Column::Enabled)->setText(QStringLiteral("e"));
        flags |= cemucore::CEMUCORE_DEBUG_WATCH_ENABLE;
    }
    else
    {
        mTbl->item(row, Column::Enabled)->setText(space);
    }

    if (mode & Watchpoint::Mode::R)
    {
        mTbl->item(row, Column::Read)->setText(QStringLiteral("r"));
        flags |= cemucore::CEMUCORE_DEBUG_WATCH_READ;
    }
    else
    {
        mTbl->item(row, Column::Read)->setText(space);
    }

    if (mode & Watchpoint::Mode::W)
    {
        mTbl->item(row, Column::Write)->setText(QStringLiteral("w"));
        flags |= cemucore::CEMUCORE_DEBUG_WATCH_WRITE;
    }
    else
    {
        mTbl->item(row, Column::Write)->setText(space);
    }

    if (mode & Watchpoint::Mode::X)
    {
        mTbl->item(row, Column::Execute)->setText(QStringLiteral("x"));
        flags |= cemucore::CEMUCORE_DEBUG_WATCH_EXECUTE;
    }
    else
    {
        mTbl->item(row, Column::Execute)->setText(space);
    }

    mTbl->item(row, Column::Enabled)->setData(Role::Mode, mode);
    core().set(cemucore::CEMUCORE_PROP_DEBUG_WATCH_FLAGS, id, flags);
}

void WatchpointsWidget::removeSelected()
{
    Q_ASSERT(mTbl->rowCount() != 0);

    for (int i = mTbl->rowCount() - 1; i >= 0; --i)
    {
        if (mTbl->item(i, Column::Address)->isSelected())
        {
            int id = mTbl->item(i, Column::Enabled)->data(Role::Id).toInt();
            core().set(cemucore::CEMUCORE_PROP_DEBUG_WATCH, id, -1);
            mTbl->removeRow(i);
        }
    }
}

bool WatchpointsWidget::toggleMode(int row, int bit)
{
    int mode = getWatchpointMode(row);
    if (mode & bit)
    {
        setWatchpointMode(row, mode & ~bit);
        return false;
    }

    setWatchpointMode(row, mode | bit);
    return true;
}

void WatchpointsWidget::itemPressed(QTableWidgetItem *item)
{
    int bit;

    switch (item->column())
    {
        default:
            return;
        case Column::Enabled:
            bit = Watchpoint::Mode::E;
            break;
        case Column::Read:
            bit = Watchpoint::Mode::R;
            break;
        case Column::Write:
            bit = Watchpoint::Mode::W;
            break;
        case Column::Execute:
            bit = Watchpoint::Mode::X;
            break;
    }

    mTbl->blockSignals(true);

    bool set = toggleMode(item->row(), bit);

    foreach(QTableWidgetItem *selected, mTbl->selectedItems())
    {
        if (selected->column() == item->column())
        {
            int mode = getWatchpointMode(selected->row());
            if (set)
            {
                setWatchpointMode(selected->row(), mode | bit);
            }
            else
            {
                setWatchpointMode(selected->row(), mode & ~bit);
            }
        }
    }

    mTbl->blockSignals(false);
}

void WatchpointsWidget::itemChanged(QTableWidgetItem *item)
{
    const QBrush invalidItemBrush(QColor(Qt::red).lighter());
    int row = item->row();

    switch (item->column())
    {
        default:
            break;
        case Column::Address:
            mTbl->blockSignals(true);
            if (Util::isHexAddress(item->text()))
            {
                item->setBackground(mNormalBackground);
                item->setText(Util::int2hex(Util::hex2int(item->text()), Util::addrByteWidth));

                if (Util::isDecString(mTbl->item(row, Column::Size)->text(), 1, ((2 << 24) - 1)))
                {
                    setWatchpointMode(row, getWatchpointMode(row) | Watchpoint::Mode::E);
                }
            }
            else
            {
                setWatchpointMode(row, getWatchpointMode(row) & ~Watchpoint::Mode::E);
                mTbl->item(row, Column::Address)->setBackground(invalidItemBrush);
                mTbl->item(row, Column::Enabled)->setBackground(invalidItemBrush);
            }
            mTbl->blockSignals(false);
            break;
        case Column::Size:
            mTbl->blockSignals(true);
            if (Util::isDecString(item->text(), 1, sMaxSize))
            {
                item->setBackground(mNormalBackground);

                if (Util::isHexAddress(mTbl->item(row, Column::Address)->text()))
                {
                    setWatchpointMode(row, getWatchpointMode(row) | Watchpoint::Mode::E);
                }
            }
            else
            {
                setWatchpointMode(row, getWatchpointMode(row) & ~Watchpoint::Mode::E);
                mTbl->item(row, Column::Size)->setBackground(invalidItemBrush);
                mTbl->item(row, Column::Enabled)->setBackground(invalidItemBrush);
            }
            mTbl->blockSignals(false);
            break;
    }
}
