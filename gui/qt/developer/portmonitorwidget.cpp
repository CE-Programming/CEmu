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

#include "portmonitorwidget.h"

#include "../dockedwidget.h"
#include "../util.h"

#include <kddockwidgets/DockWidget.h>

#include <QtGui/QColor>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QToolButton>

PortMonitorWidget::PortMonitorWidget(DockedWidgetList &list, const QList<PortMonitor> &portmonitors)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("Port Monitor")},
                   QIcon(QStringLiteral(":/assets/icons/cable_release.svg")),
                   list}
{
    mTbl = new QTableWidget(0, 6);
    mTbl->setHorizontalHeaderLabels({tr("E"), tr("R"), tr("W"), tr("F"), tr("Address"), tr("Data")});
    mTbl->horizontalHeader()->setStretchLastSection(true);
    mTbl->verticalHeader()->setDefaultSectionSize(QFontMetrics(Util::monospaceFont()).maxWidth());
    mTbl->horizontalHeader()->setDefaultSectionSize(QFontMetrics(Util::monospaceFont()).maxWidth() * 10);
    mTbl->horizontalHeader()->setMinimumSectionSize(mTbl->verticalHeader()->defaultSectionSize());
    mTbl->horizontalHeader()->setSectionResizeMode(Column::Enabled, QHeaderView::ResizeToContents);
    mTbl->horizontalHeader()->setSectionResizeMode(Column::Read, QHeaderView::ResizeToContents);
    mTbl->horizontalHeader()->setSectionResizeMode(Column::Write, QHeaderView::ResizeToContents);
    mTbl->horizontalHeader()->setSectionResizeMode(Column::Freeze, QHeaderView::ResizeToContents);
    mTbl->verticalHeader()->setVisible(false);
    mTbl->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    mTbl->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mTbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTbl->setDragDropMode(QAbstractItemView::InternalMove);
    mTbl->setDragDropOverwriteMode(false);
    mTbl->setDragEnabled(true);
    mTbl->setAcceptDrops(false);

    mBtnRemoveSelected = new QPushButton(tr("Remove selected"));

    mNormalBackground = QTableWidgetItem().background();

    QPushButton *btnAddPortMonitor = new QPushButton(tr("Add port monitor"));

    QHBoxLayout *hboxbtns = new QHBoxLayout;
    hboxbtns->addWidget(btnAddPortMonitor);
    hboxbtns->addStretch();
    hboxbtns->addWidget(mBtnRemoveSelected);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(hboxbtns);
    vLayout->addWidget(mTbl);
    setLayout(vLayout);

    foreach (const PortMonitor &portmonitor, portmonitors)
    {
        addPortMonitor(portmonitor, false);
    }

    connect(btnAddPortMonitor, &QPushButton::clicked, [this]
    {
        PortMonitor portmonitor = { PortMonitor::Mode::R | PortMonitor::Mode::W, 0 };
        addPortMonitor(portmonitor, true);
    });

    connect(mBtnRemoveSelected, &QPushButton::clicked, this, &PortMonitorWidget::removeSelected);
    connect(mTbl, &QTableWidget::itemChanged, this, &PortMonitorWidget::itemChanged);
    connect(mTbl, &QTableWidget::itemActivated, this, &PortMonitorWidget::itemActivated);
    connect(mTbl, &QTableWidget::itemPressed, this, &PortMonitorWidget::itemPressed);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}


void PortMonitorWidget::addPortMonitor(const PortMonitor &portmonitor, bool edit)
{
    QString portStr = Util::int2hex(portmonitor.port, Util::portByteWidth);

    if (mTbl->rowCount() == 0)
    {
        mBtnRemoveSelected->setEnabled(true);
    }

    QTableWidgetItem *e = new QTableWidgetItem;
    QTableWidgetItem *r = new QTableWidgetItem;
    QTableWidgetItem *w = new QTableWidgetItem;
    QTableWidgetItem *f = new QTableWidgetItem;
    QTableWidgetItem *port = new QTableWidgetItem(portStr);
    QTableWidgetItem *data = new QTableWidgetItem;

    e->setFlags(e->flags() & ~Qt::ItemIsEditable);
    r->setFlags(r->flags() & ~Qt::ItemIsEditable);
    w->setFlags(w->flags() & ~Qt::ItemIsEditable);
    f->setFlags(f->flags() & ~Qt::ItemIsEditable);

    e->setTextAlignment(Qt::AlignCenter);
    r->setTextAlignment(Qt::AlignCenter);
    w->setTextAlignment(Qt::AlignCenter);
    f->setTextAlignment(Qt::AlignCenter);

    e->setFont(Util::monospaceFont());
    r->setFont(Util::monospaceFont());
    w->setFont(Util::monospaceFont());
    f->setFont(Util::monospaceFont());
    port->setFont(Util::monospaceFont());
    data->setFont(Util::monospaceFont());

    mTbl->blockSignals(true);
    mTbl->insertRow(0);
    mTbl->setItem(0, Column::Enabled, e);
    mTbl->setItem(0, Column::Read, w);
    mTbl->setItem(0, Column::Write, r);
    mTbl->setItem(0, Column::Freeze, f);
    mTbl->setItem(0, Column::Port, port);
    mTbl->setItem(0, Column::Data, data);
    mTbl->blockSignals(false);

    if (edit)
    {
        mTbl->editItem(port);
    }

    setPortMonitorMode(0, portmonitor.mode);
}

