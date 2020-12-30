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

#include "statewidget.h"
#include "settings.h"
#include "util.h"

#include <QtCore/QFileInfo>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QTableWidget>

StateWidget::StateWidget(const QList<State> &states, QWidget *parent)
    : QWidget{parent}
{
    mTbl = new QTableWidget(0, 1);
    mTbl->setHorizontalHeaderLabels({ tr("Name") });
    mTbl->horizontalHeader()->setStretchLastSection(true);
    mTbl->verticalHeader()->setVisible(false);
    mTbl->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mTbl->setSelectionBehavior(QAbstractItemView::SelectRows);

    mBtnExportSelected = new QPushButton(tr("Export"));
    mBtnRestoreSelected = new QPushButton(tr("Restore"));
    mBtnRemoveSelected = new QPushButton(tr("Remove selected"));

    QPushButton *btnCreateState = new QPushButton(tr("Save calculator state"));

    QHBoxLayout *hboxBtns = new QHBoxLayout;
    hboxBtns->addWidget(btnCreateState);
    hboxBtns->addStretch(1);
    hboxBtns->addWidget(mBtnExportSelected);
    hboxBtns->addWidget(mBtnRestoreSelected);
    hboxBtns->addWidget(mBtnRemoveSelected);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(hboxBtns);
    vLayout->addWidget(mTbl);
    setLayout(vLayout);

    foreach (const State &state, states)
    {
        addState(state, false);
    }

    connect(mBtnRemoveSelected, &QPushButton::clicked, this, &StateWidget::removeSelected);
    connect(btnCreateState, &QPushButton::clicked, [this]
    {
        State state = { Settings::textOption(Settings::StatesPath) + '/' + Util::randomString(6) + '.' + Util::stateExtension };
        addState(state, true);
    });
    connect(mTbl, &QTableWidget::itemSelectionChanged, [this]
    {
        bool enable = mTbl->selectedItems().count() == 1;
        mBtnExportSelected->setEnabled(enable);
        mBtnRestoreSelected->setEnabled(enable);
    });

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void StateWidget::addState(const State &state, bool edit)
{
    QString name = QFileInfo(state.path).baseName();

    if (mTbl->rowCount() == 0)
    {
        mBtnRemoveSelected->setEnabled(true);
    }

    QTableWidgetItem *item = new QTableWidgetItem(name);
    item->setData(Qt::UserRole, state.path);

    mTbl->insertRow(0);
    mTbl->setItem(0, 0, item);

    if (edit)
    {
        mTbl->editItem(item);
    }
}

void StateWidget::removeSelected()
{
    Q_ASSERT(mTbl->rowCount() != 0);

    for (int i = mTbl->rowCount() - 1; i >= 0; --i)
    {
        if (mTbl->item(i, 0)->isSelected())
        {
            mTbl->removeRow(i);
        }
    }

    if (mTbl->rowCount() == 0)
    {
        mBtnRemoveSelected->setEnabled(false);
    }
}
