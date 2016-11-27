/* Inspired from a code in the Qt Examples - BSD License */

#include <QtWidgets>

#include "luaeditor.h"

LuaEditor::LuaEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    highlighter = new LuaHighlighter(document());

    lineNumberArea = new LuaLineNumberArea(this);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &LuaEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &LuaEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &LuaEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int LuaEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
#else
    int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;
#endif

    return space;
}

void LuaEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void LuaEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void LuaEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    const QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void LuaEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        const QColor lineColor = QColor(Qt::yellow).lighter(160);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void LuaEditor::lineNumberAreaPaintEvent(QPaintEvent *event) const
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);
    painter.setPen(Qt::black);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);
            painter.drawText(-2, top, lineNumberArea->width(), fontMetrics().height(), Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

/* Highlighter */

LuaHighlighter::LuaHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    keywordFormat.setForeground(Qt::darkBlue);
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;
    keywordPatterns << "\\band\\b" << "\\bbreak\\b" << "\\bdo\\b" << "\\belse\\b" << "\\belseif\\b"
                    << "\\bend\\b" << "\\bfor\\b" << "\\bif\\b" << "\\bin\\b" << "\\blocal\\b"
                    << "\\bnot\\b" << "\\bor\\b" << "\\brepeat\\b" << "\\breturn\\b" << "\\bthen\\b"
                    << "\\buntil\\b" << "\\bwhile\\n";
    foreach (const QString &pattern, keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    builtinFormat.setForeground(Qt::darkCyan);
    builtinFormat.setFontWeight(QFont::Bold);
    QStringList builtinPatterns;
    builtinPatterns << "\\b_G\\b" << "\\b_VERSION\\b" << "\\bassert\\b" << "\\bcollectgarbage\\b"
                    << "\\bdofile\\b" << "\\berror\\b" << "\\bgetfenv\\b" << "\\bgetmetatable\\b"
                    << "\\bipairs\\b" << "\\bload\\b" << "\\bloadfile\\b" << "\\bloadstring\\b"
                    << "\\bmodule\\b" << "\\bnext\\b" << "\\bpairs\\b" << "\\bpcall\\b" << "\\bprint\\b"
                    << "\\brawequal\\b" << "\\brawget\\b" << "\\brawset\\b" << "\\brequire\\b"
                    << "\\bselect\\b" << "\\bsetfenv\\b" << "\\bsetmetatable\\b" << "\\btonumber\\b"
                    << "\\btostring\\b" << "\\btype\\b" << "\\bunpack\\b" << "\\bxpcall\\b"
                    << "\\bcoroutine\\b" << "\\bdebug\\b" << "\\bio\\b" << "\\bmath\\b" << "\\bos\\b"
                    << "\\bpackage\\b" << "\\bstring\\b" << "\\btable\\b";
    foreach (const QString &pattern, builtinPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = builtinFormat;
        highlightingRules.append(rule);
    }

    literalFormat.setFontWeight(QFont::Bold);
    literalFormat.setForeground(Qt::darkMagenta);
    rule.pattern = QRegularExpression("\\b(nil|true|false)\\b");
    rule.format = literalFormat;
    highlightingRules.append(rule);

    cemuGlobalsFormat.setFontUnderline(true);
    cemuGlobalsFormat.setForeground(Qt::red);
    rule.pattern = QRegularExpression("\\b(cpu|devices|mem|gui|R|F)\\b");
    rule.format = cemuGlobalsFormat;
    highlightingRules.append(rule);

    numberFormat.setForeground(Qt::darkMagenta);
    rule.pattern = QRegularExpression(R"((\b0[xX][0-9a-fA-F]+\b)|(((\b[0-9]+)?\.)?\b[0-9]+([eE][-+]?[0-9]+)?\b))");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    quotationFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegularExpression(R"(("[^"]*")|('[^']*')|(\[=*\[.*\]=*\]))");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    functionFormat.setForeground(Qt::blue);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);

    singleLineCommentFormat.setForeground(Qt::gray);
    rule.pattern = QRegularExpression("--[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    multiLineCommentFormat.setForeground(Qt::gray);

    commentStartExpression = QRegularExpression("--\\[=*\\[");
    commentEndExpression = QRegularExpression("\\]=*\\]");
}

void LuaHighlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules) {
        const QRegularExpression expression(rule.pattern);
        QRegularExpressionMatchIterator iter = expression.globalMatch(text);
        while (iter.hasNext()) {
            const auto& match = iter.next();
            if (match.hasMatch()) {
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1) {
        startIndex = text.indexOf(commentStartExpression);
    }
    while (startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        const int endIndex = match.capturedStart();
        int commentLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + match.capturedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}
