#ifndef BASICCODEVIEWERWINDOW_H
#define BASICCODEVIEWERWINDOW_H

#include <QtCore/QString>
#include <QtCore/QRegularExpression>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtWidgets/QDialog>
#include <QtWidgets/QPlainTextEdit>
#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QTextCharFormat>

namespace Ui { class BasicCodeViewerWindow; }

/* Inspired from a code in the Qt Examples - BSD License */

QT_BEGIN_NAMESPACE
    class QPaintEvent;
    class QResizeEvent;
    class QSize;
    class QWidget;
    class QTextDocument;
QT_END_NAMESPACE


class BasicHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    BasicHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) Q_DECL_OVERRIDE;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat variableFormat;
    QTextCharFormat listFormat;
    QTextCharFormat builtinFormat;
    QTextCharFormat constFormat;
    QTextCharFormat labelFormat;
    QTextCharFormat prgmFormat;
    QTextCharFormat delvarFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat otherFormat;
};


class BasicEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    BasicEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    void toggleHighlight();

protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);

private:
    QWidget *lineNumberArea;
    BasicHighlighter *highlighter;
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(BasicEditor *editor) : QWidget(editor) {
        basicEditor = editor;
    }

    QSize sizeHint() const Q_DECL_OVERRIDE {
        return QSize(basicEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE {
        basicEditor->lineNumberAreaPaintEvent(event);
    }

private:
    virtual void anchor();
    BasicEditor *basicEditor;
};


class BasicCodeViewerWindow : public QDialog {
    Q_OBJECT

public:
    explicit BasicCodeViewerWindow(QWidget *p = Q_NULLPTR, bool doHighlight = true, bool doWrap = false, bool doFormat = false);
    void getAndProcessCurrExecPos();
    void setVariableName(const QString &name);
    void setOriginalCode(const QString &code, bool reindent);
    ~BasicCodeViewerWindow() override;

private slots:
    void toggleHighlight();
    void toggleWrap();
    void toggleFormat();

private:
    void showCode();

    Ui::BasicCodeViewerWindow *ui;
    QString m_variableName;
    QString m_originalCode;
    QString m_formattedCode;
    bool m_showingHighlighted = true;
    bool m_showingWrapped = false;
    bool m_showingFormatted = false;
    bool hasCodeYet = false;
    QTimer* currExecTimer;
};

#endif
