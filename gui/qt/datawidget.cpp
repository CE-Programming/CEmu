#include "datawidget.h"
#include "utils.h"
#include "mainwindow.h"

#include <QtWidgets/QApplication>

DataWidget::DataWidget(QWidget *p) : QPlainTextEdit(p) {
    moveable = false;
    setContextMenuPolicy(Qt::CustomContextMenu);
}

void DataWidget::clearAllHighlights() {
    disconnect(this, &DataWidget::cursorPositionChanged, this, &DataWidget::highlightCurrentLine);
    while (!highlights.isEmpty()) {
        highlights.removeFirst();
    }

    highlights.clear();
    updateAllHighlights();
}

void DataWidget::updateAllHighlights() {
    setExtraSelections(highlights);
    connect(this, &DataWidget::cursorPositionChanged, this, &DataWidget::highlightCurrentLine);
}

QString DataWidget::getSelectedAddress() {
    if (!isEnabled()) {
        return QStringLiteral("000000");
    }
    QTextCursor c = textCursor();
    c.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    c.setPosition(c.position()+6, QTextCursor::KeepAnchor); // +6 == size of the address
                                                            // See MainWindow::drawNextDisassembleLine() for details
    return c.selectedText();
}

bool DataWidget::labelCheck() {
    if (!isEnabled()) {
        return false;
    }
    QTextCursor c = textCursor();
    c.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);
    c.setPosition(c.position()-1, QTextCursor::KeepAnchor);

    return c.selectedText().at(0) == ':';
}

void DataWidget::cursorState(bool state) {
    moveable = state;
    if (moveable) {
        addHighlight(QColor(Qt::yellow).lighter(160));
    }
}

void DataWidget::addHighlight(const QColor &color) {
    QTextEdit::ExtraSelection selection;

    selection.format.setBackground(color);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.movePosition(QTextCursor::StartOfLine);

    highlights.append(selection);
}

void DataWidget::highlightCurrentLine() {
    if (moveable) {
        if (!highlights.isEmpty()) {
            highlights.removeLast();
        }
        addHighlight(QColor(Qt::yellow).lighter(160));
        setExtraSelections(highlights);
        if (QApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
            bool ok = true;

            QTextCursor cursor = textCursor();
            cursor.select(QTextCursor::WordUnderCursor);
            if (cursor.selectedText().isEmpty()) {
                return;
            }

            QString weird(QStringLiteral("[]()\",./\\-="));

            if (weird.contains(cursor.selectedText().at(0))) {
                cursor.movePosition(QTextCursor::WordLeft);
                cursor.movePosition(QTextCursor::WordLeft);
                cursor.select(QTextCursor::WordUnderCursor);
            }
            setTextCursor(cursor);

            QString equ = getAddressOfEquate(cursor.selectedText().toUpper().toStdString());
            uint32_t address;

            if (!equ.isEmpty()) {
                address = hex2int(equ);
            } else {
                address = textCursor().selectedText().toUInt(&ok, 16);
            }

            if (ok) {
                if (QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)) {
                    emit gotoMemoryAddress(address);
                } else {
                    emit gotoDisasmAddress(address);
                }
            }
        }
    }
}
