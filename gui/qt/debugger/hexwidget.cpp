#include "hexwidget.h"
#include "../../core/debug/debug.h"
#include "../../core/mem.h"
#include "../utils.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QScrollBar>
#include <QtGui/QPainter>
#include <QtGui/QClipboard>

HexWidget::HexWidget(QWidget *parent) : QAbstractScrollArea{parent}, m_data{Q_NULLPTR} {
#ifdef Q_OS_WIN
    setFont(QFont(QStringLiteral("Courier"), 10));
#else
    setFont(QFont(QStringLiteral("Monospace"), 10));
#endif

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &HexWidget::scroll);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &HexWidget::adjust);

    resetSelection();
    adjust();
}

void HexWidget::setData(const QByteArray &ba) {
    if (!isEnabled()) {
        return;
    }

    m_scrolled = false;
    m_data = ba;
    m_modified.resize(m_data.size());
    m_modified.fill(0);
    adjust();
}

void HexWidget::prependData(const QByteArray &ba) {
    m_data.prepend(ba);
    m_modified.prepend(QByteArray(ba.size(), 0));
    adjust();
}

void HexWidget::appendData(const QByteArray &ba) {
    m_data.append(ba);
    m_modified.append(QByteArray(ba.size(), 0));
    adjust();
}

void HexWidget::scroll(int value) {
    adjust();

    if (!m_scrollable || !isEnabled()) {
        return;
    }

    verticalScrollBar()->blockSignals(true);
    if (value <= verticalScrollBar()->minimum()) {
        int addr = m_base;
        QByteArray data;
        for (int i = -m_bytesPerLine; i < 0; i++) {
            if (addr + i >= 0) {
                data.append(static_cast<char>(mem_peek_byte(addr + i)));
            }
        }
        if (data.size()) {
            m_scrolled = true;
            m_base -= data.size();
            prependData(data);
            verticalScrollBar()->setValue(1);
        }
    }
    if (value >= verticalScrollBar()->maximum()) {
        int addr = m_maxOffset + m_base + 1;
        QByteArray data;
        for (int i = 0; i < m_bytesPerLine && (addr + i) < 0x1000000; i++) {
            data.append(mem_peek_byte(addr + i));
        }
        if (data.size()) {
            m_scrolled = true;
            appendData(data);
            verticalScrollBar()->setValue(verticalScrollBar()->maximum() - 1);
        }
    }
    verticalScrollBar()->blockSignals(false);
}

int HexWidget::indexPrevOf(const QByteArray &ba) {
    int res = m_data.mid(0, m_cursorOffset / 2).lastIndexOf(ba);
    if (res >= 0) {
        m_selectStart = res;
        m_selectLen = ba.size();
        m_selectEnd = m_selectStart + m_selectLen - 1;
        setCursorOffset(res * 2, false);
    }
    return res;
}

int HexWidget::indexPrevNotOf(const QByteArray &ba) {
    int res = -1;
    QByteArray buffer{m_data.mid(0, m_cursorOffset / 2 - 1)};
    std::size_t found = buffer.toStdString().find_last_not_of(ba.toStdString());
    if (found != std::string::npos) {
        setOffset(res = static_cast<int>(found));
    }
    return res;
}

int HexWidget::indexOf(const QByteArray &ba) {
    int res = m_data.indexOf(ba, m_cursorOffset / 2 + 1);
    if (res >= 0) {
        m_selectStart = res;
        m_selectLen = ba.size();
        m_selectEnd = m_selectStart + m_selectLen - 1;
        setCursorOffset(res * 2, false);
    }
    return res;
}

int HexWidget::indexNotOf(const QByteArray &ba) {
    int res = -1;
    QByteArray buffer{m_data.mid(m_cursorOffset, m_maxOffset)};
    std::size_t found = buffer.toStdString().find_first_not_of(ba.toStdString());
    if (found != std::string::npos) {
        setOffset(res = static_cast<int>(found));
    }
    return res;
}

void HexWidget::setOffset(int offset) {
    setCursorOffset(offset * 2);
}

void HexWidget::setCursorOffset(int offset, bool selection) {
    if (offset > m_size * 2) {
        offset = m_size * 2;
    }
    if (offset < 0) {
        offset = 0;
    }
    if (selection) {
        resetSelection();
    }

    m_cursorOffset = offset;
    adjust();
    showCursor();
}

