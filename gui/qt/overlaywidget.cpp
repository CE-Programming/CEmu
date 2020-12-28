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

#include "overlaywidget.h"

#include <QtCore/QEvent>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>

void OverlayWidget::newParent()
{
    if (!parent()) return;
    parent()->installEventFilter(this);
    raise();
}

OverlayWidget::OverlayWidget(QWidget *parent) : QWidget{parent}
{
    setAttribute(Qt::WA_NoSystemBackground);
    newParent();
}

bool OverlayWidget::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == parent())
    {
        if (ev->type() == QEvent::Resize)
        {
            resize(static_cast<QResizeEvent *>(ev)->size());
        }
        else if (ev->type() == QEvent::ChildAdded)
        {
            raise();
        }
    }

    return QWidget::eventFilter(obj, ev);
}

bool OverlayWidget::event(QEvent *ev)
{
    if (ev->type() == QEvent::ParentAboutToChange)
    {
        if (parent())
        {
            parent()->removeEventFilter(this);
        }
    }
    else if (ev->type() == QEvent::ParentChange)
    {
        newParent();
    }

    return QWidget::event(ev);
}
