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

#include "disassemblerwidget.h"

#include "../../corewrapper.h"
#include "../disassemblywidget.h"
#include "../../util.h"

#include <QtGui/QPainter>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QScrollBar>

DisassemblerWidget::DisassemblerWidget(QWidget *parent)
    : QTableWidget{parent}
{
    setColumnCount(Column::Count);
    setHorizontalHeaderItem(Column::Address, new QTableWidgetItem{tr("Address")});
    setHorizontalHeaderItem(Column::Data, new QTableWidgetItem{tr("Data")});
    setHorizontalHeaderItem(Column::Mnemonic, new QTableWidgetItem{tr("Mnemonic")});

    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setVisible(true);
    verticalHeader()->setVisible(false);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setFont(Util::monospaceFont());

    setAddress(512);

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &DisassemblerWidget::scroll);
}

void DisassemblerWidget::setAddress(uint32_t addr)
{
    clearContents();

    mTopAddress = addr;

    addr = addr < 32 ? 0 : addr - 32;

    for (int i = 0; i < 128; i++)
    {
        uint32_t nextAddr;

        QString mnemonic = mDis.disassemble(addr, &nextAddr);

        if (addr >= mTopAddress)
        {
            int row = rowCount();
            insertRow(row);
            setItem(row, Column::Address, new QTableWidgetItem{Util::int2hex(addr, Util::addrByteWidth)});
            setItem(row, Column::Data, new QTableWidgetItem{QStringLiteral("00")});
            setItem(row, Column::Mnemonic, new QTableWidgetItem{mnemonic});
        }

        addr = nextAddr;
    }

    mBottomAddress = addr;
}

void DisassemblerWidget::scroll(int value)
{
    QScrollBar *v = verticalScrollBar();

    v->blockSignals(true);

    if (value >= v->maximum())
    {
        if (!isAtBottom())
        {
            append();
            v->setValue(v->maximum() - 1);
        }
    }
    else if (value <= v->minimum())
    {
        if (!isAtTop())
        {
            prepend();
            v->setValue(v->minimum() + 1);
        }
    }

    v->blockSignals(false);
}

bool DisassemblerWidget::isAtTop()
{
    return mTopAddress == 0;
}

bool DisassemblerWidget::isAtBottom()
{
    return mBottomAddress >= 0xFFFFFF;
}

void DisassemblerWidget::append()
{
    uint32_t addr = mBottomAddress;
    uint32_t nextAddr;

    int row = rowCount();
    insertRow(row);
    setItem(row, Column::Address, new QTableWidgetItem{Util::int2hex(addr, Util::addrByteWidth)});
    setItem(row, Column::Data, new QTableWidgetItem{QStringLiteral("00")});
    setItem(row, Column::Mnemonic, new QTableWidgetItem{mDis.disassemble(addr, &nextAddr)});

    mBottomAddress = nextAddr;
}

void DisassemblerWidget::prepend()
{
    uint32_t addr = mBottomAddress;
    uint32_t prevAddr = addr;
    uint32_t nextAddr;

    if (mTopAddress == 0)
    {
        return;
    }

    addr = mTopAddress;
    addr = addr < 32 ? 0 : addr - 32;

    for (int i = 0; i < 64; i++)
    {
        QString mnemonic = mDis.disassemble(addr, &nextAddr);

        if (addr >= mTopAddress)
        {
            uint32_t instSize = addr - prevAddr;

            int row = 0;
            insertRow(row);
            setItem(row, Column::Address, new QTableWidgetItem{Util::int2hex(prevAddr, Util::addrByteWidth)});
            setItem(row, Column::Data, new QTableWidgetItem{QStringLiteral("00")});
            setItem(row, Column::Mnemonic, new QTableWidgetItem{mDis.disassemble(prevAddr, nullptr)});

            mTopAddress = instSize > mTopAddress ? 0 : mTopAddress - instSize;
            return;
        }

        prevAddr = addr;
        addr = nextAddr;
    }

    abort();
}
