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

#ifndef DISASSEMBLYVIEW_H
#define DISASSEMBLYVIEW_H

#include "disassembly.h"

#include <QtCore/QAbstractTableModel>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTableView>
#include <QtWidgets/QWidget>
QT_BEGIN_NAMESPACE
class QTableView;
QT_END_NAMESPACE

class DisassemblyModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column
    {
        Address,
        Data,
        Mnemonic
    };

    DisassemblyModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setAddress(uint32_t addr);
    void append();
    void prepend();
    bool isAtTop();
    bool isAtBottom();

private:
    uint32_t mTopAddress;
    uint32_t mLastAddress;

    Disassembly mDis;

    QVector<int32_t> mAddress;
    QVector<QString> mMnemonic;
};


class DisassemblyView : public QTableView
{
    Q_OBJECT

public:
    explicit DisassemblyView(QWidget *parent = nullptr);

    void setAddress(uint32_t);

private slots:
    void scroll(int);

private:
    DisassemblyModel *mModel;
};

#endif
