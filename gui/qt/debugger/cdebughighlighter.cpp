#include "cdebughighlighter.h"
#include "sourceswidget.h"

#include <QtWidgets/QPlainTextEdit>

#include <cassert>

const QSet<QString> CDebugHighlighter::s_keywords{
    QStringLiteral("_Align"),
    QStringLiteral("_At"),
    QStringLiteral("auto"),
    QStringLiteral("break"),
    QStringLiteral("case"),
    QStringLiteral("char"),
    QStringLiteral("const"),
    QStringLiteral("continue"),
    QStringLiteral("default"),
    QStringLiteral("do"),
    QStringLiteral("double"),
    QStringLiteral("else"),
    QStringLiteral("enum"),
    QStringLiteral("extern"),
    QStringLiteral("float"),
    QStringLiteral("for"),
    QStringLiteral("goto"),
    QStringLiteral("if"),
    QStringLiteral("int"),
    QStringLiteral("interrupt"),
    QStringLiteral("long"),
    QStringLiteral("nested_interrupt"),
    QStringLiteral("register"),
    QStringLiteral("return"),
    QStringLiteral("short"),
    QStringLiteral("signed"),
    QStringLiteral("sizeof"),
    QStringLiteral("static"),
    QStringLiteral("struct"),
    QStringLiteral("switch"),
    QStringLiteral("typedef"),
    QStringLiteral("union"),
    QStringLiteral("unsigned"),
    QStringLiteral("void"),
    QStringLiteral("volatile"),
    QStringLiteral("while")
};

CDebugHighlighter::CDebugHighlighter(SourcesWidget *sources, QPlainTextEdit *source)
    : QSyntaxHighlighter(source), m_sources(sources) {
    setDocument(source->document());
}

