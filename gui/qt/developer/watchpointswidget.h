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

#ifndef WATCHPOINTSWIDGET_H
#define WATCHPOINTSWIDGET_H

#include "../dockedwidget.h"
class TableWidget;

#include <QtCore/QString>
#include <QtGui/QBrush>
QT_BEGIN_NAMESPACE
class QPushButton;
class QTableWidgetItem;
QT_END_NAMESPACE

class Watchpoint
{
public:
    enum Mode
    {
        E = (1 << 0),
        R = (1 << 1),
        W = (1 << 2),
        X = (1 << 3),
    };

    int mode;
    int addr;
    int size;
    QString name;
};

class WatchpointsWidget : public DockedWidget
{
    Q_OBJECT

public:
    explicit WatchpointsWidget(CoreWindow *coreWindow, const QList<Watchpoint> &watchpoints);

private slots:
    void addWatchpoint(const Watchpoint &watchpoint, bool edit);
    void setWatchpointMode(int row, int mode);
    int getWatchpointMode(int row);
    bool toggleMode(int row, int bit);
    void removeSelected();
    void itemPressed(QTableWidgetItem *item);
    void itemChanged(QTableWidgetItem *item);

private:
    enum Column
    {
        Enabled,
        Read,
        Write,
        Execute,
        Address,
        Size,
        Name,
    };
    enum Role
    {
        Id = Qt::UserRole,
        Mode,
    };

    int mWpNum;
    int mDefaultMode;

    TableWidget *mTbl;

    QPushButton *mBtnRemoveSelected;

    QBrush mNormalBackground;

    static const uint32_t sMaxSize;
    static const uint32_t sMaxAddr;
};

#endif
