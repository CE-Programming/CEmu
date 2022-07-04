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

#include "portmonitorwidget.h"

#include "../corewrapper.h"
#include "../tablewidget.h"
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

PortMonitorWidget::PortMonitorWidget(CoreWindow *coreWindow, const QList<PortMonitor> &portmonitors)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("Port Monitor")},
                   QIcon(QStringLiteral(":/assets/icons/cable_release.svg")),
                   coreWindow}
{
    mTbl = new TableWidget(0, 5);
    mTbl->setHorizontalHeaderLabels({tr("E"), tr("R"), tr("W"), tr("Port"), tr("Data")});
    mTbl->horizontalHeader()->setStretchLastSection(true);
    mTbl->horizontalHeader()->setDefaultSectionSize(QFontMetrics(Util::monospaceFont()).maxWidth() * 10);
    mTbl->horizontalHeader()->setMinimumSectionSize(mTbl->verticalHeader()->defaultSectionSize());
    mTbl->horizontalHeader()->setSectionResizeMode(Column::Enabled, QHeaderView::ResizeToContents);
    mTbl->horizontalHeader()->setSectionResizeMode(Column::Read, QHeaderView::ResizeToContents);
    mTbl->horizontalHeader()->setSectionResizeMode(Column::Write, QHeaderView::ResizeToContents);
    mTbl->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    mTbl->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mTbl->setSelectionBehavior(QAbstractItemView::SelectRows);

    mBtnRemoveSelected = new QPushButton(QIcon(QStringLiteral(":/assets/icons/cross.svg")), tr("Remove selected"));
    mBtnRemoveSelected->setEnabled(false);

    mNormalBackground = QTableWidgetItem().background();

    QPushButton *btnAddPortMonitor = new QPushButton(QIcon(QStringLiteral(":/assets/icons/plus.svg")), tr("Add port monitor"));

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
    connect(mTbl, &TableWidget::deletePressed, this, &PortMonitorWidget::removeSelected);
    connect(mTbl, &QTableWidget::itemChanged, this, &PortMonitorWidget::itemChanged);
    connect(mTbl, &QTableWidget::itemPressed, this, &PortMonitorWidget::itemPressed);
    connect(mTbl, &QTableWidget::itemSelectionChanged, [this]
    {
        mBtnRemoveSelected->setEnabled(mTbl->selectedItems().count() >= 1);
    });

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    enableDebugWidgets(false);
}


void PortMonitorWidget::addPortMonitor(const PortMonitor &portmonitor, bool edit)
{
    int id = core().get(cemucore::CEMUCORE_PROP_DEBUG_WATCH, -1);
    if (id == -1)
    {
        return;
    }

    QString portStr = Util::int2hex(portmonitor.port, Util::portByteWidth);

    QTableWidgetItem *e = new QTableWidgetItem;
    QTableWidgetItem *r = new QTableWidgetItem;
    QTableWidgetItem *w = new QTableWidgetItem;
    QTableWidgetItem *port = new QTableWidgetItem(portStr);
    QTableWidgetItem *data = new QTableWidgetItem;

    e->setData(Role::Id, id);

    e->setFlags(e->flags() & ~Qt::ItemIsEditable);
    r->setFlags(r->flags() & ~Qt::ItemIsEditable);
    w->setFlags(w->flags() & ~Qt::ItemIsEditable);

    e->setTextAlignment(Qt::AlignCenter);
    r->setTextAlignment(Qt::AlignCenter);
    w->setTextAlignment(Qt::AlignCenter);

    e->setFont(Util::monospaceFont());
    r->setFont(Util::monospaceFont());
    w->setFont(Util::monospaceFont());
    port->setFont(Util::monospaceFont());
    data->setFont(Util::monospaceFont());

    mTbl->blockSignals(true);
    mTbl->insertRow(0);
    mTbl->setItem(0, Column::Enabled, e);
    mTbl->setItem(0, Column::Read, w);
    mTbl->setItem(0, Column::Write, r);
    mTbl->setItem(0, Column::Port, port);
    mTbl->setItem(0, Column::Data, data);
    mTbl->setVerticalHeaderItem(0, new QTableWidgetItem(QIcon(QStringLiteral(":/assets/icons/updown.svg")), QString()));
    mTbl->blockSignals(false);

    if (edit)
    {
        mTbl->editItem(port);
    }

    setPortMonitorMode(0, portmonitor.mode);
}

int PortMonitorWidget::getPortMonitorMode(int row)
{
    return mTbl->item(row, Column::Enabled)->data(Role::Mode).toInt();
}

