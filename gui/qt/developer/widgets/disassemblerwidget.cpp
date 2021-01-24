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
#include "../../corewindow.h"
#include "../disassemblywidget.h"
#include "../../util.h"
#include "../watchpointswidget.h"

#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QTextEdit>

DisassemblerWidget::DisassemblerWidget(DisassemblyWidget *parent)
    : QTableWidget{parent}
{
    mDelegate = new DisassemblerWidgetDelegate{this};

    setColumnCount(Column::Count);
    setHorizontalHeaderItem(Column::Address, new QTableWidgetItem{tr("Address")});
    setHorizontalHeaderItem(Column::Data, new QTableWidgetItem{tr("Data")});
    setHorizontalHeaderItem(Column::Mnemonic, new QTableWidgetItem{tr("Disassembly")});

    setItemDelegate(mDelegate);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setVisible(true);
    verticalHeader()->setVisible(false);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setFont(Util::monospaceFont());
    setShowGrid(false);

    verticalHeader()->setMinimumSectionSize(fontMetrics().ascent());
    verticalHeader()->setDefaultSectionSize(fontMetrics().ascent());
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    mDelegate->setOptionWidth(verticalHeader()->defaultSectionSize());

    connect(verticalScrollBar(), &QScrollBar::actionTriggered, this, &DisassemblerWidget::scrollAction);
}

DisassemblyWidget *DisassemblerWidget::parent() const
{
    return static_cast<DisassemblyWidget *>(QTableWidget::parent());
}

void DisassemblerWidget::mousePressEvent(QMouseEvent *event)
{
    QTableWidgetItem *item = itemAt(event->pos());

    if (item && item->column() == Column::Address &&
        event->x() >= 0 && event->x() <= verticalHeader()->defaultSectionSize())
    {
        Watchpoint watchpoint;

        watchpoint.addr = Util::hex2int(item->text());
        watchpoint.mode = Watchpoint::Mode::X;
        watchpoint.name = item->text();
        watchpoint.size = 1;

        emit toggleWatchpoint(watchpoint);

        // temporary
        parent()->core().set(cemucore::CEMUCORE_PROP_MEM_DBG_FLAGS, watchpoint.addr, cemucore::CEMUCORE_DBG_WATCH_EXEC);
    }

    QTableWidget::mousePressEvent(event);
}

void DisassemblerWidget::insertDisasmRow(int row,
                                         uint32_t addr,
                                         const QString &data,
                                         const QString &mnemonic)
{
    insertRow(row);
    setItem(row, Column::Address, new QTableWidgetItem{Util::int2hex(addr, Util::addrByteWidth)});
    setItem(row, Column::Data, new QTableWidgetItem{data});
    setItem(row, Column::Mnemonic, new QTableWidgetItem{mnemonic});

    if ((addr <= mPcAddr && mPcAddr <= ((addr + (data.length() / 2)) - 1)))
    {
        QBrush highlight({235, 235, 200});

        item(row, Column::Address)->setBackground(highlight);
        item(row, Column::Data)->setBackground(highlight);
        item(row, Column::Mnemonic)->setBackground(highlight);
        item(row, Column::Address)->setData(Qt::UserRole, 1);
    }
}

bool DisassemblerWidget::gotoAddress(const QString &addrStr)
{
    setRowCount(0);

    // todo: lookup the string in equates map
    uint32_t addr = Util::hex2int(addrStr);

    mPcAddr = parent()->core().get(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_PC);

    mTopAddress = addr;

    addr = addr < 32 ? 0 : addr - 32;

    int height = size().height();
    int amount = ((height / verticalHeader()->defaultSectionSize()) * 1.5) + 32;
    for (int i = 0; i < amount; i++)
    {
        uint32_t prevAddr = addr;
        QPair<QString, QString> dis = disassemble(addr);

        if (prevAddr >= mTopAddress)
        {
            insertDisasmRow(rowCount(), prevAddr, dis.second, dis.first);
        }
    }

    mBottomAddress = addr;

    selectRow(0);

    // center so we can see a little more information
    amount = ((height / verticalHeader()->defaultSectionSize()) * 0.5);
    for (int i = 0; i < amount; i++)
    {
        prepend();
    }

    return true;
}

bool DisassemblerWidget::isAtTop()
{
    return mTopAddress == 0;
}

bool DisassemblerWidget::isAtBottom()
{
    return mBottomAddress >= 0xFFFFFF;
}

void DisassemblerWidget::setAdl(bool enable)
{
    mDis.setAdl(enable);
}

void DisassemblerWidget::append()
{
    uint32_t addr = mBottomAddress;
    QPair<QString, QString> dis = disassemble(addr);

    insertDisasmRow(rowCount(), mBottomAddress, dis.second, dis.first);

    mBottomAddress = addr;
}

