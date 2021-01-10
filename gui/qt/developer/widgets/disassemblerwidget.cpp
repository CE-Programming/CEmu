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

DisassemblerWidget::DisassemblerWidget(DisassemblyWidget *parent)
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

DisassemblyWidget *DisassemblerWidget::parent() const
{
    return static_cast<DisassemblyWidget *>(QTableWidget::parent());
}

void DisassemblerWidget::setAddress(uint32_t addr)
{
    clearContents();

    mTopAddress = addr;

    addr = addr < 32 ? 0 : addr - 32;

    for (int i = 0; i < 128; i++)
    {
        uint32_t prevAddr = addr;
        QString mnemonic = disassemble(addr);

        if (prevAddr >= mTopAddress)
        {
            int row = rowCount();
            insertRow(row);
            setItem(row, Column::Address, new QTableWidgetItem{Util::int2hex(prevAddr, Util::addrByteWidth)});
            setItem(row, Column::Data, new QTableWidgetItem{QStringLiteral("00")});
            setItem(row, Column::Mnemonic, new QTableWidgetItem{mnemonic});
        }
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

    int row = rowCount();
    insertRow(row);
    setItem(row, Column::Address, new QTableWidgetItem{Util::int2hex(addr, Util::addrByteWidth)});
    setItem(row, Column::Data, new QTableWidgetItem{QStringLiteral("00")});
    setItem(row, Column::Mnemonic, new QTableWidgetItem{disassemble(addr)});

    mBottomAddress = addr;
}

void DisassemblerWidget::prepend()
{
    if (mTopAddress == 0)
    {
        return;
    }

    uint32_t addr = mBottomAddress;

    addr = mTopAddress;
    addr = addr < 32 ? 0 : addr - 32;

    for (int i = 0; i < 64; i++)
    {
        uint32_t prevAddr = addr;
        QString mnemonic = disassemble(addr);

        if (addr >= mTopAddress)
        {
            uint32_t instSize = addr - prevAddr;

            int row = 0;
            insertRow(row);
            setItem(row, Column::Address, new QTableWidgetItem{Util::int2hex(prevAddr, Util::addrByteWidth)});
            setItem(row, Column::Data, new QTableWidgetItem{QStringLiteral("00")});
            setItem(row, Column::Mnemonic, new QTableWidgetItem{mnemonic});

            mTopAddress = instSize > mTopAddress ? 0 : mTopAddress - instSize;
            return;
        }
    }

    abort();
}

QString DisassemblerWidget::disassemble(uint32_t &addr)
{
    return mDis.disassemble(parent()->core(), addr);
}
