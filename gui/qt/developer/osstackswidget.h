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

#ifndef OSSTACKSWIDGET_H
#define OSSTACKSWIDGET_H

#include "../dockedwidget.h"

QT_BEGIN_NAMESPACE
class QTableWidget;
QT_END_NAMESPACE

class OsStacksWidget : public DockedWidget
{
    Q_OBJECT

public:
    explicit OsStacksWidget(DockedWidgetList &list);

private:
    QTableWidget *mTblFp;
    QTableWidget *mTblOp;
};

#endif
