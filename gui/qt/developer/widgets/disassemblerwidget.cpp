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

#include "disassemblerwidget.h"

#include "../../corewrapper.h"
#include "../disassemblywidget.h"
#include "../../util.h"

#include <QtGui/QPainter>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QTextEdit>

DisassemblerWidget::DisassemblerWidget(DisassemblyWidget *parent)
    : QTableWidget{parent},
      mBoldFont{Util::monospaceFont()}
{
    mBoldFont.setBold(true);

    mDelegate = new DisassemblerWidgetDelegate{this};

    setColumnCount(Column::Count);
    setHorizontalHeaderItem(Column::Address, new QTableWidgetItem{tr("Address")});
    setHorizontalHeaderItem(Column::Data, new QTableWidgetItem{tr("Data")});
    setHorizontalHeaderItem(Column::Mnemonic, new QTableWidgetItem{tr("Mnemonic")});

    setItemDelegate(mDelegate);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setVisible(true);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setFont(Util::monospaceFont());

    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    verticalHeader()->setDefaultSectionSize(fontMetrics().height() + 4);
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    setShowGrid(false);

    setAddress(512);

    connect(verticalScrollBar(), &QScrollBar::actionTriggered, this, &DisassemblerWidget::scrollAction);
    connect(verticalHeader(), &QHeaderView::sectionClicked, this, &DisassemblerWidget::toggleBreakpoint);
}

DisassemblyWidget *DisassemblerWidget::parent() const
{
    return static_cast<DisassemblyWidget *>(QTableWidget::parent());
}

void DisassemblerWidget::toggleBreakpoint(int row)
{
    QTableWidgetItem *item = verticalHeaderItem(row);
    bool enabled = item->data(Qt::UserRole).toBool();

    if (enabled)
    {
        item->setIcon(QIcon(QStringLiteral(":/assets/icons/nobreakpoint.svg")));
    }
    else
    {
        item->setIcon(QIcon(QStringLiteral(":/assets/icons/breakpoint.svg")));
    }

    item->setData(Qt::UserRole, !enabled);
}

void DisassemblerWidget::setAddress(uint32_t addr)
{
    clearContents();

    mTopAddress = addr;

    addr = addr < 32 ? 0 : addr - 32;

    for (int i = 0; i < 128; i++)
    {
        uint32_t prevAddr = addr;
        QPair<QString, QString> dis = disassemble(addr);

        if (prevAddr >= mTopAddress)
        {
            int row = rowCount();
            insertRow(row);
            setItem(row, Column::Address, new QTableWidgetItem{Util::int2hex(prevAddr, Util::addrByteWidth)});
            setItem(row, Column::Data, new QTableWidgetItem{dis.second});
            setItem(row, Column::Mnemonic, new QTableWidgetItem{dis.first});
            item(row, Column::Address)->setFont(mBoldFont);
            auto item = new QTableWidgetItem(QIcon(QStringLiteral(":/assets/icons/nobreakpoint.svg")), QString());
            item->setData(Qt::UserRole, false);
            setVerticalHeaderItem(row, item);
        }
    }

    mBottomAddress = addr;
}

bool DisassemblerWidget::isAtTop()
{
    return mTopAddress == 0;
}

bool DisassemblerWidget::isAtBottom()
{
    return mBottomAddress >= 0xFFFFFF;
}

void DisassemblerWidget::append()
{
    uint32_t addr = mBottomAddress;
    QPair<QString, QString> dis = disassemble(addr);
    mBottomAddress = addr;

    int row = rowCount();
    insertRow(row);
    setItem(row, Column::Address, new QTableWidgetItem{Util::int2hex(addr, Util::addrByteWidth)});
    setItem(row, Column::Data, new QTableWidgetItem{dis.second});
    setItem(row, Column::Mnemonic, new QTableWidgetItem{dis.first});
    item(row, Column::Address)->setFont(mBoldFont);
    auto item = new QTableWidgetItem(QIcon(QStringLiteral(":/assets/icons/nobreakpoint.svg")), QString());
    item->setData(Qt::UserRole, false);
    setVerticalHeaderItem(row, item);}

void DisassemblerWidget::prepend()
{
    if (mTopAddress == 0)
    {
        return;
    }

    uint32_t addr = mBottomAddress;

    addr = mTopAddress;
    addr = addr < 32 ? 0 : addr - 32;

    for (int i = 0; i < 64; i++)
    {
        uint32_t prevAddr = addr;
        QPair<QString, QString> dis = disassemble(addr);

        if (addr >= mTopAddress)
        {
            uint32_t instSize = addr - prevAddr;

            int row = 0;
            insertRow(row);
            setItem(row, Column::Address, new QTableWidgetItem{Util::int2hex(prevAddr, Util::addrByteWidth)});
            setItem(row, Column::Data, new QTableWidgetItem{dis.second});
            setItem(row, Column::Mnemonic, new QTableWidgetItem{dis.first});
            item(row, Column::Address)->setFont(mBoldFont);
            auto item = new QTableWidgetItem(QIcon(QStringLiteral(":/assets/icons/nobreakpoint.svg")), QString());
            item->setData(Qt::UserRole, false);
            setVerticalHeaderItem(row, item);

            mTopAddress = instSize > mTopAddress ? 0 : mTopAddress - instSize;
            return;
        }
    }

    abort();
}

