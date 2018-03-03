#include "hexwidget.h"
#include "../../core/debug/debug.h"
#include "../../core/mem.h"
#include "../utils.h"

#include <QtGui/QWheelEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QScrollBar>

HexWidget::HexWidget(QWidget *parent) : QAbstractScrollArea{parent}, m_data{0} {
#ifdef Q_OS_WIN
    setFont(QFont(QStringLiteral("Courier"), 10));
#else
    setFont(QFont(QStringLiteral("Monospace"), 10));
#endif

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &HexWidget::adjust);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &HexWidget::adjust);

    adjust();
}

void HexWidget::setData(const QByteArray &ba) {
    m_data = ba;
    adjust();
}

void HexWidget::prependData(const QByteArray &ba) {
    m_data.prepend(ba);
    adjust();
}

void HexWidget::appendData(const QByteArray &ba) {
    m_data.append(ba);
    adjust();
}

int HexWidget::indexPrevOf(const QByteArray &ba) {
    int res = -1;
    QByteArray buffer{m_data.mid(0, m_addrSelected - 1)};
    if (int found = buffer.lastIndexOf(ba) >= 0) {
        res = found;
    }
    return res;
}

int HexWidget::indexPrevNotOf(const QByteArray &ba) {
    int res = -1;
    QByteArray buffer{m_data.mid(0, m_addrSelected - 1)};
    if (int found = buffer.toStdString().find_last_not_of(ba.toStdString()) != std::string::npos) {
        res = found;
    }
    return res;
}

int HexWidget::indexOf(const QByteArray &ba) {
    int res = -1;
    QByteArray buffer{m_data.mid(m_addrSelected, m_addrEnd)};
    if (int found = buffer.indexOf(ba) >= 0) {
        res = found;
    }
    return res;
}

int HexWidget::indexNotOf(const QByteArray &ba) {
    int res = -1;
    QByteArray buffer{m_data.mid(m_addrSelected, m_addrEnd)};
    if (int found = buffer.toStdString().find_first_not_of(ba.toStdString()) != std::string::npos) {
        res = found;
    }
    return res;
}

void HexWidget::adjust() {
    m_size = m_data.size();
    m_addrEnd = m_size - 1;
    m_charWidth = fontMetrics().width(QLatin1Char('D'));
    m_charHeight = fontMetrics().height();
    m_selectPart = m_charHeight / 5;
    m_marginGap = m_charWidth / 2;
    m_addrLoc = m_marginGap;

    m_dataLine = m_addrLoc + (6 * m_charWidth) + m_charWidth;
    m_dataLoc = m_dataLine + m_charWidth;

    int xWidth = m_dataLoc + (m_bytesPerLine * 3 * m_charWidth);
    if (m_asciiArea) {
        m_asciiLine = xWidth + m_marginGap;
        m_asciiLoc = m_asciiLine + m_marginGap;
        xWidth = m_asciiLoc + (m_bytesPerLine * m_charWidth) + m_marginGap;
    }
    horizontalScrollBar()->setRange(0, xWidth - viewport()->width());
    horizontalScrollBar()->setPageStep(viewport()->width());

    int rows = m_size / m_bytesPerLine;
    m_visibleRows = (viewport()->height() - 4) / m_charHeight;
    verticalScrollBar()->setRange(0, rows - m_visibleRows);
    verticalScrollBar()->setPageStep(m_visibleRows);

    int lines = verticalScrollBar()->value();
    m_lineAddrStart = lines * m_bytesPerLine;
    m_lineAddrEnd = m_lineAddrStart + m_visibleRows * m_bytesPerLine - 1;
    if (m_lineAddrEnd >= m_size) {
        m_lineAddrEnd = m_addrEnd;
    }
    repaint();
    viewport()->repaint();
}

void HexWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(viewport());
    const QPalette *pal = &viewport()->palette();
    int xOffset = horizontalScrollBar()->value();
    int xAddr = m_addrLoc - xOffset;
    QColor cText = pal->color(QPalette::WindowText);
    QString hex;

    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(event->rect(), Qt::white);

    painter.setPen(Qt::gray);
    painter.drawLine(m_dataLine - xOffset, event->rect().top(), m_dataLine, height());
    if (m_asciiArea) {
        painter.drawLine(m_asciiLine - xOffset, event->rect().top(), m_asciiLine - xOffset, height());
    }

    painter.setPen(cText);

    for (int row = 0, y = m_charHeight; row <= m_visibleRows; row++, y += m_charHeight) {
        int xData = m_dataLoc - xOffset;
        int xAscii = m_asciiLoc - xOffset;
        int lineAddr = m_lineAddrStart + row * m_bytesPerLine;
        int addr = lineAddr;
        painter.drawText(xAddr, y, int2hex(m_baseAddr + lineAddr, 6));
        for (int col = 0; col < m_bytesPerLine && addr < m_addrEnd; col++) {
            addr = lineAddr + col;

            painter.setPen(cText);
            uint8_t data = m_data.at(addr);
            uint8_t dbg = debugger.data.block[addr];

            if (dbg & DBG_MASK_READ) {
                painter.setPen(QColor(0xA3FFA3));
            }
            if (dbg & DBG_MASK_WRITE) {
                painter.setPen(QColor(0xA3A3FF));
            }
            if (dbg & DBG_MASK_EXEC) {
                painter.setPen(QColor(0xFFA3A3));
            }

            hex = int2hex(data, 2);
            if ((dbg & DBG_MASK_READ) && (dbg & DBG_MASK_WRITE)) {
                painter.setPen(QColor(0xA3FFA3));
                painter.drawText(xData, y, hex.at(0));
                xData += m_charWidth;
                painter.setPen(QColor(0xA3A3FF));
                painter.drawText(xData, y, hex.at(1));
                xData += 2 * m_charWidth;
            } else {
                painter.drawText(xData, y, hex);
                xData += 3 * m_charWidth;
            }

            if (m_asciiArea) {
                char ch = static_cast<char>(data);
                if (ch < 0x20 || ch > 0x7e) {
                    ch = '.';
                }
                painter.drawText(xAscii, y, QChar(ch));
                xAscii += m_charWidth;
            }
        }
    }
}

void HexWidget::resizeEvent(QResizeEvent *event) {
    adjust();
    event->accept();
}

void HexWidget::focusInEvent(QFocusEvent *event) {
    emit focused();
    event->accept();
}

