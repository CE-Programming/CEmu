#include <QtWidgets>

#include "datawidget.h"

/* extraHighlights (0) = current line selection */

DataWidget::DataWidget(QWidget *p) : QPlainTextEdit(p) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &DataWidget::highlightCurrentLine);
}

void DataWidget::clearAllHighlights() {
    while (!extraHighlights.isEmpty()) {
        extraHighlights.removeFirst();
    }

    extraHighlights.clear();
    updateAllHighlights();
}

void DataWidget::updateAllHighlights() {
    setExtraSelections(extraHighlights);
}

QString DataWidget::getSelectedAddress() {
  QTextCursor c = textCursor();
  c.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
  c.setPosition(c.position()+6, QTextCursor::KeepAnchor); // +6 == size of the address
                                                          // See MainWindow::drawNextDisassembleLine() for details
  return c.selectedText();
}

void DataWidget::addHighlight(QColor color) {
    QTextEdit::ExtraSelection selection;

    selection.format.setBackground(color);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.movePosition(QTextCursor::StartOfLine);

    extraHighlights.append(selection);
}

void DataWidget::cursorState(bool moveable) {
    cursor_state = moveable;
    if (moveable) {
        addHighlight(QColor(Qt::yellow).lighter(160));
    }
}

void DataWidget::highlightCurrentLine() {
    if(cursor_state == true) {
        extraHighlights.removeLast();
        addHighlight(QColor(Qt::yellow).lighter(160));
        updateAllHighlights();
    }
}
