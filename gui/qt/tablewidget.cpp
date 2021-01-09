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

#include "tablewidget.h"

#include <QtGui/QKeyEvent>

TableWidget::TableWidget(int rows, int cols, QWidget *parent)
    : QTableWidget{rows, cols, parent}
{
    setItemDelegate(new TableWidgetItemFocusDelegate{this});
}

void TableWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete)
    {
        emit deletePressed();
    }

    QTableWidget::keyPressEvent(event);
}

void TableWidgetItemFocusDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    option->state &= ~QStyle::State_HasFocus;
}
