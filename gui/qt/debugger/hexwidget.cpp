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
    QByteArray buffer{m_data.mid(0, m_addrCursor - 1)};
    int found = buffer.lastIndexOf(ba);
    if (found >= 0) {
        setAddr(res = found);
    }
    return res;
}

int HexWidget::indexPrevNotOf(const QByteArray &ba) {
    int res = -1;
    QByteArray buffer{m_data.mid(0, m_addrCursor - 1)};
    std::size_t found = buffer.toStdString().find_last_not_of(ba.toStdString());
    if (found != std::string::npos) {
        setAddr(res = static_cast<int>(found));
    }
    return res;
}

int HexWidget::indexOf(const QByteArray &ba) {
    int res = -1;
    QByteArray buffer{m_data.mid(m_addrCursor, m_addrEnd)};
    int found = buffer.indexOf(ba);
    if (found >= 0) {
        setAddr(res = found);
    }
    return res;
}

int HexWidget::indexNotOf(const QByteArray &ba) {
    int res = -1;
    QByteArray buffer{m_data.mid(m_addrCursor, m_addrEnd)};
    std::size_t found = buffer.toStdString().find_first_not_of(ba.toStdString());
    if (found != std::string::npos) {
        setAddr(res = static_cast<int>(found));
    }
    return res;
}

void HexWidget::setAddr(int addr) {
    setCursorAddr(addr * 2);
}

void HexWidget::setCursorAddr(int addr) {
    if (addr > m_addrEnd) {
        addr = m_addrEnd;
    }
    if (addr < 0) {
        addr = 0;
    }

    m_addrCursor = addr;
    adjust();
    cursorScroll();
}

int HexWidget::getCursorAddr(QPoint posa) {
    int xOffset = posa.x() + horizontalScrollBar()->value();
    int result = -1;

    if (xOffset >= m_dataLoc && xOffset < m_asciiLine) {
        int y = ((posa.y() - m_marginGap) / m_charHeight) * m_bytesPerLine * 2;
        int x = (xOffset - m_dataLoc) / m_charWidth;
        x = (x / 3) * 2 + x % 3;
        result = m_lineAddrStart * 2 + x + y;
    }

    return result;
}

void HexWidget::cursorScroll() {
    int addr = m_addrCursor / 2;
    if (addr <= m_lineAddrStart) {
        verticalScrollBar()->setValue(addr / m_bytesPerLine);
    }
    if (addr > (m_lineAddrStart + m_visibleRows * m_bytesPerLine - 1)) {
        verticalScrollBar()->setValue((addr / m_bytesPerLine) - m_visibleRows + 1);
    }
    if (m_cursor.x() < horizontalScrollBar()->value()) {
        horizontalScrollBar()->setValue(0);
    }
}

void HexWidget::adjust() {
    m_size = m_data.size();
    m_addrEnd = m_size - 1;
    m_charWidth = fontMetrics().width(QLatin1Char('D'));
    m_charHeight = fontMetrics().height();
    m_cursorHeight = m_charHeight / 7;
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
    m_visibleRows = (viewport()->height() - m_marginGap) / m_charHeight;
    verticalScrollBar()->setRange(0, rows - m_visibleRows);
    verticalScrollBar()->setPageStep(m_visibleRows);

    int lines = verticalScrollBar()->value();
    m_lineAddrStart = lines * m_bytesPerLine;
    m_lineAddrEnd = m_lineAddrStart + m_visibleRows * m_bytesPerLine - 1;
    if (m_lineAddrEnd >= m_size) {
        m_lineAddrEnd = m_addrEnd;
    }

    int y = (((m_addrCursor / 2) - m_lineAddrStart) / m_bytesPerLine + 1) * m_charHeight;
    int x = m_addrCursor % (m_bytesPerLine * 2);
    x = (((x / 2) * 3) + (x % 2)) * m_charWidth + m_dataLoc;
    m_cursor = QRect(x - horizontalScrollBar()->value(), y + m_cursorHeight, m_charWidth, m_cursorHeight);

    update();
    viewport()->update();
}

void HexWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(viewport());
    const QRect &region = event->rect();
    const QPalette &pal = viewport()->palette();
    const QColor cText = pal.color(QPalette::WindowText);
    const QColor cBg = Qt::white;
    const int xOffset = horizontalScrollBar()->value();
    const int xAddr = m_addrLoc - xOffset;

    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(region, cBg);

    if (m_data.size()) {
        painter.fillRect(m_cursor, cText);
    }

    painter.setPen(Qt::gray);
    painter.drawLine(m_dataLine - xOffset, region.top(), m_dataLine - xOffset, height());
    if (m_asciiArea) {
        painter.drawLine(m_asciiLine - xOffset, region.top(), m_asciiLine - xOffset, height());
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

            QString hex = int2hex(data, 2);
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

void HexWidget::mousePressEvent(QMouseEvent *event) {
    int addr = getCursorAddr(event->pos());
    if (addr >= 0) {
        setCursorAddr(addr);
    }
}

void HexWidget::keyPressEvent(QKeyEvent *event) {
    int addr = m_addrCursor;
    if (event->matches(QKeySequence::MoveToNextChar)) {
        addr += 1;
    }
    if (event->matches(QKeySequence::MoveToPreviousChar)) {
        addr -= 1;
    }
    if (event->matches(QKeySequence::MoveToEndOfLine)) {
        addr |= m_bytesPerLine * 2 - 1;
    }
    if (event->matches(QKeySequence::MoveToStartOfLine)) {
        addr -= m_addrCursor % (m_bytesPerLine * 2);
    }
    if (event->matches(QKeySequence::MoveToPreviousLine)) {
        addr -= m_bytesPerLine * 2;
    }
    if (event->matches(QKeySequence::MoveToNextLine)) {
        addr += m_bytesPerLine * 2;
    }
    if (event->matches(QKeySequence::MoveToPreviousPage)) {
        addr -= (m_visibleRows - 1) * m_bytesPerLine * 2;
    }
    if (event->matches(QKeySequence::MoveToNextPage)) {
        addr += (m_visibleRows - 1) * m_bytesPerLine * 2;
    }
    if (event->matches(QKeySequence::MoveToEndOfDocument)) {
        addr = m_size * 2;
    }
    if (event->matches(QKeySequence::MoveToStartOfDocument)){
        addr = 0;
    }
    setCursorAddr(addr);
}
