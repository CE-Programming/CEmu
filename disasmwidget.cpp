#include <QtWidgets>

#include "disasmwidget.h"
#include "core/mem.h"
#include "core/debug/debug.h"
#include "core/debug/disasm.h"

DisasmWidget::DisasmWidget(QWidget *p) : QPlainTextEdit(p) {
    addressArea = new LineNumberArea(this);

    connect(this, &QPlainTextEdit::updateRequest, this, &DisasmWidget::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &DisasmWidget::highlightCurrentLine);

    updateLineNumberAreaWidth();
    highlightCurrentLine();
}

int DisasmWidget::lineNumberAreaWidth() {
    return 3 + fontMetrics().width(QLatin1Char('9')) * 6;
}

void DisasmWidget::updateLineNumberAreaWidth() {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void DisasmWidget::updateLineNumberArea(const QRect &recta, int dy) {
    if (dy) {
        addressArea->scroll(0, dy);
    } else {
        addressArea->update(0, recta.y(), addressArea->width(), recta.height());
    }
    if (recta.contains(viewport()->rect()))
        updateLineNumberAreaWidth();
}

void DisasmWidget::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    addressArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void DisasmWidget::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extra;

    QTextEdit::ExtraSelection selection;

    QColor lineColor = QColor(Qt::yellow).lighter(160);

    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    extra.append(selection);

    setExtraSelections(extra);
}

void DisasmWidget::lineNumberAreaPaintEvent(QPaintEvent *e) {
    QPainter painter(addressArea);
    painter.fillRect(e->rect(), Qt::lightGray);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int blockSize = 0;
    int address_offset;

    for(int i=0; i<blockNumber; i++) {
        address_offset = disasm.start_address + blockSize;
        blockSize += mem.debug.block[address_offset]&15;
    }

    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();
    int width_s = addressArea->width() - 1;

    while (block.isValid() && top <= e->rect().bottom()) {
        address_offset = disasm.start_address + blockSize;
        if (block.isVisible() && bottom >= e->rect().top()) {
            QString number = QString::number(address_offset, 16).rightJustified(6, '0').toUpper();
            painter.setPen(Qt::black);
            painter.drawText(0, top, width_s, fontMetrics().height(), Qt::AlignLeft, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        blockSize += mem.debug.block[address_offset]&15;
    }
}
