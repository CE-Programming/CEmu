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

#ifndef DISASSEMBLERWIDGET_H
#define DISASSEMBLERWIDGET_H

#include "disassembler.h"
#include "../watchpointswidget.h"
class DisassemblyWidget;

#include <QtWidgets/QTableWidget>
#include <QtWidgets/QStyledItemDelegate>

class DisassemblerWidgetDelegate : public QStyledItemDelegate
{
public:
    DisassemblerWidgetDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent), mWidth{0} {}
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    void setOptionWidth(int value);

protected:
    virtual void initStyleOption(QStyleOptionViewItem *, const QModelIndex &) const override;

private:
    int mWidth;
};

class DisassemblerWidget : public QTableWidget
{
    Q_OBJECT

public:
    explicit DisassemblerWidget(DisassemblyWidget *);
    DisassemblyWidget *parent() const;

    bool gotoAddress(const QString &);
    void setAdl(bool enable);

    enum Column
    {
        Address,
        Data,
        Mnemonic,
        Count
    };

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *) override;
    void wheelEvent(QWheelEvent *) override;

signals:
    void toggleWatchpoint(const Watchpoint watchpoint);

private:
    bool isAtTop();
    bool isAtBottom();
    void append();
    void prepend();
    void scrollAction(int);
    void insertDisasmRow(int, uint32_t, const QString &, const QString &);

    int selectedAddress();
    QPair<QString, QString> disassemble(uint32_t &);

    uint32_t mTopAddress;
    uint32_t mBottomAddress;
    uint32_t mPcAddr;

    int mAdlMode;

    Disassembler mDis;

    DisassemblerWidgetDelegate *mDelegate;
    QFont mBoldFont;
};

#endif
