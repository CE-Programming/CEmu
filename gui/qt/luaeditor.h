/* Inspired by the Qt Code Editor Example - BSD License */

#ifndef LUAEDITOR_H
#define LUAEDITOR_H

#include <QtCore/QRegularExpression>
#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QTextCharFormat>
#include <QtWidgets/QPlainTextEdit>

QT_BEGIN_NAMESPACE
class QCompleter;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QPaintEvent;
class QResizeEvent;
class QSize;
class QStringListModel;
class QTextDocument;
class QWidget;
QT_END_NAMESPACE

class LuaHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit LuaHighlighter(QTextDocument *parent = nullptr);
    void updateTheme(bool darkMode);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    void rebuildRules(bool darkMode);

    QVector<HighlightingRule> highlightingRules;
    QTextCharFormat keywordFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat builtinFormat;
    QTextCharFormat literalFormat;
    QTextCharFormat cemuGlobalsFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat functionFormat;
};

class LuaEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit LuaEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event) const;
    int lineNumberAreaWidth() const;
    void updateDarkMode();

protected:
    void changeEvent(QEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void insertCompletion(const QString &completion);
    void updateLineNumberAreaWidth(int newBlockCount = 0);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    void indentNewLine();
    bool isCompletionSuppressed() const;
    void showCompletion(bool explicitRequest);
    void updateEditorMetrics();

    QWidget *lineNumberArea = nullptr;
    LuaHighlighter *highlighter = nullptr;
    QCompleter *completer = nullptr;
    QStringListModel *completionModel = nullptr;
};

class LuaLineNumberArea : public QWidget
{
public:
    explicit LuaLineNumberArea(LuaEditor *editor) : QWidget(editor), luaEditor(editor) {}

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override { luaEditor->lineNumberAreaPaintEvent(event); }

private:
    LuaEditor *luaEditor;
};

#endif
