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

#ifndef WATCHPOINTSWIDGET_H
#define WATCHPOINTSWIDGET_H

#include "devwidget.h"

QT_BEGIN_NAMESPACE
class QPushButton;
class QTableWidget;
class QTableWidgetItem;
QT_END_NAMESPACE

class Watchpoint
{
public:
    enum Mode
    {
        Disabled,
        Read,
        Write,
        ReadWrite
    };

    int mode;
    int addr;
    int size;
    QString name;
};

class WatchpointsWidget : public DevWidget
{
    Q_OBJECT

public:
    explicit WatchpointsWidget(const QList<Watchpoint> &watchpoints, DevWidget *parent = nullptr);

public slots:
    void addWatchpoint(const Watchpoint &watchpoint, bool edit);
    void setWatchpoint(int row, int mode);
    void toggleSelected(int mode);
    void removeSelected();

private slots:
    void itemActivated(QTableWidgetItem *item);
    void itemChanged(QTableWidgetItem *item);

private:
    enum Column
    {
        Mode,
        Address,
        Size,
        Name,
    };

    int mWpNum;

    QString mPrevAddr;
    QString mPrevSize;

    QTableWidget *mTbl;

    QPushButton *mBtnMarkDisabled;
    QPushButton *mBtnMarkRead;
    QPushButton *mBtnMarkWrite;
    QPushButton *mBtnMarkReadWrite;

    QBrush mNormalBackground;

    static const QString mDisabledText;
    static const QString mRdText;
    static const QString mWrText;
    static const QString mRdWrText;
};

#endif
