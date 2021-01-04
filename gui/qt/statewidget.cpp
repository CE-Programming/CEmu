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

#include <kddockwidgets/DockWidget.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QTableWidget>

const QString StateWidget::sDefaultStateName = tr("State");
const QString StateWidget::sStateExtension = QStringLiteral(".cemu");

StateWidget::StateWidget(DockedWidgetList &list)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("States")},
                   QIcon(QStringLiteral(":/assets/icons/filing_cabinet.svg")),
                   list},
      mStateNum{1}
{
    mTbl = new QTableWidget(0, 1);
    mTbl->setHorizontalHeaderLabels({ tr("State name") });
    mTbl->horizontalHeader()->setStretchLastSection(true);
    mTbl->verticalHeader()->setVisible(false);
    mTbl->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mTbl->setSelectionBehavior(QAbstractItemView::SelectRows);

    mBtnExport = new QPushButton(QIcon(QStringLiteral(":/assets/icons/external.svg")), tr("Export state"));
    mBtnImport = new QPushButton(QIcon(QStringLiteral(":/assets/icons/internal.svg")), tr("Import state"));

    mBtnRestore = new QPushButton(QIcon(QStringLiteral(":/assets/icons/synchronize.svg")), tr("Restore"));
    mBtnRemove = new QPushButton(QIcon(QStringLiteral(":/assets/icons/cross.svg")), tr("Remove"));

    QPushButton *btnCreateState = new QPushButton(QIcon(QStringLiteral(":/assets/icons/add_row.svg")), tr("Save state"));

    QHBoxLayout *hboxTopBtns = new QHBoxLayout;
    hboxTopBtns->addWidget(btnCreateState);
    hboxTopBtns->addStretch();
    hboxTopBtns->addWidget(mBtnRestore);
    hboxTopBtns->addWidget(mBtnRemove);

    QHBoxLayout *hboxBotBtns = new QHBoxLayout;
    hboxBotBtns->addWidget(mBtnImport);
    hboxBotBtns->addWidget(mBtnExport);
    hboxBotBtns->addStretch();

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(hboxTopBtns);
    vLayout->addWidget(mTbl);
    vLayout->addLayout(hboxBotBtns);
    setLayout(vLayout);

    QDir stateDir(Settings::textOption(Settings::StatesPath));
    if (stateDir.exists())
    {
        stateDir.setNameFilters({'*' + sStateExtension});
        stateDir.setFilter(QDir::Files | QDir::Readable);

        foreach (const QString &state, stateDir.entryList())
        {
            addState(QFileInfo(state).baseName(), false);
        }
    }

    connect(mBtnRemove, &QPushButton::clicked, this, &StateWidget::removeSelected);
    connect(mBtnImport, &QPushButton::clicked, this, &StateWidget::importState);
    connect(mBtnExport, &QPushButton::clicked, this, &StateWidget::exportState);
    connect(btnCreateState, &QPushButton::clicked, this, &StateWidget::createState);
    connect(mTbl, &QTableWidget::itemSelectionChanged, [this]
    {
        int count = mTbl->selectedItems().count();
        bool enable = count == 1;
        mBtnExport->setEnabled(enable);
        mBtnRestore->setEnabled(enable);
        mBtnRemove->setEnabled(count >= 1);
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
    return Settings::textOption(Settings::StatesPath) + '/' + state + sStateExtension;
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
        mBtnRemove->setEnabled(true);
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
    QDir dir;

    if (!dir.exists(stateDir))
    {
        dir.mkpath(stateDir);
    }

    QString state = sDefaultStateName + QString::number(mStateNum);
    QString dstPath = getStatePath(state);
    while (QFileInfo(dstPath).exists())
    {
        state = sDefaultStateName + QString::number(mStateNum++);
        dstPath = getStatePath(state);
    }

    // temporary for testing
    QFile file(getStatePath(state));
    file.open(QIODevice::WriteOnly);
    file.putChar('S');
    file.close();

    addState(state, true);
}

void StateWidget::importState()
{
    QFileDialog dialog(this, tr("Import Calculator State"));
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setDefaultSuffix(sStateExtension);
    dialog.setNameFilter('*' + sStateExtension);

    if (dialog.exec())
    {
        QString srcPath = dialog.selectedFiles().first();
        QString baseName = QFileInfo(srcPath).baseName();
        QString dstPath = getStatePath(baseName);
        int copy = 0;

        while (QFileInfo(dstPath).exists())
        {
            baseName = QFileInfo(srcPath).baseName() + QString::number(copy++);
            dstPath = getStatePath(baseName);
        }

        if (QFile::copy(srcPath, dstPath))
        {
            addState(baseName, true);
        }
        else
        {
            QMessageBox::critical(nullptr, Util::error, tr("Could not import state."));
        }
    }
}

void StateWidget::exportState()
{
    QFileDialog dialog(this, tr("Export Calculator State"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix(sStateExtension);
    dialog.setNameFilter('*' + sStateExtension);

    if (dialog.exec())
    {
        QTableWidgetItem *item = mTbl->selectedItems().first();

        QString srcPath = getStatePath(item->text());
        QString dstPath = dialog.selectedFiles().first();

        QFile::copy(srcPath, dstPath);
    }
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
        mBtnRemove->setEnabled(false);
    }
}
