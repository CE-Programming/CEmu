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

#include "dockedwidget.h"

#include "corewindow.h"

#include <kddockwidgets/DockWidgetBase.h>

#include <QtCore/QString>
#include <QtCore/QtGlobal>

DockedWidget::DockedWidget(KDDockWidgets::DockWidgetBase *dock,
                           const QIcon &icon,
                           CoreWindow *coreWindow)
    : QWidget{dock},
      DockedWidgetList{&coreWindow->dockedWidgets()},
      mCoreWindow{coreWindow}
{
    QString title = dock->title();
    int hash = title.lastIndexOf('#');
    if (hash != -1)
    {
        title = title.left(hash - 1);
    }
    dock->setTitle(tr(qUtf8Printable(title)));
    dock->setIcon(icon);
    dock->setWidget(this);
}

KDDockWidgets::DockWidgetBase *DockedWidget::dock()
{
    return static_cast<KDDockWidgets::DockWidgetBase *>(parent());
}

CoreWindow *DockedWidget::coreWindow()
{
    return mCoreWindow;
}
