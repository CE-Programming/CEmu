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

#include "memorywidget.h"

#include "widgets/memwidget.h"

#include "../util.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSpinBox>

MemoryWidget::MemoryWidget(DockedWidgetList &list, KDDockWidgets::DockWidgetBase *dock)
    : DockedWidget{dock ? dock : new KDDockWidgets::DockWidget{QStringLiteral("Memory #") + Util::randomString(6)},
                   QIcon(QStringLiteral(":/assets/icons/add_grid.svg")),
                   list}
{
    mMem = new MemWidget;

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(mMem);
    setLayout(vLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void MemoryWidget::closeEvent(QCloseEvent *)
{
    if (auto *p = parent())
    {
        p->deleteLater();
    }
}
