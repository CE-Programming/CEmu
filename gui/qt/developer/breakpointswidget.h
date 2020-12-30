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

#ifndef BREAKPOINTSWIDGET_H
#define BREAKPOINTSWIDGET_H

#include "devwidget.h"

QT_BEGIN_NAMESPACE
class QPushButton;
class QTableWidget;
class QTableWidgetItem;
QT_END_NAMESPACE

class Breakpoint
{
public:
    bool enabled;
    int addr;
    QString name;
};

class BreakpointsWidget : public DevWidget
{
    Q_OBJECT

public:
    explicit BreakpointsWidget(const QList<Breakpoint> &breakpoints, DevWidget *parent = nullptr);

public slots:
    void addBreakpoint(const Breakpoint &breakpoint, bool edit);
    void setBreakpoint(int row, bool enable);
    void removeSelected();
    void toggleSelected();

private slots:
    void itemActivated(QTableWidgetItem *item);
    void itemChanged(QTableWidgetItem *item);

private:
    enum Column
    {
        Enabled,
        Address,
        Name,
    };

    int mBpNum;

    QString mPrevAddr;

    QTableWidget *mTbl;

    QPushButton *mBtnToggleSelected;
    QPushButton *mBtnRemoveSelected;

    QBrush mNormalBackground;

    static const QString mEnabledText, mDisabledText;
};

#endif
