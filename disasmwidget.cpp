#include <QtWidgets>

#include "disasmwidget.h"

/* extraHighlights (0) = current line selection */

DisasmWidget::DisasmWidget(QWidget *p) : QPlainTextEdit(p) {
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &DisasmWidget::highlightCurrentLine);
}

void DisasmWidget::clearAllHighlights() {
    while (!extraHighlights.isEmpty()) {
        extraHighlights.removeFirst();
    }

    extraHighlights.clear();
    updateAllHighlights();
}

void DisasmWidget::updateAllHighlights() {
    setExtraSelections(extraHighlights);
}

QString DisasmWidget::getSelectedAddress() {
  QTextCursor c = textCursor();
  c.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
  c.setPosition(c.position()+5, QTextCursor::MoveAnchor); // +4 == size of the 3 debug symbols, + 1 space
  c.setPosition(c.position()+6, QTextCursor::KeepAnchor); // +6 == size of the address
                                                          // See MainWindow::drawNextDisassembleLine() for details
  return c.selectedText();
}

void DisasmWidget::addHighlight(QColor color) {
    QTextEdit::ExtraSelection selection;

    selection.format.setBackground(color);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.movePosition(QTextCursor::StartOfLine);

    extraHighlights.append(selection);
}

void DisasmWidget::cursorState(bool moveable) {
    cursor_state = moveable;
    if (moveable) {
        addHighlight(QColor(Qt::yellow).lighter(160));
    }
}

void DisasmWidget::highlightCurrentLine() {
    if(cursor_state == true) {
        extraHighlights.removeLast();
        addHighlight(QColor(Qt::yellow).lighter(160));
        updateAllHighlights();
    }
}
