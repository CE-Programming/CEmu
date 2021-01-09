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

#include "../util.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QComboBox>

ControlWidget::ControlWidget(CoreWindow *coreWindow)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("Control")},
                   QIcon(QStringLiteral(":/assets/icons/services.svg")),
                   coreWindow}
{
    int maxMonoWidth = QFontMetrics(Util::monospaceFont()).maxWidth();

    QPushButton *btnRun = new QPushButton(QIcon(QStringLiteral(":/assets/icons/stop.svg")), tr("Stop"));
    QPushButton *btnStep = new QPushButton(QIcon(QStringLiteral(":/assets/icons/right.svg")), tr("Step"));
    QPushButton *btnStepOver = new QPushButton(QIcon(QStringLiteral(":/assets/icons/down_right.svg")), tr("Over"));
    QPushButton *btnStepNext = new QPushButton(QIcon(QStringLiteral(":/assets/icons/down.svg")), tr("Next"));
    QPushButton *btnStepOut = new QPushButton(QIcon(QStringLiteral(":/assets/icons/right_up2.svg")), tr("Out"));
    QComboBox *cmbMode = new QComboBox;
    cmbMode->addItems({ tr("ASM"), tr("C"), tr("Disable") });

    btnRun->setFont(Util::monospaceFont());
    btnStep->setFont(Util::monospaceFont());
    btnStepOver->setFont(Util::monospaceFont());
    btnStepNext->setFont(Util::monospaceFont());
    btnStepOut->setFont(Util::monospaceFont());
    cmbMode->setFont(Util::monospaceFont());

    btnRun->setMinimumWidth(btnRun->iconSize().width() + maxMonoWidth * (btnRun->text().length() + 2));
    btnStep->setMinimumWidth(btnStep->iconSize().width() + maxMonoWidth * (btnStep->text().length() + 2));
    btnStepOver->setMinimumWidth(btnStepOver->iconSize().width() + maxMonoWidth * (btnStepOver->text().length() + 2));
    btnStepNext->setMinimumWidth(btnStepNext->iconSize().width() + maxMonoWidth * (btnStepNext->text().length() + 2));
    btnStepOut->setMinimumWidth(btnStepOut->iconSize().width() + maxMonoWidth * (btnStepOut->text().length() + 2));

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setSizeConstraint(QLayout::SetMaximumSize);
    hLayout->addStretch();
    hLayout->addWidget(btnRun);
    hLayout->addWidget(btnStep);
    hLayout->addWidget(btnStepOver);
    hLayout->addWidget(btnStepNext);
    hLayout->addWidget(btnStepOut);
    hLayout->addWidget(cmbMode);
    hLayout->addStretch();

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->setSizeConstraint(QLayout::SetMaximumSize);
    vLayout->addStretch();
    vLayout->addLayout(hLayout);
    vLayout->addStretch();
    setLayout(vLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
