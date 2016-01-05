#include <QtWidgets>

#include "disasmwidget.h"

DisasmWidget::DisasmWidget(QWidget *p) : QPlainTextEdit(p) {
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &DisasmWidget::highlightCurrentLine);
    highlightCurrentLine();
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