int HexWidget::getPosition(QPoint posa, bool allow) {
    int xOffset = posa.x() + horizontalScrollBar()->value();
    int result = -1;

    if (xOffset >= m_asciiLoc && xOffset < (m_asciiLoc + m_bytesPerLine * m_charWidth)) {
        if (allow) {
            m_asciiEdit = true;
        }
        if (m_asciiEdit) {
            int y = ((posa.y() - m_gap) / m_charHeight) * m_bytesPerLine * 2;
            int x = (xOffset - m_asciiLoc) / m_charWidth;
            result = m_lineStart * 2 + x * 2 + y;
        }
    }

    if (xOffset >= m_dataLoc && xOffset < m_asciiLine) {
        if (allow) {
            m_asciiEdit = false;
        }
        if (!m_asciiEdit) {
            int y = ((posa.y() - m_gap) / m_charHeight) * m_bytesPerLine * 2;
            int x = (xOffset - m_dataLoc) / m_charWidth;
            x = (x / 3) * 2 + x % 3;
            result = m_lineStart * 2 + x + y;
        }
    }

    return result;
}

void HexWidget::showCursor() {
    disconnect(verticalScrollBar(), &QScrollBar::valueChanged, this, &HexWidget::scroll);
    int addr = m_cursorOffset / 2;
    if (addr <= m_bytesPerLine) {
        verticalScrollBar()->setValue(0);
    } else if (addr <= m_lineStart) {
        verticalScrollBar()->setValue(addr / m_bytesPerLine);
    }
    if (addr > (m_lineStart + m_visibleRows * m_bytesPerLine - 1)) {
        verticalScrollBar()->setValue((addr / m_bytesPerLine) - m_visibleRows + 1);
    }
    if (m_cursor.x() < horizontalScrollBar()->value()) {
        horizontalScrollBar()->setValue(0);
    }
    adjust();
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &HexWidget::scroll);
}

void HexWidget::adjust() {
    m_size = m_data.size();
    m_maxOffset = m_size - 1;

    m_charWidth = fontMetrics().horizontalAdvance(QLatin1Char('D'));
    m_charHeight = fontMetrics().height();
    m_cursorHeight = m_charHeight / 7;
    m_margin = m_charHeight / 5;
    m_gap = m_charWidth / 2;
    m_addrLoc = m_gap;

    m_dataLine = m_addrLoc + (6 * m_charWidth) + m_charWidth;
    m_dataLoc = m_dataLine + m_charWidth;

    int xWidth = m_dataLoc + (m_bytesPerLine * 3 * m_charWidth);
    m_asciiLine = xWidth + m_gap;
    m_asciiLoc = m_asciiLine + m_gap;
    if (m_asciiArea) {
        xWidth = m_asciiLoc + (m_bytesPerLine * m_charWidth) + m_gap;
    }
    horizontalScrollBar()->setRange(0, xWidth - viewport()->width());
    horizontalScrollBar()->setPageStep(viewport()->width());

    int rows = m_size / m_bytesPerLine;
    int visibleHeight = viewport()->height() - m_gap;
    if (horizontalScrollBar()->isVisible()) {
        visibleHeight -= horizontalScrollBar()->height();
    }
    m_visibleRows = visibleHeight / m_charHeight;
    verticalScrollBar()->setRange(0, rows - m_visibleRows);
    verticalScrollBar()->setPageStep(m_visibleRows);

    int lines = verticalScrollBar()->value();
    m_lineStart = lines * m_bytesPerLine;
    m_lineEnd = m_lineStart + m_visibleRows * m_bytesPerLine - 1;
    if (m_lineEnd >= m_size) {
        m_lineEnd = m_maxOffset;
    }

    int x, y = (((m_cursorOffset / 2) - m_lineStart) / m_bytesPerLine + 1) * m_charHeight;
    if (!m_asciiEdit) {
        y = (((m_cursorOffset / 2) - m_lineStart) / m_bytesPerLine + 1) * m_charHeight;
        x = m_cursorOffset % (m_bytesPerLine * 2);
        x = (((x / 2) * 3) + (x % 2)) * m_charWidth + m_dataLoc;
    } else {
        y = (((m_cursorOffset / 2) - m_lineStart) / m_bytesPerLine + 1) * m_charHeight;
        x = (m_cursorOffset % (m_bytesPerLine * 2)) / 2;
        x = x * m_charWidth + m_asciiLoc;
    }
    m_cursor = QRect(x - horizontalScrollBar()->value(), y + m_cursorHeight, m_charWidth, m_cursorHeight);

    update();
    viewport()->update();
}

void HexWidget::setSelection(int addr) {
    if ((addr /= 2) < 0) {
        addr = 0;
    }

    if (m_selectStart == -1) {
        m_selectStart = addr;
        m_selectEnd = addr;
        m_selectLen = 0;
    }
    if (addr > m_selectStart) {
        m_selectEnd = addr;
        m_selectLen = addr - m_selectStart + 1;
    } else {
        m_selectStart = addr;
        m_selectLen = m_selectEnd - addr + 1;
    }
}

