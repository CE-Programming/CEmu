#include <QtWidgets>

#include "disasmwidget.h"

DisasmWidget::DisasmWidget(QWidget *p) : QPlainTextEdit(p)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &DisasmWidget::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &DisasmWidget::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &DisasmWidget::highlightCurrentLine);

    this->setReadOnly(true);
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int DisasmWidget::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;

    return space;
}

void DisasmWidget::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void DisasmWidget::updateLineNumberArea(const QRect &recta, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, recta.y(), lineNumberArea->width(), recta.height());

    if (recta.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void DisasmWidget::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void DisasmWidget::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelectionsa;

    QTextEdit::ExtraSelection selection;
    QColor lineColor = QColor(Qt::yellow).lighter(160);
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    extraSelectionsa.append(selection);

    setExtraSelections(extraSelectionsa);
}

void DisasmWidget::lineNumberAreaPaintEvent(QPaintEvent *e)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(e->rect(), Qt::lightGray);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top() - 1;
    int bottom = top + (int) blockBoundingRect(block).height();
    int width = lineNumberArea->width() - 1;

    while (block.isValid() && top <= e->rect().bottom()) {
        if (block.isVisible() && bottom >= e->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, width, fontMetrics().height(), Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}
