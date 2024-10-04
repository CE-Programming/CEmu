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

#include "configurationwidget.h"

#include "../corethread.h"
#include "../tablewidget.h"
#include "../util.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>

ConfigurationWidget::ConfigurationWidget(CoreWindow *coreWindow)
    : DockedWidget{new KDDockWidgets::QtWidgets::DockWidget{QStringLiteral("Configuration")},
                   QIcon(QStringLiteral(":/assets/icons/support.svg")),
                   coreWindow}
{
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
}
