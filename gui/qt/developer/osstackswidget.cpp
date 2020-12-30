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

#include "osstackswidget.h"

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QTableWidget>

OsStacksWidget::OsStacksWidget(DevWidget *parent)
    : DevWidget{parent}
{
    QGroupBox *grpFp = new QGroupBox(QStringLiteral("FP Stack"));
    QGroupBox *grpOp = new QGroupBox(tr("OP Stack"));

    mTblFp = new QTableWidget(0, 4);
    mTblFp->setHorizontalHeaderLabels({"Address", "Data", "String", "Value"});
    mTblFp->horizontalHeader()->setStretchLastSection(true);

    mTblOp = new QTableWidget(0, 2);
    mTblOp->setHorizontalHeaderLabels({"Address", "Data"});
    mTblOp->horizontalHeader()->setStretchLastSection(true);

    QHBoxLayout *hboxFp = new QHBoxLayout;
    hboxFp->addWidget(mTblFp);
    grpFp->setLayout(hboxFp);

    QHBoxLayout *hboxOp = new QHBoxLayout;
    hboxOp->addWidget(mTblOp);
    grpOp->setLayout(hboxOp);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(grpFp);
    vLayout->addWidget(grpOp);
    setLayout(vLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
