#include "datawidget.h"
#include "utils.h"
#include "mainwindow.h"

#include <QtWidgets/QApplication>
#include <QStringView>
#include <QLatin1StringView>

#include <algorithm>
#include <array>


using namespace Qt::StringLiterals;

namespace {
    inline constexpr std::array controlFlowMnemonics {
        "CALL"_L1, "DJNZ"_L1, "JP"_L1, "JR"_L1, "RET"_L1, "RETI"_L1, "RETN"_L1, "RST"_L1
    };

    inline constexpr std::array noTargetMnemonics {
        "RET"_L1, "RETI"_L1, "RETN"_L1
    };

    inline constexpr std::array reservedTokens {
        "A"_L1, "B"_L1, "C"_L1, "D"_L1, "E"_L1, "H"_L1, "L"_L1, "I"_L1, "R"_L1,
        "AF"_L1, "BC"_L1, "DE"_L1, "HL"_L1, "SP"_L1,
        "IX"_L1, "IY"_L1, "IXH"_L1, "IXL"_L1,
        "IYH"_L1, "IYL"_L1, "MB"_L1,
        "NZ"_L1, "Z"_L1, "NC"_L1, "C"_L1,
        "PO"_L1, "PE"_L1, "P"_L1, "M"_L1
    };
}

DataWidget::DataWidget(QWidget *parent) : QPlainTextEdit{parent} {
    moveable = false;
    highlighter = new AsmHighlighter(document());
    QFont font = this->font();
    font.setStyleHint(QFont::TypeWriter);
    setFont(font);
    setContextMenuPolicy(Qt::CustomContextMenu);
}

void DataWidget::updateDarkMode() {
    bool darkMode = isRunningInDarkMode();
    for (QTextEdit::ExtraSelection &selection : highlights) {
        selection.format.setBackground(selection.format.colorProperty(QTextFormat::UserProperty + darkMode));
    }
    updateAllHighlights();
    delete highlighter;
    highlighter = new AsmHighlighter(document());
}

void DataWidget::clearAllHighlights() {
    disconnect(this, &DataWidget::cursorPositionChanged, this, &DataWidget::highlightCurrentLine);
    while (!highlights.isEmpty()) {
        highlights.removeFirst();
    }

    highlights.clear();
    updateAllHighlights();
    delete highlighter;
    highlighter = new AsmHighlighter(document());
}

void DataWidget::updateAllHighlights() {
    setExtraSelections(highlights);
    connect(this, &DataWidget::cursorPositionChanged, this, &DataWidget::highlightCurrentLine);
}

QString DataWidget::getSelectedAddr() const {
    if (!isEnabled()) {
        return QStringLiteral("000000");
    }
    QTextCursor c = textCursor();
    c.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    c.setPosition(c.position()+6, QTextCursor::KeepAnchor); // +6 == size of the address
                                                            // See MainWindow::drawNextDisassembleLine() for details
    return c.selectedText();
}

bool DataWidget::labelCheck() const {
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
        addHighlight(QColor(Qt::yellow).lighter(160), Qt::black);
    }
}

