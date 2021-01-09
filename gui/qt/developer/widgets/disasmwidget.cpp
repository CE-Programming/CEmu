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

#include "disasmwidget.h"

#include "../../corewrapper.h"

#include <QtGui/QWheelEvent>
#include <QtGui/QPainter>
#include <QtCore/QDebug>

DisasmWidget::DisasmWidget(QWidget *parent)
    : QWidget{parent}
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_labels[0x00] = QStringLiteral("Rst00");
    m_labels[0x08] = QStringLiteral("Rst08");
    m_labels[0x10] = QStringLiteral("Rst10");
    m_labels[0x18] = QStringLiteral("Rst18");
    m_labels[0x20] = QStringLiteral("Rst20");
    m_labels[0x28] = QStringLiteral("Rst28");
    m_labels[0x30] = QStringLiteral("Rst30");
    m_labels[0x38] = QStringLiteral("Rst38");
    m_baseAddr = next(m_baseAddr);
}

QSize DisasmWidget::sizeHint() const
{
    return {800, 100};
}

void DisasmWidget::wheelEvent(QWheelEvent *event)
{
    QPoint delta = event->pixelDelta();
    if (delta.isNull()) {
        delta = event->angleDelta() / 2;
    }
    if (delta.y()) {
        if (m_scroll >= delta.y()) {
            m_scroll -= delta.y();
        } else {
            m_scroll = 0;
        }
        repaint();
    }
    event->accept();
}

auto DisasmWidget::next(addr_t addr) -> addr_t {
    switch (addr.type) {
        case Label:
            addr.type = Inst;
            break;
        case Inst:
            //disasm.baseAddress = addr.addr;
            //disassembleInstruction();
            //addr.addr = disasm.newAddress;
        case None:
            addr.type = m_labels.contains(addr.addr) ? Label : Inst;
            break;
    }
    return addr;
}

void DisasmWidget::paintEvent(QPaintEvent *)
{
    const int lineHeight = fontMetrics().height();
    QPainter painter{this};
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::white);
    addr_t addr = m_baseAddr;

    for (QRect lineRect{0, -m_scroll, width(), lineHeight}; lineRect.y() < height(); lineRect.translate(0, lineHeight))
    {
        addr_t nextAddr = next(addr);
        if (lineRect.bottom() >= 0)
        {
            switch (addr.type)
            {
                case None:
                    return;
                case Label:
                    painter.drawText(lineRect.translated(100, 0), Qt::TextSingleLine, m_labels[addr.addr] + ':');
                    break;
                case Inst: {
                    uint32_t data = 0, size = nextAddr.addr - addr.addr;
                    for (uint32_t i = 0; i != size; i++)
                        data = data << 8 | cemucore::mem_peek_byte(addr.addr + i);
                    /*painter.drawText(lineRect, Qt::TextSingleLine, QStringLiteral("%1 %2")
                                     .arg(addr.addr, 6, 16, QChar('0'))
                                     .arg(data, size << 1, 16, QChar('0'))
                                     .toUpper().leftJustified(20) +
                                     QString::fromStdString(disasm.instruction.opcode) +
                                     QString::fromStdString(disasm.instruction.modeSuffix) +
                                     QString::fromStdString(disasm.instruction.arguments));*/
                    break;
                }
            }
        }
        addr = nextAddr;
    }
}
