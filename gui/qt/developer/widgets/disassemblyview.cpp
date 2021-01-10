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

#include "disassemblyview.h"

#include "../../corewrapper.h"
#include "../disassemblywidget.h"
#include "../../util.h"

#include <QtGui/QWheelEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QScrollBar>

DisassemblyView::DisassemblyView(QWidget *parent)
    : QTableView{parent}
{
    mModel = new DisassemblyModel{this};

    setModel(mModel);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setVisible(true);
    verticalHeader()->setVisible(true);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setFont(Util::monospaceFont());
    verticalHeader()->sectionResizeMode(QHeaderView::Fixed);

    setAddress(512);

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &DisassemblyView::scroll);
}

void DisassemblyView::setAddress(uint32_t addr)
{
    mModel->setAddress(addr);
}

void DisassemblyView::scroll(int value)
{
    QScrollBar *v = verticalScrollBar();

    v->blockSignals(true);

    if (value >= v->maximum())
    {
        if (!mModel->isAtBottom())
        {
            mModel->append();
            v->setValue(v->maximum() - 1);
        }
    }
    else if (value <= v->minimum())
    {
        if (!mModel->isAtTop())
        {
            mModel->prepend();
            v->setValue(v->minimum() + 1);
        }
    }

    v->blockSignals(false);
}

DisassemblyModel::DisassemblyModel(QObject *parent)
    : QAbstractTableModel{parent}
{
}

bool DisassemblyModel::isAtTop()
{
    return mTopAddress == 0;
}

bool DisassemblyModel::isAtBottom()
{
    return mTopAddress == (2 << 24) - 1;
}

void DisassemblyModel::append()
{
    uint32_t addr = mLastAddress;
    uint32_t nextAddr;

    beginInsertRows(QModelIndex(), mAddress.count(), mAddress.count());

    mAddress.append(addr);
    mMnemonic.append(mDis.disassemble(addr, &nextAddr));

    mLastAddress = nextAddr;

    endInsertRows();
}

void DisassemblyModel::prepend()
{
    uint32_t addr = mLastAddress;
    uint32_t prevAddr = addr;
    uint32_t nextAddr;

    if (mTopAddress == 0)
    {
        return;
    }

    addr = mTopAddress;
    addr = addr < 32 ? 0 : addr - 32;

    beginInsertRows(QModelIndex(), 0, 0);

    for (int i = 0; i < 64; i++)
    {
        QString mnemonic = mDis.disassemble(addr, &nextAddr);

        if (addr >= mTopAddress)
        {
            uint32_t instSize = addr - prevAddr;
            mAddress.prepend(prevAddr);
            mMnemonic.prepend(mDis.disassemble(prevAddr, nullptr));
            mTopAddress = instSize > mTopAddress ? 0 : mTopAddress - instSize;
            endInsertRows();
            return;
        }

        prevAddr = addr;
        addr = nextAddr;
    }

    abort();
}

void DisassemblyModel::setAddress(uint32_t addr)
{
    mAddress.clear();
    mMnemonic.clear();

    beginResetModel();

    mTopAddress = addr;

    addr = addr < 32 ? 0 : addr - 32;

    for (int i = 0; i < 128; i++)
    {
        uint32_t nextAddr;

        QString mnemonic = mDis.disassemble(addr, &nextAddr);

        if (addr >= mTopAddress)
        {
            mAddress.append(addr);
            mMnemonic.append(mnemonic);
        }

        addr = nextAddr;
    }

    mLastAddress = addr;

    endResetModel();
}

int DisassemblyModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return mAddress.count();
}

int DisassemblyModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
}

QVariant DisassemblyModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && role == Qt::DisplayRole)
    {
        int row = index.row();
        switch (index.column())
        {
            default:
                break;
            case Column::Address:
                return Util::int2hex(mAddress.at(row), Util::addrByteWidth);
            case Column::Data:
                return QString("00");
                break;
            case Column::Mnemonic:
                return mMnemonic.at(row);
                break;
        }
    }

    return QVariant();
}

QVariant DisassemblyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
            default:
                break;
            case Column::Address:
                return tr("Address");
            case Column::Data:
                return tr("Data");
            case Column::Mnemonic:
                return tr("Mnemonic");
        }
    }

    return QVariant();
}

