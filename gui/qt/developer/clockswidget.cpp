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

#include "clockswidget.h"
#include "widgets/highlighteditwidget.h"

#include <kddockwidgets/DockWidget.h>

#include <QtGui/QIntValidator>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSizePolicy>

ClocksWidget::ClocksWidget(DockedWidgetList &list)
    : DockedWidget{new KDDockWidgets::DockWidget(QStringLiteral("Clocks")),
                   QIcon(QStringLiteral(":/assets/icons/clock.svg")),
                   list}
{
    QGroupBox *grpGpt = new QGroupBox(tr("General Purpose Timers"));
    QGroupBox *grpRtc = new QGroupBox(tr("Real Time Clock"));

    QLabel *lblValue = new QLabel(tr("Value"));
    QLabel *lblReloadValue = new QLabel(tr("Reload Value"));
    QLabel *lblTimer1 = new QLabel(tr("Timer 1"));
    QLabel *lblTimer2 = new QLabel(tr("Timer 2"));
    QLabel *lblTimer3 = new QLabel(tr("Timer 3"));
    QLabel *lblDays = new QLabel(tr("Days"));
    QLabel *lblHrs = new QLabel(tr("Hours"));
    QLabel *lblMin = new QLabel(tr("Minutes"));
    QLabel *lblSec = new QLabel(tr("Seconds"));

    mEdtTimer1V = new HighlightEditWidget;
    mEdtTimer2V = new HighlightEditWidget;
    mEdtTimer3V = new HighlightEditWidget;
    mEdtTimer1RV = new HighlightEditWidget;
    mEdtTimer2RV = new HighlightEditWidget;
    mEdtTimer3RV = new HighlightEditWidget;

    mEdtDays = new HighlightEditWidget;
    mEdtHrs = new HighlightEditWidget;
    mEdtMin = new HighlightEditWidget;
    mEdtSec = new HighlightEditWidget;

    QHBoxLayout *hboxRtc = new QHBoxLayout;
    hboxRtc->addWidget(lblDays);
    hboxRtc->addWidget(mEdtDays);
    hboxRtc->addWidget(lblHrs);
    hboxRtc->addWidget(mEdtHrs);
    hboxRtc->addWidget(lblMin);
    hboxRtc->addWidget(mEdtMin);
    hboxRtc->addWidget(lblSec);
    hboxRtc->addWidget(mEdtSec);
    grpRtc->setLayout(hboxRtc);

    lblValue->setAlignment(Qt::AlignCenter);
    lblReloadValue->setAlignment(Qt::AlignCenter);

    QGridLayout *gridGpt = new QGridLayout;
    gridGpt->addWidget(lblValue, 0, 1);
    gridGpt->addWidget(lblReloadValue, 0, 2);
    gridGpt->addWidget(lblTimer1, 1, 0);
    gridGpt->addWidget(lblTimer2, 2, 0);
    gridGpt->addWidget(lblTimer3, 3, 0);
    gridGpt->addWidget(mEdtTimer1V, 1, 1);
    gridGpt->addWidget(mEdtTimer2V, 2, 1);
    gridGpt->addWidget(mEdtTimer3V, 3, 1);
    gridGpt->addWidget(mEdtTimer1RV, 1, 2);
    gridGpt->addWidget(mEdtTimer2RV, 2, 2);
    gridGpt->addWidget(mEdtTimer3RV, 3, 2);
    grpGpt->setLayout(gridGpt);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addStretch();
    vLayout->addWidget(grpGpt);
    vLayout->addWidget(grpRtc);
    vLayout->addStretch();

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addStretch();
    hLayout->addLayout(vLayout);
    hLayout->addStretch();
    setLayout(hLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
