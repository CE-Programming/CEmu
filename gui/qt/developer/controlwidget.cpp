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
#include <QtWidgets/QComboBox>

ControlWidget::ControlWidget(DevWidget *parent)
    : DevWidget{parent}
{
    QPushButton *btnRun = new QPushButton(tr("Stop"));
    QPushButton *btnStep = new QPushButton(tr("Step"));
    QPushButton *btnStepOver = new QPushButton(tr("Over"));
    QPushButton *btnStepNext = new QPushButton(tr("Next"));
    QPushButton *btnStepOut = new QPushButton(tr("Out"));
    QComboBox *cmbMode = new QComboBox;
    cmbMode->addItems({ tr("ASM"), tr("C"), tr("Disable") });

    btnRun->setMinimumWidth(5);
    btnStep->setMinimumWidth(5);
    btnStepOver->setMinimumWidth(5);
    btnStepNext->setMinimumWidth(5);
    btnStepOut->setMinimumWidth(5);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setSizeConstraint(QLayout::SetMaximumSize);
    hLayout->addStretch(1);
    hLayout->addWidget(btnRun);
    hLayout->addWidget(btnStep);
    hLayout->addWidget(btnStepOver);
    hLayout->addWidget(btnStepNext);
    hLayout->addWidget(btnStepOut);
    hLayout->addWidget(cmbMode);
    hLayout->addStretch(1);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->setSizeConstraint(QLayout::SetMaximumSize);
    vLayout->addStretch(1);
    vLayout->addLayout(hLayout);
    vLayout->addStretch(1);
    setLayout(vLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
