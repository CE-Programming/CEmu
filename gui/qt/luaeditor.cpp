/* Inspired by the Qt Code Editor Example - BSD License */

#include "luaeditor.h"

#include "utils.h"

#include <QtCore/QHash>
#include <QtCore/QStringListModel>
#include <QtGui/QFocusEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
#include <QtGui/QTextBlock>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QCompleter>
#include <QtWidgets/QScrollBar>

#include <algorithm>

namespace {

constexpr int GutterLeftPadding = 6;
constexpr int GutterRightPadding = 8;
constexpr int LongStringStateBase = 1024;

const QStringList &luaKeywords()
{
    static const QStringList values = QStringLiteral("and break do else elseif end false for function goto if "
                                                     "in local nil not or repeat "
                                                     "return then true until while")
                                          .split(QLatin1Char(' '));
    return values;
}

const QStringList &luaBuiltins()
{
    static const QStringList values = QStringLiteral("_G _VERSION assert collectgarbage dofile error "
                                                     "getmetatable ipairs load loadfile next "
                                                     "pairs pcall print rawequal rawget rawlen rawset require "
                                                     "select setmetatable tonumber "
                                                     "tostring type warn xpcall coroutine debug io math os "
                                                     "package string table utf8")
                                          .split(QLatin1Char(' '));
    return values;
}

const QStringList &cemuGlobals()
{
    static const QStringList values = QStringLiteral("cemu cpu coproc mem vars peripherals lcd keys gui emu link dbg basic "
                                                     "autotester cLog cErr R F")
                                          .split(QLatin1Char(' '));
    return values;
}

QString words(const char *value)
{
    return QString::fromLatin1(value);
}

QStringList wordList(const char *value)
{
    return words(value).split(QLatin1Char(' '), Qt::SkipEmptyParts);
}

QString wordPattern(const QStringList &values)
{
    QStringList escaped;
    escaped.reserve(values.size());
    for (const QString &value : values) escaped.append(QRegularExpression::escape(value));
    return QStringLiteral("\\b(?:%1)\\b").arg(escaped.join(QLatin1Char('|')));
}

const QHash<QString, QStringList> &completionMembers()
{
    static const QStringList registers = wordList("flags AF F A BC BCS C B BCU DE DES E D DEU HL HLS L H HLU _HL "
                                                  "IX IXS IXL IXH "
                                                  "IXU IY IYS IYL IYH IYU _AF _BC _DE SPS SPL PC PCS PCL PCH PCU "
                                                  "I R MBASE");
    static const QStringList flags = wordList("C N PV _3 H _5 Z S");
    static const QStringList ranges = wordList("control flash sha256 usb lcd interrupts watchdog timers rtc "
                                               "protected keypad backlight "
                                               "misc spi uart reserved");
    static const QHash<QString, QStringList> values = {
        {words("cemu"), wordList("on off onUnload offUnload cleanup stopScript reloadScript scriptPath")},
        {words("cpu"), wordList("registers halted ADL MADL IEF1 IEF2 inBlock cycles next prefetch")},
        {words("cpu.registers"), registers},
        {words("R"), registers},
        {words("cpu.registers.flags"), flags},
        {words("R.flags"), flags},
        {words("F"), flags},
        {words("coproc"), wordList("present bootloader state time readByte readHalf readWord "
                                   "writeByte writeHalf writeWord reset loadFlash spiSelect "
                                   "spiPeek spiTransfer uartSend uartReceive")},
        {words("mem"), wordList("read readTable readByte readShort readLong readWord write "
                                "writeByte writeShort writeLong writeWord fill copy crc32 search")},
        {words("vars"), wordList("list find read launch types")},
        {words("peripherals"), wordList("peek poke read write describe snapshot "
                                        "monitor monitorState ranges")},
        {words("peripherals.ranges"), ranges},
        {words("lcd"), wordList("controllerState panelState state panelCommand "
                                "refreshDebugPane applyDebugPane "
                                "showDebugPane setDma setGamma setResponse "
                                "setScale setUpscale setSkin width height framebuffer region pixel "
                                "matches frameHash frameInfo")},
        {words("keys"), wordList("press sequence down up hold")},
        {words("gui"), wordList("screenshot refresh messageBox status setKeypadColor "
                                "setFullscreen openScriptsFolder quit")},
        {words("emu"), wordList("reset reloadROM throttle setSpeed wait time cycles "
                                "after afterCycles every everyCycles cancel cancelAll "
                                "saveState loadState sendFile deviceType")},
        {words("link"), wordList("send cancel busy status RAM ARCHIVE AUTO")},
        {words("dbg"), wordList("stop resume stepIn stepOver stepNext stepOut stepUntilReturn "
                                "addBreakpoint "
                                "removeBreakpoint addWatchpoint removeWatchpoint watchRegister "
                                "registerWatchState breakpoints watchpoints peripheralMonitors "
                                "registerWatches registerSnapshot equates resolveSymbol symbolAt symbolsAt "
                                "loadEquates clearBreakpoints clearWatchpoints "
                                "gotoDisasm disasm disasmPC")},
        {words("debug"), wordList("debug gethook getinfo getlocal getmetatable getregistry "
                                  "getupvalue getuservalue setcstacklimit sethook setlocal "
                                  "setmetatable setupvalue setuservalue traceback upvalueid upvaluejoin")},
        {words("basic"), wordList("enable enabled showDebugger state source step stepNext resume "
                                  "setHighlight setShowFetches "
                                  "setShowTemporaryParser setLiveExecution setSourceBreakpoint "
                                  "sourceBreakpoints "
                                  "watchVariable watchedVariables prepareSource deindentSource")},
        {words("autotester"), wordList("loadJSON reloadJSON launchTest")},
        {words("coroutine"), wordList("close create isyieldable resume running status wrap yield")},
        {words("math"), wordList("abs acos asin atan ceil cos deg exp floor fmod huge log max "
                                 "maxinteger min mininteger "
                                 "modf pi rad random randomseed sin sqrt tan tointeger type ult")},
        {words("string"), wordList("byte char dump find format gmatch gsub len "
                                   "lower match pack packsize rep reverse sub "
                                   "unpack upper")},
        {words("table"), wordList("concat insert move pack remove sort unpack")},
        {words("utf8"), wordList("char charpattern codepoint codes len offset")}};
    return values;
}

QStringList globalCompletions()
{
    QStringList values = luaKeywords();
    values.append(luaBuiltins());
    values.append(cemuGlobals());
    std::sort(values.begin(), values.end(),
              [](const QString &left, const QString &right) { return left.compare(right, Qt::CaseInsensitive) < 0; });
    values.removeDuplicates();
    return values;
}

int longBracketEquals(const QString &text, int openingBracket)
{
    if (openingBracket < 0 || openingBracket >= text.size() || text.at(openingBracket) != QLatin1Char('[')) {
        return -1;
    }
    int index = openingBracket + 1;
    while (index < text.size() && text.at(index) == QLatin1Char('=')) ++index;
    return index < text.size() && text.at(index) == QLatin1Char('[') ? index - openingBracket - 1 : -1;
}

QString longBracketClose(int equals)
{
    return QLatin1Char(']') + QString(equals, QLatin1Char('=')) + QLatin1Char(']');
}

} // namespace

