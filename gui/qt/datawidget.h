#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QtWidgets/QPlainTextEdit>
#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QTextCharFormat>
#include <QtCore/QRegularExpression>
#include <QtCore/QObject>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
    class QPaintEvent;
    class QResizeEvent;
    class QSize;
    class QWidget;
QT_END_NAMESPACE

class AsmHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    AsmHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) Q_DECL_OVERRIDE;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat addressFormat;
    QTextCharFormat symbolFormat;
    QTextCharFormat watchRFormat;
    QTextCharFormat watchWFormat;
    QTextCharFormat breakPFormat;
    QTextCharFormat bytesFormat;
    QTextCharFormat mnemonicFormat;
    QTextCharFormat controlFlowFormat;
    QTextCharFormat controlFlowTargetFormat;
    QTextCharFormat hexFormat;
    QTextCharFormat decimalFormat;
    QTextCharFormat parenFormat;
    QTextCharFormat registerFormat;

    QRegularExpression labelPattern;
    QRegularExpression instructionPattern;
};

class DataWidget : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit DataWidget(QWidget *p = Q_NULLPTR);
    void updateDarkMode();
    void clearAllHighlights();
    void updateAllHighlights();
    void addHighlight(const QColor &lightModeColor, const QColor &darkModeColor);
    void highlightCurrentLine();
    void cursorState(bool movable);
    bool labelCheck() const;
    QString getSelectedAddr() const;

signals:
    void gotoDisasmAddress(uint32_t address);
    void gotoMemoryAddress(uint32_t address);

private:
    bool moveable;
    AsmHighlighter *highlighter;
    QList<QTextEdit::ExtraSelection> highlights;
};


#endif