int PortMonitorWidget::getPortMonitorMode(int row)
{
    return mTbl->item(row, Column::Enabled)->data(Qt::UserRole).toInt();
}

void PortMonitorWidget::setPortMonitorMode(int row, int mode)
{
    const QString space = QStringLiteral(" ");

    mTbl->item(row, Column::Enabled)->setText(space);
    mTbl->item(row, Column::Read)->setText(space);
    mTbl->item(row, Column::Write)->setText(space);
    mTbl->item(row, Column::Freeze)->setText(space);
    mTbl->item(row, Column::Enabled)->setData(Qt::UserRole, mode);

    if (mode & PortMonitor::Mode::E)
    {
        mTbl->item(row, Column::Enabled)->setText(QStringLiteral("e"));
    }
    if (mode & PortMonitor::Mode::R)
    {
        mTbl->item(row, Column::Read)->setText(QStringLiteral("r"));
    }
    if (mode & PortMonitor::Mode::W)
    {
        mTbl->item(row, Column::Write)->setText(QStringLiteral("w"));
    }
    if (mode & PortMonitor::Mode::F)
    {
        mTbl->item(row, Column::Freeze)->setText(QStringLiteral("f"));
    }
}

void PortMonitorWidget::removeSelected()
{
    Q_ASSERT(mTbl->rowCount() != 0);

    for (int i = mTbl->rowCount() - 1; i >= 0; --i)
    {
        if (mTbl->item(i, Column::Port)->isSelected())
        {
            mTbl->removeRow(i);
        }
    }

    if (mTbl->rowCount() == 0)
    {
        mBtnRemoveSelected->setEnabled(false);
    }
}

bool PortMonitorWidget::toggleMode(int row, int bit)
{
    int mode = getPortMonitorMode(row);
    if (mode & bit)
    {
        setPortMonitorMode(row, mode & ~bit);
        return false;
    }

    setPortMonitorMode(row, mode | bit);
    return true;
}

void PortMonitorWidget::itemPressed(QTableWidgetItem *item)
{
    int bit;

    switch (item->column())
    {
        default:
            return;
        case Column::Enabled:
            bit = PortMonitor::Mode::E;
            break;
        case Column::Read:
            bit = PortMonitor::Mode::R;
            break;
        case Column::Write:
            bit = PortMonitor::Mode::W;
            break;
        case Column::Freeze:
            bit = PortMonitor::Mode::F;
            break;
    }

    mTbl->blockSignals(true);

    bool set = toggleMode(item->row(), bit);

    foreach(QTableWidgetItem *selected, mTbl->selectedItems())
    {
        if (selected->column() == item->column())
        {
            int mode = getPortMonitorMode(selected->row());
            if (set)
            {
                setPortMonitorMode(selected->row(), mode | bit);
            }
            else
            {
                setPortMonitorMode(selected->row(), mode & ~bit);
            }
        }
    }

    mTbl->blockSignals(false);
}

void PortMonitorWidget::itemActivated(QTableWidgetItem *item)
{
    switch (item->column())
    {
        default:
            break;
        case Column::Port:
            mPrevPort = item->text();
            break;
    }
}

void PortMonitorWidget::itemChanged(QTableWidgetItem *item)
{
    const QBrush invalidItemBrush(QColor(Qt::red).lighter());
    int row = item->row();

    switch (item->column())
    {
        default:
            break;
        case Column::Port:
            if (Util::isHexPort(item->text()))
            {
                mTbl->blockSignals(true);
                item->setText(Util::int2hex(Util::hex2int(item->text()), Util::addrByteWidth));
                mTbl->blockSignals(false);
                setPortMonitorMode(row, getPortMonitorMode(row) | PortMonitor::Mode::E);
                mTbl->item(row, Column::Port)->setBackground(mNormalBackground);
                mTbl->item(row, Column::Enabled)->setBackground(mNormalBackground);
            }
            else
            {
                setPortMonitorMode(row, getPortMonitorMode(row) & ~PortMonitor::Mode::E);
                mTbl->item(row, Column::Port)->setBackground(invalidItemBrush);
                mTbl->item(row, Column::Enabled)->setBackground(invalidItemBrush);
                mTbl->item(row, Column::Data)->setText(QString());
            }
            break;
    }
}
