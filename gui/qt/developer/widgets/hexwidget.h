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

#ifndef HEXWIDGET_H
#define HEXWIDGET_H

class MemWidget;

#include <QtCore/QPoint>
#include <QtCore/QStack>
#include <QtWidgets/QWidget>
#include <QtWidgets/QAbstractScrollArea>

class HexWidget : public QAbstractScrollArea
{
    Q_OBJECT

public:
    explicit HexWidget(MemWidget *parent);
    MemWidget *parent() const;

    void setData(const QByteArray &ba);
    void setBase(int address) { m_base = address; adjust(); }
    void setBytesPerLine(int bytes) { m_bytesPerLine = bytes; adjust(); }
    void setAsciiArea(bool area) { m_asciiArea = area; adjust(); }
    void setCursorOffset(int address, bool selection = true);
    void setScrollable(bool state) { m_scrollable = state; adjust(); }
    void setFont(const QFont &font) { QAbstractScrollArea::setFont(font); adjust(); }
    void setOffset(int addr);
    void prependData(const QByteArray &ba);
    void appendData(const QByteArray &ba);
    int getBase() { return m_base; }
    int getOffset() { return m_cursorOffset / 2; }
    int getCursorOffset() { return m_cursorOffset; }
    bool getAsciiArea() { return m_asciiArea; }
    bool getScrolled() { return m_scrolled; }
    int getSize() { return m_size; }
    int modifiedCount() { return m_modified.count(); }
    int indexNotOf(const QByteArray &ba);
    int indexPrevOf(const QByteArray &ba);
    int indexPrevNotOf(const QByteArray &ba);
    int indexOf(const QByteArray &ba);
    const char *data() { return m_data.constData(); }
    const char *modified() { return m_modified.constData(); }

protected:
    virtual void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    virtual void focusInEvent(QFocusEvent *) Q_DECL_OVERRIDE;
    virtual void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    virtual void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent *) Q_DECL_OVERRIDE;

signals:
    void focused();

private slots:
    void adjust();
    void scroll(int value);

private:
    void redo();
    void undo();
    void showCursor();
    void setSelection(int addr);
    void resetSelection() { m_selectStart = m_selectEnd = -1; }
    bool isSelected() { return m_selectStart != -1; }
    void setSelected(char n) { overwrite(m_selectStart * 2, m_selectLen, QByteArray(m_selectLen, n)); }
    void overwrite(int pos, char c);
    void overwrite(int pos, int len, const QByteArray &ba);
    int getPosition(QPoint posa, bool allow = true);

    typedef struct {
        int addr;
        QByteArray ba;
    } stack_entry_t;

    int m_bytesPerLine = 8;
    int m_base = 0;

    int m_charWidth;
    int m_charHeight;
    int m_margin;
    int m_gap;
    int m_addrLoc;
    int m_dataLine;
    int m_dataLoc;
    int m_asciiLine;
    int m_asciiLoc;

    int m_visibleRows;

    int m_lineStart;
    int m_lineEnd;

    QByteArray m_data;
    QByteArray m_modified;
    int m_size;
    int m_maxOffset;

    QRect m_cursor;
    int m_cursorOffset = 0;
    int m_cursorHeight;

    int m_selectStart;
    int m_selectEnd;
    int m_selectLen;

    bool m_scrollable = false;          // fetch bytes from memory on scroll
    bool m_asciiArea = true;            // show character representations
    bool m_scrolled = false;            // scrolled while focused
    bool m_asciiEdit = false;           // editing from the ascii side

    QStack<stack_entry_t> m_stack;
};

#endif
