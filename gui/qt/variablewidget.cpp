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

#include "variablewidget.h"
#include "tablewidget.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QTableWidget>

VariableWidget::VariableWidget(CoreWindow *coreWindow)
    : DockedWidget{new KDDockWidgets::QtWidgets::DockWidget{QStringLiteral("Calculator Variables")},
                   QIcon(QStringLiteral(":/assets/icons/opened_folder.svg")),
                   coreWindow}
{
    mCalcVars = new TableWidget(0, 3);
    mCalcVars->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Preview")});
    mCalcVars->horizontalHeader()->setStretchLastSection(true);
    mCalcVars->verticalHeader()->setVisible(false);
    mCalcVars->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mCalcVars->setSelectionBehavior(QAbstractItemView::SelectRows);

    mBtnSaveSelected = new QPushButton(QIcon(QStringLiteral(":/assets/icons/save.svg")), tr("Save"));
    mBtnSaveGroup = new QPushButton(QIcon(QStringLiteral(":/assets/icons/save.svg")), tr("Save as group"));

    mBtnSaveSelected->setEnabled(false);
    mBtnSaveGroup->setEnabled(false);

    QPushButton *btnRefreshList = new QPushButton(QIcon(QStringLiteral(":/assets/icons/process.svg")), tr("Refresh list"));

    QHBoxLayout *hboxCalcBtns = new QHBoxLayout;
    hboxCalcBtns->addWidget(btnRefreshList);
    hboxCalcBtns->addStretch(1);
    hboxCalcBtns->addWidget(mBtnSaveSelected);
    hboxCalcBtns->addWidget(mBtnSaveGroup);

    QVBoxLayout *vboxCalc = new QVBoxLayout;
    vboxCalc->addLayout(hboxCalcBtns);
    vboxCalc->addWidget(mCalcVars);

    setLayout(vboxCalc);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