void DisassemblerWidget::prepend()
{
    if (mTopAddress == 0)
    {
        return;
    }

    if (mTopAddress == 1)
    {
        uint32_t addr = 0;
        QPair<QString, QString> dis = disassemble(addr);
        insertDisasmRow(0, 0, dis.second, dis.first);
        mTopAddress = 0;
        return;
    }

    uint32_t addr = mTopAddress;
    addr = addr < 32 ? 0 : addr - 32;

    for (int i = 0; i < 64; i++)
    {
        uint32_t prevAddr = addr;
        QPair<QString, QString> dis = disassemble(addr);

        if (addr >= mTopAddress)
        {
            uint32_t instSize = addr - prevAddr;

            insertDisasmRow(0, prevAddr, dis.second, dis.first);

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

int DisassemblerWidget::selectedAddress()
{
    return Util::hex2int(item(currentRow(), Column::Address)->text());
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
        delta = event->angleDelta() / 4;
    }

    if (delta.y())
    {
        if (delta.y() > 0 && v->value() == v->minimum())
        {
            for (int i = 0; i < delta.y(); ++i)
            {
                prepend();
            }
        }
        else if (v->value() == v->maximum())
        {
            for (int i = 0; i < -delta.y(); ++i)
            {
                append();
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
            break;
    }

    v->blockSignals(false);
}

void DisassemblerWidgetDelegate::setOptionWidth(int value)
{
    mWidth = value;
}

void DisassemblerWidgetDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());

    QStyledItemDelegate::initStyleOption(option, index);
    option->state &= ~QStyle::State_HasFocus;
}

QWidget *DisassemblerWidgetDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const

{
    Q_UNUSED(option);
    Q_ASSERT(index.isValid());

    QLineEdit *editor = new QLineEdit{parent};
    editor->setReadOnly(true);

    return editor;
}

void DisassemblerWidgetDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());

    Q_UNUSED(editor);
    Q_UNUSED(model);
    Q_UNUSED(index);
}

void DisassemblerWidgetDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const
{
    Q_ASSERT(index.isValid());

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const QWidget *widget = option.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();

    QFontMetrics metrics{QFontMetrics(Util::monospaceFont())};

    opt.text.clear();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

    painter->save();

    QString string = index.data().toString();

    if (index.column() == DisassemblerWidget::Mnemonic)
    {
        painter->setPen(Qt::blue);
    }
    else
    {
        painter->setPen(opt.palette.text().color());
    }

    const DisassemblerWidget *disasm = static_cast<const DisassemblerWidget *>(widget);
    int addr = disasm->item(index.row(), DisassemblerWidget::Column::Address)->text().toUInt(nullptr, 16);
    int flags = disasm->parent()->core().get(cemucore::CEMUCORE_PROP_MEM_DBG_FLAGS, addr);

    if (index.column() == DisassemblerWidget::Address)
    {
        QRect rect{option.rect};
        rect.setWidth(mWidth);

        painter->fillRect(rect, option.palette.window().color());
        opt.rect.moveRight(opt.rect.right() + rect.width());

        if (flags & cemucore::CEMUCORE_DBG_WATCH_EXEC)
        {
            painter->drawPixmap(rect, QPixmap(QStringLiteral(":/assets/icons/breakpoint.svg")));
        }
    }

    if (!(option.state & QStyle::State_Selected))
    {
        if (flags & cemucore::CEMUCORE_DBG_WATCH_READ)
        {
            painter->fillRect(opt.rect, {200, 255, 235});
        }
        if (flags & cemucore::CEMUCORE_DBG_WATCH_WRITE)
        {
            painter->fillRect(opt.rect, {200, 235, 255});
        }
    }
    else
    {
        painter->setPen(opt.palette.highlightedText().color());
    }

    opt.rect.moveRight(opt.rect.right() + metrics.horizontalAdvance(' ') / 2);

    for (int i = 0, t = string.count(); i < t; ++i)
    {
        bool reset = false;

        if (index.column() == DisassemblerWidget::Mnemonic && !(option.state & QStyle::State_Selected))
        {
            if (string.at(i) == QLatin1Char{'.'})
            {
                painter->setPen(Qt::magenta);
            }
            else if (string.at(i) == QLatin1Char{'$'} || string.at(i).isDigit())
            {
                painter->setPen(Qt::darkGreen);
            }
            else if (string.at(i) == QLatin1Char{'('} || string.at(i) == QLatin1Char{')'})
            {
                painter->setPen(Qt::darkMagenta);
                reset = true;
            }
            else if (string.at(i) == QLatin1Char{','} || string.at(i) == QLatin1Char{' '} ||
                    (string.at(i) == QLatin1Char{'+'} || string.at(i) == QLatin1Char{'-'}))
            {
                painter->setPen(opt.palette.text().color());
            }
        }

        style->drawItemText(painter, opt.rect, opt.displayAlignment, opt.palette, true, string.at(i));
        opt.rect.moveRight(opt.rect.right() + metrics.horizontalAdvance(string.at(i)));

        if (reset)
        {
            painter->setPen(opt.palette.text().color());
        }
    }

    painter->restore();
}
