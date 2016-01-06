#include <QtWidgets>

#include "disasmwidget.h"

DisasmWidget::DisasmWidget(QWidget *p) : QPlainTextEdit(p) {
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &DisasmWidget::highlightCurrentLine);
    highlightCurrentLine();
}

void DisasmWidget::highlightPCLine() {
    pccursor = textCursor();
    pccursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::MoveAnchor);
}

void DisasmWidget::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extra;
    QTextEdit::ExtraSelection selection;
    QTextEdit::ExtraSelection pcselection;

    QColor lineColor = QColor(Qt::yellow).lighter(160);
    QColor pclineColor = QColor(Qt::red).lighter(160);

    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();

    pcselection.format.setBackground(pclineColor);
    pcselection.format.setProperty(QTextFormat::FullWidthSelection, true);
    pcselection.cursor = pccursor;
    pcselection.cursor.clearSelection();

    extra.append(selection);
    extra.append(pcselection);
    setExtraSelections(extra);
}
