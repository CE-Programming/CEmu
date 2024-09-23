/*
 * Copyright (c) 2015-2024 CE Programming.
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

#include "dockwidget.h"

#include <kddockwidgets/DockWidget.h>
#include <kddockwidgets/private/multisplitter/Separator_qwidget.h>
#include <kddockwidgets/private/widgets/FrameWidget_p.h>
#include <kddockwidgets/private/widgets/TitleBarWidget_p.h>

#include <QtWidgets/QApplication>

class DockTitleBar : public KDDockWidgets::TitleBarWidget
{
public:
    explicit DockTitleBar(KDDockWidgets::Frame *frame)
        : KDDockWidgets::TitleBarWidget(frame)
    {
    }

    explicit DockTitleBar(KDDockWidgets::FloatingWindow *fw)
        : KDDockWidgets::TitleBarWidget(fw)
    {
    }

    void paintEvent(QPaintEvent *event) override
    {
        KDDockWidgets::TitleBarWidget::paintEvent(event);
    }
};

class DockSeparator : public Layouting::SeparatorWidget
{
public:
    explicit DockSeparator(Layouting::Widget *parent)
        : Layouting::SeparatorWidget(parent)
    {
        setContentsMargins(0, 0, 0, 0);
    }
};

class DockFrame : public KDDockWidgets::FrameWidget
{
public:
    explicit DockFrame(QWidget *parent, KDDockWidgets::FrameOptions options)
        : KDDockWidgets::FrameWidget(parent, options)
    {
        setContentsMargins(0, 0, 0, 0);
    }

    void paintEvent(QPaintEvent *event) override
    {
        KDDockWidgets::FrameWidget::paintEvent(event);
    }
};

KDDockWidgets::Frame *DockWidgetFactory::createFrame(KDDockWidgets::QWidgetOrQuick *parent, KDDockWidgets::FrameOptions options) const
{
    return new DockFrame(parent, options);
}

KDDockWidgets::TitleBar *DockWidgetFactory::createTitleBar(KDDockWidgets::Frame *frame) const
{
    return new DockTitleBar(frame);
}

KDDockWidgets::TitleBar *DockWidgetFactory::createTitleBar(KDDockWidgets::FloatingWindow *fw) const
{
    return new DockTitleBar(fw);
}

Layouting::Separator *DockWidgetFactory::createSeparator(Layouting::Widget *parent) const
{
    return new DockSeparator(parent);
}

KDDockWidgets::DockWidgetBase *DockWidgetFactory::dockWidgetFactory(const QString &name)
{
    return name.contains('#') ? new KDDockWidgets::DockWidget(name) : nullptr;
}
