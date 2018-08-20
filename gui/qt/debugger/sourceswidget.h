#ifndef SOURCESWIDGET_H
#define SOURCESWIDGET_H

class CDebugHighlighter;

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtGui/QFont>
#include <QtGui/QTextCursor>
#include <QtWidgets/QWidget>
class QPushButton;
class QTabWidget;

class SourcesWidget : public QWidget {
    Q_OBJECT

public:
    SourcesWidget(QWidget * = Q_NULLPTR);

    void setSourceFont(const QFont &font);

signals:
    void breakToggled(quint32 addr, bool gui = true);

public slots:
    void selectDebugFile();
    void updatePC(quint32 pc);
    void updateAddr(quint32 addr);

private slots:
    void sourceContextMenu(const QPoint &pos);

private:
    void clear();
    void update(quint32 addr, bool pc);
    void updateAll();

    friend CDebugHighlighter;
    QPushButton *m_button;
    QTabWidget *m_tabs;
    QTextCharFormat m_defaultFormat, m_operatorFormat, m_literalFormat, m_escapeFormat,
        m_preprocessorFormat, m_commentFormat, m_keywordFormat, m_identifierFormat, m_errorFormat;

    struct Line {
        quint32 num, addr;
    };
    struct Symbol {
        qint32 value = 0, type = 0;
        quint32 tag = 0;
        quint8 kind = 0, length = 0;
        QVector<quint32> dims;
    };
    struct Record;
    struct Scope {
        QMultiHash<quint32, Symbol> symbols;
        QMultiHash<quint32, Record> records;
    };
    struct Record : Scope {
        quint32 size = 0;
    };
    struct FunctionBase {
        QVector<Line> lines;
    };
    struct Function : FunctionBase, Scope {
    };
    struct SourceBase {
        quint32 startAddr, endAddr;
    };
    struct Source : SourceBase, Scope {
        QVector<Function> functionList;
        QMultiHash<quint32, int> functionMap;
    };
    QVector<Source> m_sources;
    QStringList m_stringList;
    QHash<QString, quint32> m_stringMap;
#ifndef NDEBUG
    friend QDebug operator<<(QDebug debug, const Line &line);
    friend QDebug operator<<(QDebug debug, const Symbol &line);
    friend QDebug operator<<(QDebug debug, const Record &line);
    friend QDebug operator<<(QDebug debug, const Function &line);
    friend QDebug operator<<(QDebug debug, const Source &line);
#endif
};

#endif
