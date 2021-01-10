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

#ifndef DISASSEMBLERWIDGET_H
#define DISASSEMBLERWIDGET_H

#include "disassembler.h"

#include <QtWidgets/QTableWidget>

class DisassemblerWidget : public QTableWidget
{
    Q_OBJECT

public:
    explicit DisassemblerWidget(QWidget *parent = nullptr);

    void setAddress(uint32_t);

    enum Column
    {
        Address,
        Data,
        Mnemonic,
        Count
    };

private slots:
    void scroll(int);

private:
    bool isAtTop();
    bool isAtBottom();
    void append();
    void prepend();

    uint32_t mTopAddress;
    uint32_t mBottomAddress;

    Disassembler mDis;
};

#endif
