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

#include "controlwidget.h"

#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QBoxLayout>

ControlWidget::ControlWidget(DevWidget *parent)
    : DevWidget{parent}
{
    QPushButton *btnRun = new QPushButton(tr("Stop"));
    QPushButton *btnStep = new QPushButton(tr("Step"));
    QPushButton *btnStepOver = new QPushButton(tr("Step Over"));
    QPushButton *btnStepNext = new QPushButton(tr("Step Next"));
    QPushButton *btnStepOut = new QPushButton(tr("Step Out"));
    QPushButton *btnMode = new QPushButton(tr("Mode: ASM"));

    QSpacerItem *spacel = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding);
    QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetMinimumSize);
    layout->addSpacerItem(spacel);
    layout->addWidget(btnRun, Qt::AlignCenter);
    layout->addWidget(btnStep, Qt::AlignCenter);
    layout->addWidget(btnStepOver, Qt::AlignCenter);
    layout->addWidget(btnStepNext, Qt::AlignCenter);
    layout->addWidget(btnStepOut, Qt::AlignCenter);
    layout->addWidget(btnMode, Qt::AlignCenter);
    layout->addSpacerItem(spacer);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
