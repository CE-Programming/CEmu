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

#include "flashramwidget.h"
#include "widgets/memwidget.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QSizePolicy>

FlashRamWidget::FlashRamWidget(DockedWidgetList &list)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("Flash/RAM")}, list}
{
    QGroupBox *grpFlash = new QGroupBox(tr("Flash"));
    QGroupBox *grpRam = new QGroupBox(tr("RAM"));

    mFlash = new MemWidget;
    mRam = new MemWidget;

    QHBoxLayout *hboxFlash = new QHBoxLayout;
    hboxFlash->addWidget(mFlash);
    grpFlash->setLayout(hboxFlash);

    QHBoxLayout *hboxRam = new QHBoxLayout;
    hboxRam->addWidget(mRam);
    grpRam->setLayout(hboxRam);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(grpFlash);
    vLayout->addWidget(grpRam);
    setLayout(vLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
