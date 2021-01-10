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
class DisassemblyWidget;

#include <QtWidgets/QTableWidget>
#include <QtWidgets/QStyledItemDelegate>

class DisassemblerWidgetDelegate : public QStyledItemDelegate
{
public:
    DisassemblerWidgetDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

protected:
    virtual void initStyleOption(QStyleOptionViewItem *, const QModelIndex &) const;
    virtual void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const;
};

class DisassemblerWidget : public QTableWidget
{
    Q_OBJECT

public:
    explicit DisassemblerWidget(DisassemblyWidget *);
    DisassemblyWidget *parent() const;

    void setAddress(uint32_t);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    enum Column
    {
        Address,
        Data,
        Mnemonic,
        Count
    };

    bool isAtTop();
    bool isAtBottom();
    void append();
    void prepend();
    void scrollAction(int);
    void toggleBreakpoint(int);

    QPair<QString, QString> disassemble(uint32_t &addr);

    uint32_t mTopAddress;
    uint32_t mBottomAddress;

    Disassembler mDis;

    DisassemblerWidgetDelegate *mDelegate;
    QFont mBoldFont;
};

#endif
