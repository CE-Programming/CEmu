#ifndef CHIGHLIGHTER_H
#define CHIGHLIGHTER_H

class SourcesWidget;

#include <QtCore/QRegularExpression>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtGui/QSyntaxHighlighter>
class QPlainTextEdit;

class CDebugHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    CDebugHighlighter(SourcesWidget *sources, QPlainTextEdit *source = Q_NULLPTR);

protected:
    void highlightBlock(const QString &text) override;

private:
    SourcesWidget *m_sources;
    const static QSet<QString> s_keywords;
};

#endif
