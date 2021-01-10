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

#include "../corewrapper.h"
#include "widgets/memwidget.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QSizePolicy>

FlashRamWidget::FlashRamWidget(CoreWindow *coreWindow)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("Flash/RAM")},
                   QIcon(QStringLiteral(":/assets/icons/grid.svg")),
                   coreWindow}
{
    QGroupBox *grpFlash = new QGroupBox(tr("Flash"));
    QGroupBox *grpRam = new QGroupBox(tr("RAM"));

    mFlash = new MemWidget{this, MemWidget::Area::Flash};
    mRam = new MemWidget{this, MemWidget::Area::Ram};

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

    enableDebugWidgets(false);
}

void FlashRamWidget::enableDebugWidgets(bool enbaled)
{
    setEnabled(enbaled);
}
