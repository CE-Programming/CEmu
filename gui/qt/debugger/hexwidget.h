#ifndef HEXWIDGET_H
#define HEXWIDGET_H

#include <QtCore/QPoint>
#include <QtCore/QStack>
#include <QtWidgets/QWidget>
#include <QtWidgets/QAbstractScrollArea>

class HexWidget : public QAbstractScrollArea {
    Q_OBJECT

public:
    explicit HexWidget(QWidget *parent = Q_NULLPTR);
    virtual ~HexWidget() { }

    void setBase(int address) { m_base = address; adjust(); }
    int getBase() { return m_base; }

    void setBytesPerLine(int bytes) { m_bytesPerLine = bytes; adjust(); }
    int bytesPerLine() { return m_bytesPerLine; }

    void setAsciiArea(bool area) { m_asciiArea = area; adjust(); }
    bool asciiArea() { return m_asciiArea; }

    void setScrollable(bool state) { m_scrollable = state; adjust(); }
    bool scrollable() { return m_scrollable; }
    void scroll(int value);

    void setOffset(int addr);
    int getOffset() { return m_cursorOffset / 2; }

    void setCursorOffset(int address, bool selection = true);
    int getCursorOffset() { return m_cursorOffset; }

    void setData(const QByteArray &ba);
    void prependData(const QByteArray &ba);
    void appendData(const QByteArray &ba);

    const char *data() { return m_data.constData(); }
    const char *modified() { return m_modified.constData(); }
    int modifiedCount() { return m_modified.count(); }

    int size() { return m_size; }

    int indexNotOf(const QByteArray &ba);
    int indexPrevOf(const QByteArray &ba);
    int indexPrevNotOf(const QByteArray &ba);
    int indexOf(const QByteArray &ba);

    void setFont(const QFont &font) { QAbstractScrollArea::setFont(font); adjust(); }

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
    void adjustScroll(int value);

private:
    void redo();
    void undo();
    void cursorScroll();
    void setSelection(int addr);
    void resetSelection() { m_selectStart = m_selectEnd = -1; }
    bool isSelected() { return m_selectStart != -1; }
    void setSelected(char n) { overwrite(m_selectStart * 2, m_selectLen, QByteArray(m_selectLen, n)); }
    void overwrite(int pos, char c);
    void overwrite(int pos, int len, const QByteArray &ba);
    int getPosition(QPoint posa);

    typedef struct {
        int addr;
        QByteArray ba;
    } stack_entry_t;

    int m_bytesPerLine = 8;
    int m_base = 0;
    bool m_asciiArea = true;

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

    QStack<stack_entry_t> m_stack;
};

#endif