void HexWidget::undo() {
    if (!m_stack.isEmpty()) {
        stack_entry_t entry = m_stack.pop();
        int address = entry.addr / 2;
        int len = entry.ba.size();
        m_data.replace(address, len, entry.ba);
        for (int i = address; i < address + len; i++) {
            m_modified[i] = m_modified[i] - 1;
        }
        setCursorOffset(entry.addr);
    }
}

void HexWidget::overwrite(int addr, char c) {
    int address = addr / 2;
    stack_entry_t entry{addr, m_data.mid(address, 1)};
    m_stack.push(entry);
    m_data[address] = c;
    m_modified[address] = m_modified[address] + 1;
}

void HexWidget::overwrite(int addr, const QByteArray &ba) {
    int address = addr / 2;
    stack_entry_t entry{addr, m_data.mid(address, ba.size())};
    int len = entry.ba.size();
    m_stack.push(std::move(entry));
    m_data.replace(address, len, QByteArray::fromRawData(ba.constData(), len));
    for (int i = address; i < address + len; i++) {
        m_modified[i] = m_modified[i] + 1;
    }
}

void HexWidget::paintEvent(QPaintEvent *event) {
    QRect r;
    QPainter painter(viewport());
    const QRect &region = event->rect();
    const QPalette &pal = viewport()->palette();
    const QColor &cText = pal.color(QPalette::WindowText);
    const QColor &cBg = pal.color(QPalette::Window);
    const QColor &cSelected = pal.color(QPalette::Highlight);
    const QColor cModified = QColor(Qt::blue).lighter(160);
    const QColor cBoth = QColor(Qt::green).lighter(160);
    const int xOffset = horizontalScrollBar()->value();
    const int xAddr = m_addrLoc - xOffset;

    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(region, cBg);

    painter.setPen(Qt::gray);
    painter.drawLine(m_dataLine - xOffset, region.top(), m_dataLine - xOffset, height());
    if (m_asciiArea) {
        painter.drawLine(m_asciiLine - xOffset, region.top(), m_asciiLine - xOffset, height());
    }

    for (int row = 0, y = m_charHeight; row <= m_visibleRows; row++, y += m_charHeight) {
        int xData = m_dataLoc - xOffset;
        int xAscii = m_asciiLoc - xOffset;
        int lineAddr = m_lineStart + row * m_bytesPerLine;
        int addr = lineAddr;
        if (addr + m_base > 0xffffff) { break; }
        painter.setPen(cText);
        painter.drawText(xAddr, y, int2hex(m_base + lineAddr, 6));
        for (int col = 0; col < m_bytesPerLine && addr < m_maxOffset; col++) {
            addr = lineAddr + col;

            painter.setPen(cText);
            uint8_t data = static_cast<uint8_t>(m_data[addr]);
            uint8_t flags = debug.addr[addr + m_base];
            bool selected = addr >= m_selectStart && addr <= m_selectEnd;
            bool modified = !m_modified.isEmpty() && m_modified[addr];

            QFont font = painter.font();
            const QFont fontorig = painter.font();

            if (flags & DBG_MASK_READ) {
                font.setWeight(QFont::DemiBold);
                painter.setFont(font);
                painter.setPen(Qt::darkGreen);
            }
            if (flags & DBG_MASK_WRITE) {
                font.setWeight(QFont::DemiBold);
                painter.setFont(font);
                painter.setPen(Qt::darkYellow);
            }
            if (flags & DBG_MASK_EXEC) {
                font.setWeight(QFont::DemiBold);
                painter.setFont(font);
                painter.setPen(Qt::darkRed);
            }

            if (modified || selected) {
                if (!col) {
                    r.setRect(xData, y - m_charHeight + m_margin, 2 * m_charWidth, m_charHeight);
                } else {
                    r.setRect(xData - m_charWidth, y - m_charHeight + m_margin, 3 * m_charWidth, m_charHeight);
                }
                painter.fillRect(r, modified ? selected ? cBoth : cModified : cSelected);
            }

            QString hex = int2hex(data, 2);
            if ((flags & DBG_MASK_READ) && (flags & DBG_MASK_WRITE)) {
                painter.setPen(Qt::darkGreen);
                painter.drawText(xData, y, hex.at(0));
                xData += m_charWidth;
                painter.setPen(Qt::darkYellow);
                painter.drawText(xData, y, hex.at(1));
                xData += 2 * m_charWidth;
            } else {
                painter.drawText(xData, y, hex);
                xData += 3 * m_charWidth;
            }

            painter.setFont(fontorig);

            if (m_asciiArea) {
                char ch = static_cast<char>(data);
                if (ch < 0x20 || ch > 0x7e) {
                    ch = '.';
                }
                if (modified || selected) {
                    r.setRect(xAscii, y - m_charHeight + m_margin, m_charWidth, m_charHeight);
                    painter.fillRect(r, modified ? selected ? cBoth : cModified : cSelected);
                }
                painter.drawText(xAscii, y, QChar(ch));
                xAscii += m_charWidth;
            }
        }
    }

    if (!isEnabled()) {
        m_stack.clear();
        m_modified.clear();
    }

    if (m_data.size()) {
        painter.fillRect(m_cursor, cText);
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
    int addr = getPosition(event->pos());
    if (addr >= 0) {
        setCursorOffset(addr, true);
    }
}

void HexWidget::mouseMoveEvent(QMouseEvent *event) {
    int addr = getPosition(event->pos(), false);
    if (addr >= 0) {
        setSelection(addr);
        setCursorOffset(addr, false);
    }
}

void HexWidget::keyPressEvent(QKeyEvent *event) {
    int addr = m_cursorOffset;
    if (event->matches(QKeySequence::MoveToNextChar)) {
        setCursorOffset(m_asciiEdit ? addr + 2 : addr + 1);
    } else
    if (event->matches(QKeySequence::MoveToPreviousChar)) {
        setCursorOffset(m_asciiEdit ? addr - 2 : addr - 1);
    } else
    if (event->matches(QKeySequence::MoveToEndOfLine)) {
        setCursorOffset(addr | (m_bytesPerLine * 2 - 1));
    } else
    if (event->matches(QKeySequence::MoveToStartOfLine)) {
        setCursorOffset(addr - (m_cursorOffset % (m_bytesPerLine * 2)));
    } else
    if (event->matches(QKeySequence::MoveToPreviousLine)) {
        setCursorOffset(addr - m_bytesPerLine * 2);
    } else
    if (event->matches(QKeySequence::MoveToNextLine)) {
        setCursorOffset(addr + m_bytesPerLine * 2);
    } else
    if (event->matches(QKeySequence::MoveToPreviousPage)) {
        setCursorOffset(addr - (m_visibleRows - 1) * m_bytesPerLine * 2);
    } else
    if (event->matches(QKeySequence::MoveToNextPage)) {
        setCursorOffset(addr + (m_visibleRows - 1) * m_bytesPerLine * 2);
    } else
    if (event->matches(QKeySequence::MoveToEndOfDocument)) {
        setCursorOffset(m_size * 2);
    } else
    if (event->matches(QKeySequence::MoveToStartOfDocument)){
        setCursorOffset(0);
    } else
    if (event->matches(QKeySequence::Copy) && isSelected()) {
        if (m_asciiEdit) {
            QByteArray ba = m_data.mid(m_selectStart, m_selectLen);
            QByteArray ascii;
            for (const char ch : ba) {
                ascii.append((ch < 0x20 || ch > 0x7e) ? '.' : ch);
            }
            ascii.append('\0');
            qApp->clipboard()->setText(ascii);
        } else {
            QByteArray ba = m_data.mid(m_selectStart, m_selectLen).toHex();
            qApp->clipboard()->setText(ba);
        }
    } else
    if (event->matches(QKeySequence::Paste)) {
        QByteArray ba = qApp->clipboard()->text().toLatin1();
        if (!m_asciiEdit) {
            ba = QByteArray::fromHex(ba);
        }
        overwrite(addr, ba);
        setCursorOffset(addr + ba.size() * 2);
    } else
    if (event->matches(QKeySequence::Delete)) {
        if (isSelected()) {
            setSelected(0);
        } else {
            overwrite(addr, 0);
        }
        setCursorOffset(addr + 2);
    } else
    if (event->matches(QKeySequence::Undo)) {
        undo();
    } else
    if (!(event->modifiers() & ~(Qt::ShiftModifier | Qt::KeypadModifier))) {
        int key = event->key();
        if (!m_asciiEdit) {
            if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'F')) {

                if (isSelected()) {
                    setSelected(0);
                }

                if (m_data.size() > 0) {
                    uint8_t value;
                    uint8_t num =  (key <= '9') ? (key - '0') : (key - 'A' + 10);
                    if (m_cursorOffset % 2) {
                        value = (m_data[addr / 2] & 0xf0) | num;
                    } else {
                        value = (m_data[addr / 2] & 0x0f) | (num << 4);
                    }
                    overwrite(addr, value);
                    setCursorOffset(addr + 1);
                }
            }
        } else {
            if (isSelected()) {
                setSelected(0);
            }

            if (m_data.size() > 0) {
                overwrite(addr, static_cast<char>(key));
                setCursorOffset(addr + 2);
            }
        }
    }
}
