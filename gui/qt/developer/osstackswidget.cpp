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

OsStacksWidget::OsStacksWidget(QWidget *parent)
    : QWidget{parent}
{
    QGroupBox *grpFp = new QGroupBox(QStringLiteral("FP Stack"));
    QGroupBox *grpOp = new QGroupBox(tr("OP Stack"));

    QTableWidget *tblFp = new QTableWidget(0, 4);
    tblFp->setHorizontalHeaderLabels({"Address", "Data", "String", "Value"});
    tblFp->horizontalHeader()->setStretchLastSection(true);

    QTableWidget *tblOp = new QTableWidget(0, 2);
    tblOp->setHorizontalHeaderLabels({"Address", "Data"});
    tblOp->horizontalHeader()->setStretchLastSection(true);

    QHBoxLayout *hboxFp = new QHBoxLayout;
    hboxFp->addWidget(tblFp);
    grpFp->setLayout(hboxFp);

    QHBoxLayout *hboxOp = new QHBoxLayout;
    hboxOp->addWidget(tblOp);
    grpOp->setLayout(hboxOp);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(grpFp);
    vLayout->addWidget(grpOp);
    setLayout(vLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
