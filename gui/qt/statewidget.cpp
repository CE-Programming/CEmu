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

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QTableWidget>

StateWidget::StateWidget(QWidget *parent)
    : QWidget{parent}
{
    mTbl = new QTableWidget(0, 1);
    mTbl->setHorizontalHeaderLabels({ tr("State name") });
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

    QDir stateDir(Settings::textOption(Settings::StatesPath));
    if (stateDir.exists())
    {
        stateDir.setNameFilters({QStringLiteral("*.ce")});
        stateDir.setFilter(QDir::Files | QDir::Readable);

        foreach (const QString &state, stateDir.entryList())
        {
            addState(QFileInfo(state).baseName(), false);
        }
    }

    connect(mBtnRemoveSelected, &QPushButton::clicked, this, &StateWidget::removeSelected);
    connect(btnCreateState, &QPushButton::clicked, this, &StateWidget::createState);
    connect(mTbl, &QTableWidget::itemSelectionChanged, [this]
    {
        bool enable = mTbl->selectedItems().count() == 1;
        mBtnExportSelected->setEnabled(enable);
        mBtnRestoreSelected->setEnabled(enable);
    });
    connect(mTbl, &QTableWidget::itemChanged, [this](QTableWidgetItem *item)
    {
        mTbl->blockSignals(true);
        QString oldState = item->data(Qt::UserRole).toString();
        QString newState = item->text();
        QString oldPath = getStatePath(oldState);
        QString newPath = getStatePath(newState);
        if (!QFile::rename(oldPath, newPath))
        {
            bool exists = QFileInfo(oldPath).exists();
            QString error = !exists ? tr("State was removed from disk!")
                                    : tr("Could not rename state.");
            QMessageBox::critical(nullptr, Util::error, error);
            item->setText(oldState);
            if (!exists)
            {
                mTbl->removeRow(item->row());
            }
        }
        else
        {
            item->setData(Qt::UserRole, newState);
        }
        mTbl->blockSignals(false);
    });

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QString StateWidget::getStatePath(const QString &state) const
{
    return Settings::textOption(Settings::StatesPath) + '/' + state + '.' + Util::stateExtension;
}

void StateWidget::addState(const QString &state, bool edit)
{
    QString statePath = getStatePath(state);

    if (!QFileInfo(statePath).exists())
    {
        return;
    }

    if (mTbl->rowCount() == 0)
    {
        mBtnRemoveSelected->setEnabled(true);
    }

    QTableWidgetItem *item = new QTableWidgetItem(state);
    item->setData(Qt::UserRole, state);

    mTbl->blockSignals(true);
    mTbl->insertRow(0);
    mTbl->setItem(0, 0, item);
    mTbl->blockSignals(false);

    if (edit)
    {
        mTbl->editItem(item);
    }
}

void StateWidget::createState()
{
    QString stateDir = Settings::textOption(Settings::StatesPath);
    QString state = Util::randomString(6);
    QDir dir;

    if (!dir.exists(stateDir))
    {
        dir.mkpath(stateDir);
    }

    // temporary for testing
    QFile file(getStatePath(state));
    file.open(QIODevice::WriteOnly);
    file.putChar('E');
    file.close();

    addState(state, true);
}

void StateWidget::removeSelected()
{
    Q_ASSERT(mTbl->rowCount() != 0);

    for (int i = mTbl->rowCount() - 1; i >= 0; --i)
    {
        QTableWidgetItem *item = mTbl->item(i, 0);
        if (item->isSelected())
        {
            QFile::remove(getStatePath(item->text()));
            mTbl->removeRow(i);
        }
    }

    if (mTbl->rowCount() == 0)
    {
        mBtnRemoveSelected->setEnabled(false);
    }
}
