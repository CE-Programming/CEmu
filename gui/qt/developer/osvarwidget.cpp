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

#include "osvarswidget.h"

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QTableWidget>

OsVarsWidget::OsVarsWidget(QWidget *parent)
    : QWidget{parent}
{
    QGroupBox *grpVat = new QGroupBox(QStringLiteral("VAT View"));
    QGroupBox *grpOp = new QGroupBox(tr("OP Variables"));

    QTableWidget *tblFp = new QTableWidget(0, 5);
    tblFp->setHorizontalHeaderLabels({"Address", "VAT", "Size", "Name", "Type"});
    tblFp->horizontalHeader()->setStretchLastSection(true);

    QTableWidget *tblOp = new QTableWidget(0, 5);
    tblOp->setHorizontalHeaderLabels({"Address", "OP", "Data", "String", "Value"});
    tblOp->horizontalHeader()->setStretchLastSection(true);

    QHBoxLayout *hboxFp = new QHBoxLayout;
    hboxFp->addWidget(tblFp);
    grpVat->setLayout(hboxFp);

    QHBoxLayout *hboxOp = new QHBoxLayout;
    hboxOp->addWidget(tblOp);
    grpOp->setLayout(hboxOp);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(grpVat);
    vLayout->addWidget(grpOp);
    setLayout(vLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
