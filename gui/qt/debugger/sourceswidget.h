#ifndef SOURCESWIDGET_H
#define SOURCESWIDGET_H

#include "../../core/cpu.h"

class CDebugHighlighter;

#include <QtCore/QAbstractItemModel>
#include <QtCore/QMultiHash>
#include <QtCore/QStringList>
#include <QtCore/QVector>
#include <QtGui/QTextCharFormat>
#include <QtWidgets/QWidget>
QT_BEGIN_NAMESPACE
class QFont;
class QPushButton;
class QTabWidget;
class QTreeView;
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

class SourcesWidget : public QWidget {
    Q_OBJECT

public:
    SourcesWidget(QWidget * = Q_NULLPTR);

    void setSourceFont(const QFont &font);

signals:
    void runUntilTriggered(quint32 addr);
    void breakToggled(quint32 addr, bool gui = true);

public slots:
    void selectDebugFile();
    void updatePC(quint32 pc);
    void updateAddr(quint32 addr, unsigned type, bool state);

private slots:
    void sourceContextMenu(const QPoint &pos);

private:
    void updateFormats();

    friend CDebugHighlighter;
    class VariableModel;
    class GlobalModel;
    class StackModel;

    QTabWidget *m_tabs;
    //QTextCursor m_pcCursor;
    //QBlockFormat m_pcFormat;
    QTreeView *m_global, *m_stack;
    GlobalModel *m_globalModel;
    StackModel *m_stackModel;
    QTextCharFormat m_defaultFormat, m_operatorFormat, m_literalFormat, m_escapeFormat,
        m_preprocessorFormat, m_commentFormat, m_keywordFormat, m_identifierFormat, m_errorFormat;

    constexpr static quint32 s_unnamed = 0;
    struct Line {
        quint32 num, addr;
    };
    enum class SymbolKind : quint8 {
        Unknown       = 0,
        Function      = 2,
        StructField   = 8,
        UnionField    = 11,
        BitField      = 18,
        StackSlot     = 65, // can be Constant, Uninitialized or Variable
        Constant      = 69,
        Uninitialized = 83, // can be Constant or Variable
        Variable      = 84,
    };
    struct Symbol {
        quint32 name = s_unnamed, alias = s_unnamed;
        qint32 value = 0, type = 0;
        quint32 tag = 0;
        SymbolKind kind = SymbolKind::Unknown;
        quint8 length = 0;
        QList<quint32> dims;
    };
    struct Record;
    struct Scope {
        QVector<Symbol> symbolList;
        QVector<Record> recordList;
        QMultiHash<quint32, int> symbolMap;
        QMultiHash<quint32, int> recordMap;
    };
    struct Context {
        const Scope *local, *global;
    };
    struct Record : Scope {
        quint32 name, size;
    };
    struct FunctionBase {
        QVector<Line> lines;
    };
    struct Function : FunctionBase, Scope {
        quint32 name;
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
#ifdef QT_DEBUG
    friend QDebug operator<<(QDebug debug, const Line &line);
    friend QDebug operator<<(QDebug debug, const SymbolKind &kind);
    friend QDebug operator<<(QDebug debug, const Context &context);
    friend QDebug operator<<(QDebug debug, const Symbol &symbol);
    friend QDebug operator<<(QDebug debug, const Record &record);
    friend QDebug operator<<(QDebug debug, const Function &function);
    friend QDebug operator<<(QDebug debug, const Source &sourc);
#endif

    bool lookupAddr(quint32 addr, int *sourceIndex = nullptr,
                    int *functionIndex = nullptr, int *lineIndex = nullptr) const;
};

#endif