QSize LuaLineNumberArea::sizeHint() const
{
    return QSize(luaEditor->lineNumberAreaWidth(), 0);
}

LuaEditor::LuaEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    highlighter = new LuaHighlighter(document());
    lineNumberArea = new LuaLineNumberArea(this);

    completionModel = new QStringListModel(this);
    completer = new QCompleter(completionModel, this);
    completer->setWidget(this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setFilterMode(Qt::MatchStartsWith);
    completer->setWrapAround(false);

    connect(completer, QOverload<const QString &>::of(&QCompleter::activated), this, &LuaEditor::insertCompletion);
    connect(this, &QPlainTextEdit::blockCountChanged, this, &LuaEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &LuaEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &LuaEditor::highlightCurrentLine);

    setLineWrapMode(QPlainTextEdit::NoWrap);
    updateEditorMetrics();
    updateDarkMode();
}

int LuaEditor::lineNumberAreaWidth() const
{
    const QString widestNumber(QString::number(qMax(1, blockCount())).size(), QLatin1Char('9'));
    return GutterLeftPadding + fontMetrics().horizontalAdvance(widestNumber) + GutterRightPadding;
}

void LuaEditor::updateEditorMetrics()
{
    lineNumberArea->setFont(font());
    setTabStopDistance(fontMetrics().horizontalAdvance(QStringLiteral("    ")));
    updateLineNumberAreaWidth();
}

void LuaEditor::updateLineNumberAreaWidth(int)
{
    const int width = lineNumberAreaWidth();
    setViewportMargins(width, 0, 0, 0);
    const QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), width, cr.height()));
}

void LuaEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }
    if (rect.contains(viewport()->rect())) updateLineNumberAreaWidth();
}

void LuaEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    updateLineNumberAreaWidth();
}

void LuaEditor::changeEvent(QEvent *event)
{
    QPlainTextEdit::changeEvent(event);
    switch (event->type()) {
    case QEvent::FontChange:
        updateEditorMetrics();
        break;
    case QEvent::ApplicationPaletteChange:
    case QEvent::PaletteChange:
    case QEvent::ThemeChange:
        updateDarkMode();
        break;
    default:
        break;
    }
}

void LuaEditor::updateDarkMode()
{
    const bool darkMode = isRunningInDarkMode();
    QPalette editorPalette = palette();
    const QColor base = QColor(darkMode ? "#1E1E1E" : "#FFFFFF");
    const QColor text = QColor(darkMode ? "#D4D4D4" : "#202124");
    if (editorPalette.color(QPalette::Base) != base || editorPalette.color(QPalette::Text) != text) {
        editorPalette.setColor(QPalette::Base, base);
        editorPalette.setColor(QPalette::Text, text);
        editorPalette.setColor(QPalette::AlternateBase, QColor(darkMode ? "#252526" : "#F3F4F6"));
        editorPalette.setColor(QPalette::Highlight, QColor(darkMode ? "#264F78" : "#ADD6FF"));
        editorPalette.setColor(QPalette::HighlightedText, QColor(darkMode ? "#FFFFFF" : "#111111"));
        setPalette(editorPalette);
    }
    if (highlighter) highlighter->updateTheme(darkMode);
    highlightCurrentLine();
    if (lineNumberArea) lineNumberArea->update();
}

void LuaEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> selections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = palette().color(QPalette::Highlight);
        lineColor.setAlpha(isRunningInDarkMode() ? 42 : 26);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        selections.append(selection);
    }
    setExtraSelections(selections);
    if (lineNumberArea) lineNumberArea->update();
}

void LuaEditor::lineNumberAreaPaintEvent(QPaintEvent *event) const
{
    QPainter painter(lineNumberArea);
    const QPalette colors = palette();
    QColor background = colors.color(QPalette::AlternateBase);
    if (background == colors.color(QPalette::Base)) background = colors.color(QPalette::Window);
    painter.fillRect(event->rect(), background);
    painter.setFont(font());

    QColor separator = colors.color(QPalette::Mid);
    separator.setAlpha(150);
    painter.setPen(separator);
    painter.drawLine(lineNumberArea->width() - 1, event->rect().top(), lineNumberArea->width() - 1,
                     event->rect().bottom());

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());
    const int currentBlock = textCursor().blockNumber();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QFont numberFont = font();
            QColor numberColor = colors.color(QPalette::Text);
            if (blockNumber == currentBlock) {
                numberFont.setBold(true);
                numberColor = colors.color(QPalette::Highlight);
            } else {
                numberColor.setAlpha(165);
            }
            painter.setFont(numberFont);
            painter.setPen(numberColor);
            const QRect numberRect(GutterLeftPadding, top,
                                   lineNumberArea->width() - GutterLeftPadding - GutterRightPadding, bottom - top);
            painter.drawText(numberRect, Qt::AlignRight | Qt::AlignVCenter, QString::number(blockNumber + 1));
        }
        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

bool LuaEditor::isCompletionSuppressed() const
{
    QTextCursor cursor = textCursor();
    if (cursor.position() <= cursor.block().position()) {
        const QTextBlock previous = cursor.block().previous();
        return previous.isValid() && previous.userState() != 0;
    }
    cursor.setPosition(cursor.position() - 1);
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
    return cursor.charFormat().property(QTextFormat::UserProperty).toInt() != 0;
}

