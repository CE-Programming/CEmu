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
#include "../watchpointswidget.h"

#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMenu>
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
    setHorizontalHeaderItem(Column::Mnemonic, new QTableWidgetItem{tr("Disassembly")});

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
    verticalHeader()->setFixedWidth(fontMetrics().maxWidth() * 5);

    setShowGrid(false);
    QPalette p(palette());
    p.setColor(QPalette::Highlight, QColor(Qt::yellow).lighter());
    setPalette(p);

    connect(verticalScrollBar(), &QScrollBar::actionTriggered, this, &DisassemblerWidget::scrollAction);
    connect(verticalHeader(), &QHeaderView::sectionClicked, this, &DisassemblerWidget::toggleBreakpoint);
}

DisassemblyWidget *DisassemblerWidget::parent() const
{
    return static_cast<DisassemblyWidget *>(QTableWidget::parent());
}

void DisassemblerWidget::toggleBreakpoint(int row)
{
    const QString toggleX = tr("Toggle execute watchpoint (x)");
    const QString toggleR = tr("Toggle read watchpoint (r)");
    const QString toggleW = tr("Toggle write watchpoint (w)");

    QMenu menu;
    menu.addAction(toggleX);
    menu.addAction(toggleR);
    menu.addAction(toggleW);

    int y = rowViewportPosition(row) + verticalHeader()->defaultSectionSize() + 1;
    QAction *action = menu.exec(mapToGlobal({0, y}));
    if (action)
    {
        QString flags;
        int mode = verticalHeaderItem(row)->data(Qt::UserRole).toInt();

        if (action->text() == toggleX)
        {
            mode ^= Watchpoint::Mode::X;
        }
        else if (action->text() == toggleR)
        {
            mode ^= Watchpoint::Mode::R;
        }
        else if (action->text() == toggleW)
        {
            mode ^= Watchpoint::Mode::W;
        }
        flags += mode & Watchpoint::Mode::R ? QString{'r'} : QString{' '};
        flags += mode & Watchpoint::Mode::W ? QString{'w'} : QString{' '};
        flags += mode & Watchpoint::Mode::X ? QString{'x'} : QString{' '};

        verticalHeaderItem(row)->setData(Qt::UserRole, mode);
        verticalHeaderItem(row)->setText(flags);
    }
}

void DisassemblerWidget::insertDisasmRow(int row,
                                         uint32_t addr,
                                         const QString &data,
                                         const QString &mnemonic)
{
    uint32_t pcAddr = 0x200;//parent()->core().get(cemucore::CEMUCORE_PROP_REG, cemucore::CEMUCORE_REG_PC);

    insertRow(row);
    setItem(row, Column::Address, new QTableWidgetItem{Util::int2hex(addr, Util::addrByteWidth)});
    setItem(row, Column::Data, new QTableWidgetItem{data});
    setItem(row, Column::Mnemonic, new QTableWidgetItem{mnemonic});
    item(row, Column::Address)->setFont(mBoldFont);
    setVerticalHeaderItem(row, new QTableWidgetItem);
    verticalHeaderItem(row)->setTextAlignment(Qt::AlignCenter);
    verticalHeaderItem(row)->setData(Qt::UserRole, 0);

    if ((addr <= pcAddr && pcAddr <= ((addr + (data.length() / 2)) - 1)))
    {
        QBrush highlight({200, 235, 255});

        item(row, Column::Address)->setBackground(highlight);
        item(row, Column::Data)->setBackground(highlight);
        item(row, Column::Mnemonic)->setBackground(highlight);
    }
}

bool DisassemblerWidget::gotoAddress(const QString &addrStr)
{
    setRowCount(0);

    // todo: lookup the string in equates map
    uint32_t addr = Util::hex2int(addrStr);

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
