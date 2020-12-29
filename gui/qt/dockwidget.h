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

#ifndef DOCKWIDGET_H
#define DOCKWIDGET_H

#include <kddockwidgets/FrameworkWidgetFactory.h>
namespace KDDockWidgets
{
class DockWidgetBase;
}

class DockWidgetFactory : public KDDockWidgets::DefaultWidgetFactory
{
public:
    KDDockWidgets::Frame *createFrame(KDDockWidgets::QWidgetOrQuick *parent = nullptr, KDDockWidgets::FrameOptions options = KDDockWidgets::FrameOption_None) const override;
    KDDockWidgets::TitleBar *createTitleBar(KDDockWidgets::Frame *frame) const override;
    KDDockWidgets::TitleBar *createTitleBar(KDDockWidgets::FloatingWindow *fw) const override;
    Layouting::Separator *createSeparator(Layouting::Widget *parent = nullptr) const override;
    static KDDockWidgets::DockWidgetBase *dockWidgetFactory(const QString &name);
};

#endif