void PortMonitorWidget::setPortMonitorMode(int row, int mode)
{
    const QString space = QStringLiteral(" ");
    int id = mTbl->item(row, Column::Enabled)->data(Role::Id).toInt();

    QString portStr = mTbl->item(row, Column::Port)->text();
    if (Util::isHexPort(portStr))
    {
        core().set(cemucore::CEMUCORE_PROP_DEBUG_WATCH_ADDR, id, Util::hex2int(portStr));
    }

    mTbl->item(row, Column::Enabled)->setText(space);
    mTbl->item(row, Column::Read)->setText(space);
    mTbl->item(row, Column::Write)->setText(space);
    mTbl->item(row, Column::Enabled)->setData(Role::Mode, mode);

    QFlags<cemucore::debug_flags> flags;
    flags |= cemucore::CEMUCORE_DEBUG_WATCH_PORT;
    flags |= cemucore::CEMUCORE_DEBUG_WATCH_ANY;
    if (mode & PortMonitor::Mode::E)
    {
        mTbl->item(row, Column::Enabled)->setText(QStringLiteral("e"));
        flags |= cemucore::CEMUCORE_DEBUG_WATCH_ENABLE;
    }
    if (mode & PortMonitor::Mode::R)
    {
        mTbl->item(row, Column::Read)->setText(QStringLiteral("r"));
        flags |= cemucore::CEMUCORE_DEBUG_WATCH_READ;
    }
    if (mode & PortMonitor::Mode::W)
    {
        mTbl->item(row, Column::Write)->setText(QStringLiteral("w"));
        flags |= cemucore::CEMUCORE_DEBUG_WATCH_WRITE;
    }
    core().set(cemucore::CEMUCORE_PROP_DEBUG_WATCH_FLAGS, id, flags);
}

void PortMonitorWidget::removeSelected()
{
    Q_ASSERT(mTbl->rowCount() != 0);

    for (int i = mTbl->rowCount() - 1; i >= 0; --i)
    {
        if (mTbl->item(i, Column::Port)->isSelected())
        {
            int id = mTbl->item(i, Column::Enabled)->data(Role::Id).toInt();
            core().set(cemucore::CEMUCORE_PROP_DEBUG_WATCH, id, -1);
            mTbl->removeRow(i);
        }
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

void PortMonitorWidget::itemChanged(QTableWidgetItem *item)
{
    const QBrush invalidItemBrush(QColor(Qt::red).lighter());
    int row = item->row();

    QTableWidgetItem *dataItem = mTbl->item(row, Column::Data);
    QTableWidgetItem *portItem = mTbl->item(row, Column::Port);

    switch (item->column())
    {
        default:
            break;
        case Column::Port:
            mTbl->blockSignals(true);
            if (Util::isHexPort(portItem->text()))
            {
                uint32_t port = Util::hex2int(portItem->text());

                portItem->setText(Util::int2hex(Util::hex2int(portItem->text()), Util::addrByteWidth));
                setPortMonitorMode(row, getPortMonitorMode(row) | PortMonitor::Mode::E);
                mTbl->item(row, Column::Port)->setBackground(mNormalBackground);
                mTbl->item(row, Column::Enabled)->setBackground(mNormalBackground);
                if (mInDebug)
                {
                    dataItem->setText(Util::int2hex(core().get(cemucore::CEMUCORE_PROP_PORT, port), 2));
                    dataItem->setFlags(dataItem->flags() | Qt::ItemIsEnabled);
                }
            }
            else
            {
                setPortMonitorMode(row, getPortMonitorMode(row) & ~PortMonitor::Mode::E);
                mTbl->item(row, Column::Port)->setBackground(invalidItemBrush);
                mTbl->item(row, Column::Enabled)->setBackground(invalidItemBrush);
                dataItem->setText(QString());
                dataItem->setFlags(dataItem->flags() & ~Qt::ItemIsEnabled);
            }
            mTbl->blockSignals(false);
            break;
        case Column::Data:
            if (!mInDebug)
            {
                break;
            }

            dataItem->setBackground(Util::isHexString(dataItem->text(), 0, 255) ? mNormalBackground :
                                                                                  invalidItemBrush);
            break;
    }
}

void PortMonitorWidget::enableDebugWidgets(bool enabled)
{
    mInDebug = enabled;

    for (int i = 0; i < mTbl->rowCount(); ++i)
    {
        QTableWidgetItem *item = mTbl->item(i, Column::Data);

        QString portStr = mTbl->item(i, Column::Port)->text();
        QString dataStr = item->text();

        if (enabled &&
            Util::isHexPort(portStr) &&
            Util::isHexString(dataStr, 0, 255))
        {
            item->setFlags(item->flags() | Qt::ItemIsEnabled);
        }
        else
        {
            item->setText(QString());
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        }
    }
}

void PortMonitorWidget::loadFromCore(const CoreWrapper &core)
{
    for (int i = 0; i < mTbl->rowCount(); ++i)
    {
        uint32_t port = Util::hex2int(mTbl->item(i, Column::Port)->text());
        mTbl->item(i, Column::Data)->setText(Util::int2hex(core.get(cemucore::CEMUCORE_PROP_PORT, port), 2));
    }
}

void PortMonitorWidget::storeToCore(CoreWrapper &core) const
{
    for (int i = 0; i < mTbl->rowCount(); ++i)
    {
        QString portStr = mTbl->item(i, Column::Port)->text();
        QString dataStr = mTbl->item(i, Column::Data)->text();

        if (Util::isHexPort(portStr) && Util::isHexString(dataStr, 0, 255))
        {
            uint32_t port = Util::hex2int(portStr);
            uint32_t data = Util::hex2int(dataStr);

            core.set(cemucore::CEMUCORE_PROP_PORT, port, data);
        }
    }
}