void LuaEditor::showCompletion(bool explicitRequest)
{
    if (isCompletionSuppressed()) {
        completer->popup()->hide();
        return;
    }

    const QTextCursor cursor = textCursor();
    const QString beforeCursor = cursor.block().text().left(cursor.positionInBlock());
    static const QRegularExpression memberExpression(
        QStringLiteral("((?:[A-Za-z_][A-Za-z0-9_]*\\.)+)([A-Za-z0-9_]*)$"));
    static const QRegularExpression wordExpression(QStringLiteral("([A-Za-z_][A-Za-z0-9_]*)$"));

    QStringList candidates;
    QString prefix;
    const QRegularExpressionMatch memberMatch = memberExpression.match(beforeCursor);
    if (memberMatch.hasMatch()) {
        QString objectPath = memberMatch.captured(1);
        objectPath.chop(1);
        prefix = memberMatch.captured(2);
        candidates = completionMembers().value(objectPath);
        if (candidates.isEmpty()) {
            completer->popup()->hide();
            return;
        }
    } else {
        const QRegularExpressionMatch wordMatch = wordExpression.match(beforeCursor);
        prefix = wordMatch.hasMatch() ? wordMatch.captured(1) : QString();
        if (!explicitRequest && prefix.size() < 2) {
            completer->popup()->hide();
            return;
        }
        candidates = globalCompletions();
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const QString &left, const QString &right) { return left.compare(right, Qt::CaseInsensitive) < 0; });
    completionModel->setStringList(candidates);
    completer->setCompletionPrefix(prefix);
    completer->popup()->setCurrentIndex(completer->completionModel()->index(0, 0));
    if (completer->completionCount() == 0) {
        completer->popup()->hide();
        return;
    }

    QRect popupRect = cursorRect();
    popupRect.setWidth(completer->popup()->sizeHintForColumn(0) +
                       completer->popup()->verticalScrollBar()->sizeHint().width() + 12);
    completer->complete(popupRect);
}

void LuaEditor::insertCompletion(const QString &completion)
{
    QTextCursor cursor = textCursor();
    const int suffixLength = completion.size() - completer->completionPrefix().size();
    if (suffixLength > 0) cursor.insertText(completion.right(suffixLength));
    setTextCursor(cursor);
}

void LuaEditor::indentNewLine()
{
    QTextCursor cursor = textCursor();
    const QString previousLine = cursor.block().previous().text();
    const QRegularExpressionMatch indentMatch = QRegularExpression(QStringLiteral("^\\s*")).match(previousLine);
    QString indentation = indentMatch.captured(0);
    static const QRegularExpression opensBlock(QStringLiteral("(?:\\b(?:then|do|repeat|function)\\b|[\\{\\(]\\s*)$"));
    if (opensBlock.match(previousLine.trimmed()).hasMatch()) indentation += QStringLiteral("    ");
    cursor.insertText(indentation);
    setTextCursor(cursor);
}

void LuaEditor::keyPressEvent(QKeyEvent *event)
{
    if (completer->popup()->isVisible()) {
        switch (event->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            event->ignore();
            return;
        case Qt::Key_Tab: {
            const QModelIndex current = completer->popup()->currentIndex();
            if (current.isValid()) insertCompletion(current.data().toString());
            completer->popup()->hide();
            event->accept();
            return;
        }
        case Qt::Key_Escape:
            completer->popup()->hide();
            event->accept();
            return;
        default:
            break;
        }
    }

    const bool completionShortcut = event->key() == Qt::Key_Space && event->modifiers().testFlag(Qt::ControlModifier);
    if (completionShortcut) {
        showCompletion(true);
        event->accept();
        return;
    }

    const bool newLine = event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter;
    QPlainTextEdit::keyPressEvent(event);
    if (newLine && event->modifiers() == Qt::NoModifier) indentNewLine();

    const bool hasTextModifier = event->modifiers().testFlag(Qt::ControlModifier) ||
                                 event->modifiers().testFlag(Qt::AltModifier) ||
                                 event->modifiers().testFlag(Qt::MetaModifier);
    if (!hasTextModifier &&
        (!event->text().isEmpty() || event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete)) {
        showCompletion(false);
    } else {
        completer->popup()->hide();
    }
}

void LuaEditor::focusOutEvent(QFocusEvent *event)
{
    completer->popup()->hide();
    QPlainTextEdit::focusOutEvent(event);
}

LuaHighlighter::LuaHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    rebuildRules(isRunningInDarkMode());
}

void LuaHighlighter::updateTheme(bool darkMode)
{
    rebuildRules(darkMode);
    rehighlight();
}

