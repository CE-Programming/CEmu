/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019-2020 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#pragma once

#include <kddockwidgets/FrameworkWidgetFactory.h>

#include <QPainter>

class DockWidgetFactory : public KDDockWidgets::DefaultWidgetFactory
{
public:
    KDDockWidgets::Frame *createFrame(KDDockWidgets::QWidgetOrQuick *parent = Q_NULLPTR, KDDockWidgets::FrameOptions options = KDDockWidgets::FrameOption_None) const override;
    KDDockWidgets::TitleBar *createTitleBar(KDDockWidgets::Frame *frame) const override;
    KDDockWidgets::TitleBar *createTitleBar(KDDockWidgets::FloatingWindow *fw) const override;
    Layouting::Separator *createSeparator(Layouting::Widget *parent = nullptr) const override;
};
