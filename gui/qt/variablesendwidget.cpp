/*
 * Copyright (c) 2015-2021 CE Programming.
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

#include "variablesendwidget.h"
#include "tablewidget.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QTableWidget>

VariableSendWidget::VariableSendWidget(CoreWindow *coreWindow, const QStringList &recentVars)
    : DockedWidget{new KDDockWidgets::QtWidgets::DockWidget{QStringLiteral("Send Variable")},
                   QIcon(QStringLiteral(":/assets/icons/opened_folder.svg")),
                   coreWindow}
{
    mSentVars = new TableWidget(0, 2);
    mSentVars->setHorizontalHeaderLabels({tr("Location"), tr("Path")});
    mSentVars->horizontalHeader()->setStretchLastSection(true);
    mSentVars->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mSentVars->setSelectionBehavior(QAbstractItemView::SelectRows);

    mBtnResendVars = new QPushButton(QIcon(QStringLiteral(":/assets/icons/internal.svg")), tr("Resend"));
    mBtnRemoveVars = new QPushButton(QIcon(QStringLiteral(":/assets/icons/cross.svg")), tr("Remove"));

    mBtnResendVars->setEnabled(false);
    mBtnRemoveVars->setEnabled(false);

    QPushButton *btnSendVars = new QPushButton(QIcon(QStringLiteral(":/assets/icons/opened_folder.svg")), tr("Send variables"));

    QHBoxLayout *hboxSentBtns = new QHBoxLayout;
    hboxSentBtns->addWidget(btnSendVars);
    hboxSentBtns->addStretch(1);
    hboxSentBtns->addWidget(mBtnResendVars);
    hboxSentBtns->addWidget(mBtnRemoveVars);

    QVBoxLayout *vboxSent = new QVBoxLayout;
    vboxSent->addLayout(hboxSentBtns);
    vboxSent->addWidget(mSentVars);

    setLayout(vboxSent);

    foreach (const QString &var, recentVars)
    {
        addRecentVar(var);
    }

    connect(mBtnRemoveVars, &QPushButton::clicked, this, &VariableSendWidget::removeRecentSelected);
    connect(mSentVars, &TableWidget::deletePressed, this, &VariableSendWidget::removeRecentSelected);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void VariableSendWidget::addRecentVar(const QString &path)
{
    if (mSentVars->rowCount() == 0)
    {
        mBtnResendVars->setEnabled(true);
        mBtnRemoveVars->setEnabled(true);
    }

    mSentVars->insertRow(0);
    mSentVars->setItem(0, 0, new QTableWidgetItem(path));
    mSentVars->setVerticalHeaderItem(0, new QTableWidgetItem(QIcon(QStringLiteral(":/assets/icons/updown.svg")), QString()));
}

void VariableSendWidget::removeRecentSelected()
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
