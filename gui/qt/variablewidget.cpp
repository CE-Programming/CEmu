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

#include "variablewidget.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QTableWidget>

VariableWidget::VariableWidget(DockedWidgetList &list, const QStringList &recentVars)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("Variable Transfer")}, list}
{
    mCalcVars = new QTableWidget(0, 3);
    mCalcVars->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Preview")});
    mCalcVars->horizontalHeader()->setStretchLastSection(true);
    mCalcVars->verticalHeader()->setVisible(false);
    mCalcVars->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mCalcVars->setSelectionBehavior(QAbstractItemView::SelectRows);

    mSentVars = new QTableWidget(0, 1);
    mSentVars->setHorizontalHeaderLabels({tr("Recently Sent")});
    mSentVars->horizontalHeader()->setStretchLastSection(true);
    mSentVars->verticalHeader()->setVisible(false);
    mSentVars->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mSentVars->setSelectionBehavior(QAbstractItemView::SelectRows);

    mBtnSaveSelected = new QPushButton(tr("Save selected"));
    mBtnSaveGroup = new QPushButton(tr("Save selected as group"));
    mBtnResendVars = new QPushButton(tr("Resend selected"));
    mBtnRemoveVars = new QPushButton(tr("Remove selected"));

    mBtnResendVars->setEnabled(false);
    mBtnRemoveVars->setEnabled(false);
    mBtnSaveSelected->setEnabled(false);
    mBtnSaveGroup->setEnabled(false);

    QGroupBox *grpSent = new QGroupBox(tr("Send variables to calculator"));
    QGroupBox *grpCalc = new QGroupBox(tr("Get variables from calculator"));

    QPushButton *btnSendVars = new QPushButton(tr("Send variables"));
    QPushButton *btnRefreshList = new QPushButton(tr("Refresh list"));

    QHBoxLayout *hboxSentBtns = new QHBoxLayout;
    hboxSentBtns->addWidget(btnSendVars);
    hboxSentBtns->addStretch(1);
    hboxSentBtns->addWidget(mBtnResendVars);
    hboxSentBtns->addWidget(mBtnRemoveVars);

    QVBoxLayout *vboxSent = new QVBoxLayout;
    vboxSent->addLayout(hboxSentBtns);
    vboxSent->addWidget(mSentVars);
    grpSent->setLayout(vboxSent);

    QHBoxLayout *hboxCalcBtns = new QHBoxLayout;
    hboxCalcBtns->addWidget(btnRefreshList);
    hboxCalcBtns->addStretch(1);
    hboxCalcBtns->addWidget(mBtnSaveSelected);
    hboxCalcBtns->addWidget(mBtnSaveGroup);

    QVBoxLayout *vboxCalc = new QVBoxLayout;
    vboxCalc->addLayout(hboxCalcBtns);
    vboxCalc->addWidget(mCalcVars);
    grpCalc->setLayout(vboxCalc);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(grpSent);
    vLayout->addWidget(grpCalc);
    setLayout(vLayout);

    foreach (const QString &var, recentVars)
    {
        addRecentVar(var);
    }

    connect(mBtnRemoveVars, &QPushButton::clicked, this, &VariableWidget::removeRecentSelected);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void VariableWidget::addRecentVar(const QString &path)
{
    if (mSentVars->rowCount() == 0)
    {
        mBtnResendVars->setEnabled(true);
        mBtnRemoveVars->setEnabled(true);
    }

    QTableWidgetItem *item = new QTableWidgetItem(path);
    mSentVars->insertRow(0);
    mSentVars->setItem(0, 0, item);
}

void VariableWidget::removeRecentSelected()
{
    Q_ASSERT(mSentVars->rowCount() != 0);

    for (int i = mSentVars->rowCount() - 1; i >= 0; --i)
    {
        if (mSentVars->item(i, 0)->isSelected())
        {
            mSentVars->removeRow(i);
        }
    }

    if (mSentVars->rowCount() == 0)
    {
        mBtnResendVars->setEnabled(false);
        mBtnRemoveVars->setEnabled(false);
    }
}