void CDebugHighlighter::highlightBlock(const QString &text) {
    enum class ParseState {
        Default = -1,
        StringLiteral,
        PreprocessorString,
        PreprocessorDirective,
        PreprocessorInstruction,
        PreprocessorInclude,
        PreprocessorIf,
        PreprocessorOther,
        CharacterLiteral,
        MultilineComment,
        Comment,
        NumberLiteral,
        PreprocessorFilename,
        Identifier,
        Escape,
        EscapeOctal,
        EscapeHexadecimal
    } state = static_cast<ParseState>(previousBlockState()), baseState = state;
    assert(state == ParseState::Default || state == ParseState::MultilineComment);
    int start = 0, literalStart = 0, literalInfo = 0;
    for (int i = 0; i <= text.length(); i++) {
        QChar c = text.data()[i];
        switch (state) {
            case ParseState::Default:
                if (c == '\"') {
                    state = ParseState::StringLiteral;
                    literalStart = i;
                } else if (c == '#' && !i) {
                    state = ParseState::PreprocessorDirective;
                } else if (c == '\'') {
                    state = ParseState::CharacterLiteral;
                    literalStart = i;
                    literalInfo = 0;
                } else if (c == '.' && i < text.length() - 1) {
                    c = text[i + 1];
                    if (c >= '0' && c <= '9') {
                        state = ParseState::NumberLiteral;
                        literalInfo = true;
                    }
                } else if (c == '/' && i < text.length() - 1) {
                    c = text[i + 1];
                    if (c == '*') {
                        state = ParseState::MultilineComment;
                    } else if (c == '/') {
                        state = ParseState::Comment;
                    }
                } else if (c >= '0' && c <= '9') {
                    state = ParseState::NumberLiteral;
                    literalInfo = false;
                } else if ((c >='A' && c <= 'Z') || c == '_' || (c >= 'a' && c <= 'z')) {
                    state = ParseState::Identifier;
                }
                if (i == text.length() || state != ParseState::Default) {
                    setFormat(start, i - start, m_sources->m_defaultFormat);
                    start = i;
                }
                break;
            case ParseState::StringLiteral:
            case ParseState::PreprocessorString:
            case ParseState::CharacterLiteral:
            case ParseState::PreprocessorFilename:
                if (i == text.length()) {
                    setFormat(literalStart, i - literalStart, m_sources->m_errorFormat);
                } else if ((c == '\"' && (state == ParseState::StringLiteral ||
                                          state == ParseState::PreprocessorString)) ||
                           (c == '\'' && state == ParseState::CharacterLiteral) ||
                           (c == '>' && state == ParseState::PreprocessorFilename)) {
                    setFormat(start, i + 1 - start, m_sources->m_literalFormat);
                    if (state == ParseState::CharacterLiteral && literalInfo != 1) {
                        setFormat(literalStart, i + 1 - literalStart,
                                  m_sources->m_errorFormat);
                    }
                    start = i + 1;
                    if (state == ParseState::StringLiteral || state == ParseState::CharacterLiteral) {
                        state = ParseState::Default;
                    } else {
                        state = ParseState::PreprocessorOther;
                    }
                } else {
                    literalInfo++;
                    if (c == '\\' &&
                        state != ParseState::PreprocessorString &&
                        state != ParseState::PreprocessorFilename) {
                        setFormat(start, i - start, m_sources->m_literalFormat);
                        start = i;
                        baseState = state;
                        state = ParseState::Escape;
                    }
                }
                break;
            case ParseState::PreprocessorDirective:
                if (i == text.length() || (c != ' ' && c != '\t')) {
                    state = ParseState::PreprocessorInstruction;
                    start = i;
                } else {
                    break;
                }
                [[fallthrough]];
            case ParseState::PreprocessorInstruction:
                if (c == ' ' || c == '\t') {
                    QStringRef instruction = text.midRef(start, i - start);
                    if (instruction == QStringLiteral("include")) {
                        state = ParseState::PreprocessorInclude;
                    } else if (instruction == QStringLiteral("if")) {
                        state = ParseState::PreprocessorIf;
                    } else {
                        state = ParseState::PreprocessorOther;
                    }
                    start = 0;
                }
                if (i != text.length()) {
                    break;
                }
                start = 0;
                [[fallthrough]];
            case ParseState::PreprocessorInclude:
            case ParseState::PreprocessorIf:
            case ParseState::PreprocessorOther:
                if (i == text.length() || c == '\"' ||
                    (c == '<' && state == ParseState::PreprocessorInclude)) {
                    setFormat(start, i - start, m_sources->m_preprocessorFormat);
                    state = c == '\"' ? ParseState::PreprocessorString
                                      : ParseState::PreprocessorFilename;
                    start = literalStart = i;
                } else if (c != ' ' && c != '\t') {
                    state = ParseState::PreprocessorOther;
                }
                break;
            case ParseState::MultilineComment:
                if (i == text.length()) {
                    setFormat(start, i - start, m_sources->m_commentFormat);
                } else if (i > start + 1 + (baseState != ParseState::MultilineComment) &&
                           c == '/' && text[i - 1] == '*') {
                    setFormat(start, i + 1 - start, m_sources->m_commentFormat);
                    start = i + 1;
                    state = baseState = ParseState::Default;
                }
                break;
            case ParseState::Comment:
                i = text.length();
                setFormat(start, i - start, m_sources->m_commentFormat);
                break;
            case ParseState::NumberLiteral:
            case ParseState::Identifier:
                if (i == text.length() ||
                    ((c < '0' || c > '9') && (c < 'A' || c > 'Z') &&
                     c != '_' && (c < 'a' || c > 'z'))) {
                    QStringRef token = text.midRef(start, i - start);
                    if (state == ParseState::NumberLiteral) {
                        if (c == '.' || ((c == '+' || c == '-') && i > start &&
                                         (text[i - 1] == 'E' || text[i - 1] == 'e'))) {
                            literalInfo = true;
                            break;
                        }
                        bool ok;
                        if (literalInfo) {
                            if (token.endsWith('f', Qt::CaseInsensitive) ||
                                token.endsWith('l', Qt::CaseInsensitive)) {
                                token.chop(1);
                            }
                            token.toFloat(&ok);
                        } else {
                            bool unsignedSuffix = false, longSuffix = false;
                            forever {
                                if (!unsignedSuffix &&
                                    token.endsWith('u', Qt::CaseInsensitive)) {
                                    token.chop(1);
                                    unsignedSuffix = true;
                                } else if (!longSuffix &&
                                           token.endsWith('l', Qt::CaseInsensitive)) {
                                    token.chop(1);
                                    longSuffix = true;
                                } else {
                                    break;
                                }
                            }
                            if (unsignedSuffix) {
                                uint value = token.toUInt(&ok, 0);
                                ok &= longSuffix || value < 1u << 24;
                            } else {
                                int value = token.toInt(&ok, 0);
                                ok &= longSuffix || (value >= -(1 << 23) &&
                                                     value < 1 << 23);
                            }
                        }
                        setFormat(start, i - start, ok ? m_sources->m_literalFormat
                                                       : m_sources->m_errorFormat);
                    } else {
                        setFormat(start, i - start,
                                  s_keywords.contains(token.toString()) ?
                                  m_sources->m_keywordFormat :
                                  m_sources->m_identifierFormat);
                    }
                    start = i--;
                    state = ParseState::Default;
                }
                break;
            case ParseState::Escape:
                if (i == text.length()) {
                    setFormat(literalStart, i - literalStart, m_sources->m_errorFormat);
                } else if (c >= QChar('0') && c <= QChar('7')) {
                    state = ParseState::EscapeOctal;
                } else if (c == QChar('x') || c == QChar('X')) {
                    state = ParseState::EscapeHexadecimal;
                } else {
                    bool error = c != QChar('\'') && c != QChar('\"') && c != QChar('\?') && c != QChar('\\') &&
                        c != QChar('a') && c != QChar('b') && c != QChar('e') && c != QChar('f') &&
                        c != QChar('n') && c != QChar('r') && c != QChar('t') && c != QChar('v');
                    setFormat(start, i + 1 - start, error ? m_sources->m_errorFormat : m_sources->m_escapeFormat);
                    start = i + 1;
                    state = baseState;
                    baseState = ParseState::Default;
                }
                break;
            case ParseState::EscapeOctal:
                if (i == text.length()) {
                    setFormat(literalStart, i - literalStart, m_sources->m_errorFormat);
                } else if (i == start + 4 || c < QChar('0') || c > QChar('9')) {
                    setFormat(start, i - start, m_sources->m_escapeFormat);
                    start = i--;
                    state = baseState;
                    baseState = ParseState::Default;
                }
                break;
            case ParseState::EscapeHexadecimal:
                if (i == text.length()) {
                    setFormat(literalStart, i - literalStart, m_sources->m_errorFormat);
                } else if ((c < QChar('0') || c > QChar('9')) &&
                           (c < QChar('a') || c > QChar('f')) &&
                           (c < QChar('A') || c > QChar('F'))) {
                    setFormat(start, i - start, i == start + 2 ? m_sources->m_errorFormat : m_sources->m_escapeFormat);
                    start = i--;
                    state = baseState;
                    baseState = ParseState::Default;
                }
                break;
        }
    }
    setCurrentBlockState(static_cast<int>(state == ParseState::MultilineComment ? state : ParseState::Default));
}
