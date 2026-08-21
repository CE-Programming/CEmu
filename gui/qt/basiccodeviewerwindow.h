#ifndef BASICCODEVIEWERWINDOW_H
#define BASICCODEVIEWERWINDOW_H

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QRegularExpression>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtCore/QVector>
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
    class QMenu;
    class QMouseEvent;
    class QKeyEvent;
    class QContextMenuEvent;
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
    QTextCharFormat commentFormat;
    QTextCharFormat otherFormat;
};


class BasicEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    BasicEditor(QWidget *parent = nullptr);

    void updateDarkMode();
    void lineNumberAreaPaintEvent(QPaintEvent *event) const;
    void lineNumberAreaMousePressEvent(QMouseEvent *event);
    int lineNumberAreaWidth();
    void toggleHighlight();
    void addCodeContextActions(QMenu *menu, const QPoint &pos);
    void setBreakpointEditingEnabled(bool enabled);
    void setBreakpoints(const QSet<int> &lines);
    bool hasBreakpoint(int line) const;

signals:
    void breakpointToggled(int line, bool enabled);

protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &, int);

private:
    int blockNumberAtY(int y) const;
    QString gotoLabelAt(const QPoint &pos) const;
    QStringList labels() const;
    bool goToLabel(const QString &label);
    bool goBack();

    QWidget *lineNumberArea;
    BasicHighlighter *highlighter;
    QSet<int> breakpoints;
    QVector<int> navigationHistory;
    bool breakpointEditingEnabled = false;
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
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE {
        basicEditor->lineNumberAreaMousePressEvent(event);
    }

private:
    virtual void anchor();
    BasicEditor *basicEditor;
};


class BasicCodeViewerWindow : public QDialog {
    Q_OBJECT

public:
    explicit BasicCodeViewerWindow(QWidget *p = Q_NULLPTR, bool doHighlight = true, bool doWrap = false, bool doFormat = false);
    void setVariableName(const QString &name);
    void setOriginalCode(const QString &code, bool reindent);
    void setArchived(bool archived);
    void setEditable(bool editable);
    void saveStarted();
    ~BasicCodeViewerWindow() override;

signals:
    void saveRequested(const QString &code, bool archived);

public slots:
    void saveFinished(bool success);

private slots:
    void toggleHighlight();
    void toggleWrap();
    void toggleFormat();
    void save();

private:
    void showCode();
    void updateSaveButton();

    Ui::BasicCodeViewerWindow *ui;
    QString m_variableName;
    QString m_originalCode;
    QString m_formattedCode;
    bool m_showingHighlighted = true;
    bool m_showingWrapped = false;
    bool m_showingFormatted = false;
    bool m_savePending = false;
    bool m_editable = false;
    bool m_canReformat = false;
    bool m_originalArchived = false;
    bool hasCodeYet = false;
};

#endif