void DataWidget::addHighlight(const QColor &lightModeColor, const QColor &darkModeColor) {
    QTextEdit::ExtraSelection selection;

    selection.format.setBackground(isRunningInDarkMode() ? darkModeColor : lightModeColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.format.setProperty(QTextFormat::UserProperty, lightModeColor);
    selection.format.setProperty(QTextFormat::UserProperty + 1, darkModeColor);
    selection.cursor = textCursor();
    selection.cursor.movePosition(QTextCursor::StartOfLine);

    highlights.append(selection);
}

void DataWidget::highlightCurrentLine() {
    if (moveable) {
        if (!highlights.isEmpty()) {
            highlights.removeLast();
        }
        addHighlight(QColor(Qt::yellow).lighter(160), Qt::black);
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

AsmHighlighter::AsmHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent) {
    HighlightingRule rule;
    bool darkMode = isRunningInDarkMode();

    addressFormat.setForeground(QColor(darkMode ? "#888" : "#444"));
    addressFormat.setFontWeight(QFont::Bold);

    watchRFormat.setForeground(QColor("#008000"));
    watchRFormat.setFontWeight(QFont::Bold);
    watchWFormat.setForeground(QColor("#808000"));
    watchWFormat.setFontWeight(QFont::Bold);
    breakPFormat.setForeground(QColor("#800000"));
    breakPFormat.setFontWeight(QFont::Bold);

    mnemonicFormat.setForeground(QColor(darkMode ? "darkorange" : "darkblue"));

    controlFlowFormat.setForeground(QColor("crimson"));
    controlFlowTargetFormat.setForeground(QColor("#dc795d"));

    symbolFormat.setFontWeight(disasm.bold_sym ? QFont::DemiBold : QFont::Normal);
    rule.pattern = QRegularExpression("\\b\\w+\\b");
    rule.format = symbolFormat;
    highlightingRules.append(rule);

    decimalFormat.setForeground(QColor(darkMode ? "lime" : "green"));
    rule.pattern = QRegularExpression("\\b\\d+\\b");
    rule.format = decimalFormat;
    highlightingRules.append(rule);

    registerFormat.setForeground(QColor(darkMode ? "magenta" : "purple"));
    rule.pattern = QRegularExpression("\\b([abcdehlirmpz]|af|bc|de|hl|sp|i[xy][hl]?|mb|n[cz]|p[eo])\\b", QRegularExpression::CaseInsensitiveOption);
    rule.format = registerFormat;
    highlightingRules.append(rule);

    hexFormat.setForeground(QColor(darkMode ? "lime" : "green"));
    rule.pattern = QRegularExpression("\\$[0-9a-fA-F]+\\b");
    rule.format = hexFormat;
    highlightingRules.append(rule);

    parenFormat.setForeground(QColor(darkMode ? "lightblue" : "navy"));
    rule.pattern = QRegularExpression("[()]");
    rule.format = parenFormat;
    highlightingRules.append(rule);

    labelPattern = QRegularExpression(QStringLiteral("^(%1)\\s+(\\S+):")
                                      .arg(disasm.addr ? QStringLiteral("[0-9a-fA-F]+") : QString()));
    instructionPattern = QRegularExpression(QStringLiteral("^(%1) ([ R])([ W])([ X])\\s+(%2)\\s+(\\S+)")
                                            .arg(disasm.addr ? QStringLiteral("[0-9a-fA-F]+") : QString(),
                                                 disasm.bytes ? QStringLiteral("[0-9a-fA-F]+") : QStringLiteral(" ")));
}

void AsmHighlighter::highlightBlock(const QString &text) {
    QRegularExpressionMatch match;
    if ((match = labelPattern.match(text)).hasMatch()) {
        setFormat(match.capturedStart(1), match.capturedLength(1), addressFormat);
        setFormat(match.capturedStart(2), match.capturedLength(2), symbolFormat);
    } else if ((match = instructionPattern.match(text)).hasMatch()) {
        setFormat(match.capturedStart(1), match.capturedLength(1), addressFormat);
        setFormat(match.capturedStart(2), match.capturedLength(2), watchRFormat);
        setFormat(match.capturedStart(3), match.capturedLength(3), watchWFormat);
        setFormat(match.capturedStart(4), match.capturedLength(4), breakPFormat);
        setFormat(match.capturedStart(5), match.capturedLength(5), bytesFormat);
        setFormat(match.capturedStart(6), match.capturedLength(6), mnemonicFormat);

        const QString fullMnemonic = match.captured(6);
        const QString primary = fullMnemonic.section('.', 0, 0).toUpper();
        const QStringView pv{primary};

        const bool isControlFlow = std::any_of(controlFlowMnemonics.begin(), controlFlowMnemonics.end(),
                        [&](const QLatin1StringView tok){ return pv == tok; });

        if (isControlFlow) {
            setFormat(match.capturedStart(6), match.capturedLength(6), controlFlowFormat);
        }

        foreach(const HighlightingRule &rule, highlightingRules) {
            QRegularExpressionMatchIterator iter = rule.pattern.globalMatch(text, match.capturedEnd());
            while (iter.hasNext()) {
                const auto& innerMatch = iter.next();
                if (innerMatch.hasMatch()) {
                    setFormat(innerMatch.capturedStart(), innerMatch.capturedLength(), rule.format);
                }
            }
        }

        if (!isControlFlow) {
            return;
        }

        if (std::any_of(noTargetMnemonics.begin(), noTargetMnemonics.end(),
                        [&](const QLatin1StringView tok){ return pv == tok; })) {
            return;
        }

        const int operandStart = match.capturedEnd();
        const QString operands = text.mid(operandStart);

        // hex address patterns allowing optional +/ - after '$' for relative forms like $+5
        const QRegularExpression hexTargetRe(QStringLiteral("\\$[+\\-]?[0-9A-Fa-f]+\\b"));
        QRegularExpressionMatchIterator hIt = hexTargetRe.globalMatch(operands);
        while (hIt.hasNext()) {
            const auto m2 = hIt.next();
            if (m2.hasMatch()) {
                const int s = operandStart + m2.capturedStart();
                const int l = m2.capturedLength();
                setFormat(s, l, controlFlowTargetFormat);
            }
        }

        // equate/label tokens (exclude registers and condition codes)
        const QRegularExpression labelRe(QStringLiteral("\\b[._A-Za-z][._A-Za-z0-9]*\\b"));

        QRegularExpressionMatchIterator lIt = labelRe.globalMatch(operands);
        while (lIt.hasNext()) {
            const auto m3 = lIt.next();
            if (m3.hasMatch()) {
                const QString tok = m3.captured().toUpper();
                if (std::any_of(reservedTokens.begin(), reservedTokens.end(),
                                [&](const QLatin1StringView t){ return tok == t; })) {
                    continue;
                }
                const int start = operandStart + m3.capturedStart();
                const int len = m3.capturedLength();
                setFormat(start, len, controlFlowTargetFormat);
            }
        }
    }
}