void LuaHighlighter::rebuildRules(bool darkMode)
{
    highlightingRules.clear();

    keywordFormat = QTextCharFormat();
    keywordFormat.setForeground(QColor(darkMode ? "#7CB7FF" : "#0033B3"));
    keywordFormat.setFontWeight(QFont::DemiBold);
    highlightingRules.append({QRegularExpression(wordPattern(luaKeywords())), keywordFormat});

    builtinFormat = QTextCharFormat();
    builtinFormat.setForeground(QColor(darkMode ? "#4EC9B0" : "#007A72"));
    builtinFormat.setFontWeight(QFont::DemiBold);
    highlightingRules.append({QRegularExpression(wordPattern(luaBuiltins())), builtinFormat});

    literalFormat = QTextCharFormat();
    literalFormat.setForeground(QColor(darkMode ? "#C586C0" : "#871094"));
    literalFormat.setFontWeight(QFont::DemiBold);
    highlightingRules.append({QRegularExpression(QStringLiteral("\\b(?:nil|true|false)\\b")), literalFormat});

    cemuGlobalsFormat = QTextCharFormat();
    cemuGlobalsFormat.setForeground(QColor(darkMode ? "#FF7B72" : "#B42318"));
    cemuGlobalsFormat.setFontWeight(QFont::DemiBold);
    highlightingRules.append({QRegularExpression(wordPattern(cemuGlobals())), cemuGlobalsFormat});

    numberFormat = QTextCharFormat();
    numberFormat.setForeground(QColor(darkMode ? "#B5CEA8" : "#7A3E9D"));
    highlightingRules.append({QRegularExpression(QStringLiteral(
                                  R"((\b0[xX][0-9a-fA-F]+\b)|(((\b[0-9]+)?\.)?\b[0-9]+([eE][-+]?[0-9]+)?\b))")),
                              numberFormat});

    functionFormat = QTextCharFormat();
    functionFormat.setForeground(QColor(darkMode ? "#DCDCAA" : "#795E26"));
    highlightingRules.append(
        {QRegularExpression(QStringLiteral("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\s*\\()")), functionFormat});

    quotationFormat = QTextCharFormat();
    quotationFormat.setForeground(QColor(darkMode ? "#CE9178" : "#008000"));
    quotationFormat.setProperty(QTextFormat::UserProperty, 1);

    singleLineCommentFormat = QTextCharFormat();
    singleLineCommentFormat.setForeground(QColor(darkMode ? "#78A66A" : "#5F6A60"));
    singleLineCommentFormat.setProperty(QTextFormat::UserProperty, 2);
    multiLineCommentFormat = singleLineCommentFormat;
}

void LuaHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : std::as_const(highlightingRules)) {
        QRegularExpressionMatchIterator matches = rule.pattern.globalMatch(text);
        while (matches.hasNext()) {
            const QRegularExpressionMatch match = matches.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    setCurrentBlockState(0);
    int index = 0;
    const int previousState = previousBlockState();
    if (previousState > 0) {
        const bool stringState = previousState >= LongStringStateBase;
        const int equals = stringState ? previousState - LongStringStateBase : previousState - 1;
        const QString close = longBracketClose(equals);
        const int end = text.indexOf(close);
        if (end < 0) {
            setFormat(0, text.size(), stringState ? quotationFormat : multiLineCommentFormat);
            setCurrentBlockState(previousState);
            return;
        }
        index = end + close.size();
        setFormat(0, index, stringState ? quotationFormat : multiLineCommentFormat);
    }

    while (index < text.size()) {
        const QChar character = text.at(index);
        if (character == QLatin1Char('\'') || character == QLatin1Char('"')) {
            const QChar quote = character;
            const int start = index++;
            bool escaped = false;
            while (index < text.size()) {
                const QChar current = text.at(index++);
                if (current == quote && !escaped) break;
                if (current == QLatin1Char('\\') && !escaped) {
                    escaped = true;
                } else {
                    escaped = false;
                }
            }
            setFormat(start, index - start, quotationFormat);
            continue;
        }

        if (character == QLatin1Char('-') && index + 1 < text.size() && text.at(index + 1) == QLatin1Char('-')) {
            const int equals = longBracketEquals(text, index + 2);
            if (equals < 0) {
                setFormat(index, text.size() - index, singleLineCommentFormat);
                return;
            }
            const QString close = longBracketClose(equals);
            const int openingLength = equals + 4;
            const int end = text.indexOf(close, index + openingLength);
            if (end < 0) {
                setFormat(index, text.size() - index, multiLineCommentFormat);
                setCurrentBlockState(equals + 1);
                return;
            }
            const int length = end + close.size() - index;
            setFormat(index, length, multiLineCommentFormat);
            index += length;
            continue;
        }

        const int equals = longBracketEquals(text, index);
        if (equals >= 0) {
            const QString close = longBracketClose(equals);
            const int openingLength = equals + 2;
            const int end = text.indexOf(close, index + openingLength);
            if (end < 0) {
                setFormat(index, text.size() - index, quotationFormat);
                setCurrentBlockState(LongStringStateBase + equals);
                return;
            }
            const int length = end + close.size() - index;
            setFormat(index, length, quotationFormat);
            index += length;
            continue;
        }
        ++index;
    }
}