QPair<QString, QString> DisassemblerWidget::disassemble(uint32_t &addr)
{
    return mDis.disassemble(parent()->core(), addr);
}

void DisassemblerWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
        default:
            break;
        case Qt::Key_Up:
            if (currentRow() == 0)
            {
                prepend();
            }
            break;
        case Qt::Key_Down:
            if (currentRow() == rowCount() - 1)
            {
                append();
            }
            break;
        case Qt::Key_PageUp:
            if (currentRow() == 0)
            {
                for (int i = 0; i < verticalScrollBar()->pageStep(); ++i)
                {
                    prepend();
                }
            }
            break;
        case Qt::Key_PageDown:
            if (currentRow() == rowCount() - 1)
            {
                for (int i = 0; i < verticalScrollBar()->pageStep(); ++i)
                {
                    append();
                }
            }
            break;
    }

    QTableWidget::keyPressEvent(event);
}

void DisassemblerWidget::wheelEvent(QWheelEvent* event)
{
    QPoint delta = event->pixelDelta();
    QScrollBar *v = verticalScrollBar();

    if (delta.isNull())
    {
        delta = event->angleDelta() / 2;
    }

    if (delta.y())
    {
        int step = verticalHeader()->defaultSectionSize();

        if (delta.y() > 0)
        {
            if (v->value() == v->minimum())
            {
                int amount = (delta.y() + (step - 1)) / step;
                for (int i = 0; i < amount; ++i)
                {
                    prepend();
                }
            }
        }
        else
        {
            if (v->value() == v->maximum())
            {
                int amount = ((-delta.y()) + (step - 1)) / step;
                for (int i = 0; i < amount; ++i)
                {
                    append();
                }
            }
        }

        v->setValue(v->value() - delta.y());
    }

    event->accept();
}

void DisassemblerWidget::scrollAction(int action)
{
    QScrollBar *v = verticalScrollBar();

    v->blockSignals(true);

    switch (action)
    {
        case QAbstractSlider::SliderSingleStepAdd:
            if (v->sliderPosition() == v->maximum())
            {
                append();
                v->setSliderPosition(v->maximum());
            }
            break;
        case QAbstractSlider::SliderSingleStepSub:
            if (v->sliderPosition() == v->minimum())
            {
                prepend();
                v->setSliderPosition(v->minimum());
            }
            break;
        case QAbstractSlider::SliderPageStepAdd:
            if (v->sliderPosition() == v->maximum())
            {
                for (int i = 0; i < v->pageStep(); ++i)
                {
                    append();
                }
                v->setSliderPosition(v->maximum());
            }
            break;
        case QAbstractSlider::SliderPageStepSub:
            if (v->sliderPosition() == v->minimum())
            {
                for (int i = 0; i < v->pageStep(); ++i)
                {
                    prepend();
                }
                v->setSliderPosition(v->minimum());
            }
            break;
        case QAbstractSlider::SliderMove:
            qDebug() << "move";
            break;
    }

    v->blockSignals(false);
}

void DisassemblerWidgetDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    option->state &= ~QStyle::State_HasFocus;
}

void DisassemblerWidgetDelegate::paint(QPainter* painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() != 2)
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem options = option;
    initStyleOption(&options, index);

    painter->save();

    QString highlighted = options.text
        .replace(QRegularExpression(QStringLiteral("(\\$[0-9a-fA-F]+)")), QStringLiteral("<font color='green'>\\1</font>"))               // hex numbers
        .replace(QRegularExpression(QStringLiteral("(^\\d)")), QStringLiteral("<font color='blue'>\\1</font>"))                           // dec number
        .replace(QRegularExpression(QStringLiteral("([()])")), QStringLiteral("<font color='brown'>\\1</font>"))                          // parentheses
        .replace(QRegularExpression(QStringLiteral("(?:^|(?:[.!?])\\s)([^\\s]+)")), QStringLiteral("<font color='darkblue'>\\1</font>")); // opcode

    QTextDocument doc;
    doc.setTextWidth(options.rect.width());
    doc.setDefaultFont(Util::monospaceFont());
    doc.setHtml(highlighted);

    options.text = "";
    options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter);

    painter->translate(options.rect.left(), options.rect.top());
    QRect clip(0, 0, options.rect.width(), options.rect.height());
    doc.drawContents(painter, clip);

    painter->restore();
}
