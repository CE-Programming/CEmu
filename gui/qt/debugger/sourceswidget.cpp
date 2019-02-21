#include "sourceswidget.h"
#include "cdebughighlighter.h"
#include "mainwindow.h"
#include "utils.h"
#include "../../core/mem.h"

#include <QtCore/QDirIterator>
#include <QtCore/QModelIndex>
#include <QtCore/QVariant>
#include <QtGui/QTextBlock>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>

#include <algorithm>
#include <iterator>
#include <utility>

struct SourcesWidget::Context {
    const Function *local;
    const Source *global;

    typedef Scope value_type;
    typedef const Scope &reference;
    typedef const Scope *pointer;
    typedef qint8 difference_type;
    typedef quint8 size_type;
    struct const_iterator {
        typedef Scope value_type;
        typedef const Scope &reference;
        typedef const Scope *pointer;
        typedef qint8 difference_type;
        typedef std::random_access_iterator_tag iterator_category;

        const_iterator(const Context *context = nullptr, difference_type i = 0) : context(context), i(i) {}
        reference operator*() const { switch (i) { case 0: return *context->local; default: return *context->global; } }
        pointer operator->() const { return &**this; }
        reference operator[](difference_type n) { return *(*this + n); }
        const_iterator &operator++() { ++i; return *this; }
        const_iterator operator++(int) { const_iterator temp(*this); ++*this; return temp; }
        const_iterator &operator--() { ++i; return *this; }
        const_iterator operator--(int) { const_iterator temp(*this); ++*this; return temp; }
        const_iterator &operator+=(difference_type n) { i += n; return *this; }
        friend const_iterator operator+(const_iterator iter, difference_type n) { return iter += n; }
        friend const_iterator operator+(difference_type n, const_iterator iter) { return iter += n; }
        const_iterator &operator-=(difference_type n) { i -= n; return *this; }
        friend const_iterator operator-(const_iterator iter, difference_type n) { return iter -= n; }
        difference_type operator-(const_iterator iter) const { return i - iter.i; }
        bool operator==(const_iterator iter) const { return context == iter.context && i == iter.i; }
        bool operator!=(const_iterator iter) const { return !(*this == iter); }
        bool operator<(const_iterator iter) const { return i < iter.i; }
        bool operator>(const_iterator iter) const { return i > iter.i; }
        bool operator<=(const_iterator iter) const { return i <= iter.i; }
        bool operator>=(const_iterator iter) const { return i >= iter.i; }

    private:
        const Context *context;
        difference_type i;
    };
    typedef const_iterator iterator;

    size_type size() const { return !!local + !!global; }
    static size_type max_size() { return 2; }
    const_iterator cbegin() const { return const_iterator(this, max_size() - size()); }
    const_iterator cend() const { return const_iterator(this, max_size()); }
    iterator begin() const { return cbegin(); }
    iterator end() const { return cend(); }
};


#ifdef QT_DEBUG
#include <QtTest/QAbstractItemModelTester>

QDebug operator<<(QDebug debug, const SourcesWidget::Line &line) {
    QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "Line(num=" << line.num << ", addr=" << QString::number(line.addr, 16) << ')';
    return debug.maybeSpace();
}
QDebug operator<<(QDebug debug, const SourcesWidget::SymbolKind &kind) {
    QDebugStateSaver saver(debug);
    switch (kind) {
        case SourcesWidget::SymbolKind::Function:
            debug << "SymbolKind::Function";
            break;
        case SourcesWidget::SymbolKind::StaticFunction:
            debug << "SymbolKind::StaticFunction";
            break;
        case SourcesWidget::SymbolKind::StructField:
            debug << "SymbolKind::StructField";
            break;
        case SourcesWidget::SymbolKind::UnionField:
            debug << "SymbolKind::UnionField";
            break;
        case SourcesWidget::SymbolKind::BitField:
            debug << "SymbolKind::BitField";
            break;
        case SourcesWidget::SymbolKind::StackSlot:
            debug << "SymbolKind::StackSlot";
            break;
        case SourcesWidget::SymbolKind::Constant:
            debug << "SymbolKind::Constant";
            break;
        case SourcesWidget::SymbolKind::Uninitialized:
            debug << "SymbolKind::Uninitialized";
            break;
        case SourcesWidget::SymbolKind::Variable:
            debug << "SymbolKind::Variable";
            break;
        default:
            debug.nospace() << "SymbolKind(" << quint8(kind) << ')';
            break;
    }
    return debug.maybeSpace();
}
QDebug operator<<(QDebug debug, const SourcesWidget::Context &context) {
    QDebugStateSaver saver(debug);
    debug.nospace() << "Context(local=" << context.local << ", global=" << context.global << ')';
    return debug.maybeSpace();
}
QDebug operator<<(QDebug debug, const SourcesWidget::Symbol &symbol) {
    QDebugStateSaver saver(debug);
    debug.nospace() << "Symbol(name=" << symbol.name << ", alias=" << symbol.alias << ", value=" << QString::number(symbol.value, symbol.kind == SourcesWidget::SymbolKind::StackSlot ? 10 : 16) << ", type=" << symbol.type << ", tag=" << symbol.tag << ", kind=" << symbol.kind << ", length=" << symbol.length << ", dims=" << symbol.dims << ')';
    return debug.maybeSpace();
}
QDebug operator<<(QDebug debug, const SourcesWidget::Record &record) {
    QDebugStateSaver saver(debug);
    debug << "Record(symbolList=" << record.symbolList << ", symbolMap=" << record.symbolMap << ", recordList=" << record.recordList << ", recordMap=" << record.recordMap << ", size=" << record.size << ')';
    return debug.maybeSpace();
}
QDebug operator<<(QDebug debug, const SourcesWidget::Function &function) {
    QDebugStateSaver saver(debug);
    debug << "Function(symbolList=" << function.symbolList << ", symbolMap=" << function.symbolMap << ", recordList=" << function.recordList << ", recordMap=" << function.recordMap << ", lines=" << function.lines << ')';
    return debug.maybeSpace();
}
QDebug operator<<(QDebug debug, const SourcesWidget::Source &source) {
    QDebugStateSaver saver(debug);
    debug << "Source(symbolList=" << source.symbolList << ", symbolMap=" << source.symbolMap << ", recordList=" << source.recordList << ", recordMap=" << source.recordMap << ", functionList=" << source.functionList << ", functionMap=" << source.functionMap << ')';
    return debug.maybeSpace();
}
#endif

class SourcesWidget::VariableModel : public QAbstractItemModel {
    void deleteVariable(int id);
protected:
    explicit VariableModel(SourcesWidget *parent) : QAbstractItemModel(parent) {}
#define using(const, field) const decltype(m_##field) &field() const {			\
        return static_cast<const SourcesWidget *>(QObject::parent())->m_##field;	\
    }
    using(const, sources)
    using(const, stringList)
    using(const, stringMap)
#undef using
    enum class VariableFlag : quint8 {
        None = 0,
        Deleted = 1 << 0,
        Changed = 1 << 1,
    };
    Q_DECLARE_FLAGS(VariableFlags, VariableFlag)
    struct Variable {
        int parent, parentIndex, childIndex;
        QList<int> children;
        Symbol symbol;
        qint32 base;
        Context context;
        QString data[2];
        VariableFlags flags;
    };
    int m_freeVariable;
    QVector<Variable> m_variables;
    QStringList m_topLevelData[2];
    QList<QList<int>> m_topLevelChildren;
    bool lookupAddr(quint32 addr, int *sourceIndex = nullptr,
                    int *functionIndex = nullptr, int *lineIndex = nullptr) const {
        return static_cast<const SourcesWidget *>(QObject::parent())->
            lookupAddr(addr, sourceIndex, functionIndex, lineIndex);
    }
    static quint32 sizeOfSymbol(const Symbol &symbol, Context context);
    QString variableTypeToString(const Variable &variable) const;
    QString variableValueToString(const Variable &variable) const;
    int createVariable(int parent, int parentIndex, int childIndex, const Symbol &symbol, Context context, qint32 base = 0, const QString &name = {});
    void createVariable(const QModelIndex &parent, const Symbol &symbol, qint32 base = 0, const QString &name = {});
    void removeTopLevels(int first, int last);
    constexpr static quintptr s_topLevelId = ~quintptr();
    constexpr static int s_topLevelParent = int(s_topLevelId);
public:
    virtual void update();
    QModelIndex parent(const QModelIndex &child) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = {}) const override;
    int childCount(const QModelIndex &parent = {}) const;
    bool hasChildren(const QModelIndex &parent = {}) const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;
};
class SourcesWidget::GlobalModel : public VariableModel {
    static int commonPrefixLength(const QStringList &paths);
public:
    explicit GlobalModel(SourcesWidget *parent) : VariableModel(parent) {}
    void init(const QStringList &paths);
};
class SourcesWidget::StackModel : public VariableModel {
    struct StackEntry {
        quint32 ix, pc, cookie[2];
        const Source *source;
        const Function *function;
    };
    QVector<StackEntry> m_stack;
public:
    explicit StackModel(SourcesWidget *parent) : VariableModel(parent) {}
    void update() override;
    void init();
};

namespace {
enum class DebugTokenKind {
    Eof,
    Alias,
    Base,
    BegFunc,
    BegRec,
    Class,
    Debug,
    Define,
    Dim,
    End,
    Endef,
    EndFunc,
    EndRec,
    File,
    Length,
    Line,
    Reg,
    Size,
    Space,
    Strings,
    Tag,
    Type,
    Value,
};

class DebugFile : public QFile {
public:
    bool success;
    DebugFile(QString name) : QFile(name), success(open(QIODevice::ReadOnly)) {}
    void setErrorString(const QString &errorString) {
        if (success) {
            success = false;
            QFile::setErrorString(errorString);
        }
    }
    quint32 readULEB128() {
        if (!success) {
            return 0;
        }
        quint32 result = 0, shift = 0;
        char next;
        do {
            if (!getChar(&next)) {
                success = false;
                return 0;
            }
            result |= (next & 0x7F) << shift;
            if (shift >= 25 && next >> (32 - shift)) {
                setErrorString(tr("Shift overflow"));
                return 0;
            }
            shift += 7;
        } while (next & 0x80);
        return result;
    }
    qint32 readSLEB128() {
        if (!success) {
            return 0;
        }
        qint32 result = 0, shift = 0;
        char next;
        do {
            if (!getChar(&next)) {
                success = false;
                return 0;
            }
            result |= (next & 0x7F) << shift;
            if (shift >= 25 && (next ^ ((qint8)next << 1 >> 7 & 0x7F)) >> (32 - shift)) {
                setErrorString(tr("Shift overflow"));
                return 0;
            }
            shift += 7;
        } while (next & 0x80);
        if (shift >= 32) {
            return result;
        }
        shift = 32 - shift;
        return result << shift >> shift;
    }
    QString readString() {
        if (!success) {
            return QString();
        }
        quint32 length = readULEB128();
        if (!length) {
            return QString("");
        }
        QByteArray result = read(length);
        if (result.isEmpty()) {
            setErrorString(tr("Invalid string"));
            return QString();
        }
        return QString::fromLocal8Bit(result);
    }
};
}

SourcesWidget::SourcesWidget(QWidget *parent) : QWidget(parent) {
    QPushButton *button = new QPushButton(tr("Select Debug File"));
    connect(button, &QPushButton::clicked, this, &SourcesWidget::selectDebugFile);
    m_tabs = new QTabWidget;
    m_tabs->setTabPosition(QTabWidget::South);
    m_global = new QTreeView;
    m_globalModel = new GlobalModel(this);
    m_global->setModel(m_globalModel);
    m_stack = new QTreeView;
    m_stackModel = new StackModel(this);
#ifdef QT_DEBUG
    new QAbstractItemModelTester(m_globalModel,
                                 QAbstractItemModelTester::FailureReportingMode::Fatal,
                                 this);
    new QAbstractItemModelTester(m_stackModel,
                                 QAbstractItemModelTester::FailureReportingMode::Fatal,
                                 this);
#endif
    m_stack->setModel(m_stackModel);
    QSplitter *treeSplitter = new QSplitter(Qt::Vertical);
    treeSplitter->addWidget(m_global);
    treeSplitter->addWidget(m_stack);
    treeSplitter->setSizes({INT_MAX, INT_MAX});
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(m_tabs);
    splitter->addWidget(treeSplitter);
    splitter->setSizes({INT_MAX, INT_MAX});
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(button);
    layout->addWidget(splitter);
    setLayout(layout);
    m_literalFormat.setForeground(QColor::fromRgb(0x008000));
    m_escapeFormat.setForeground(QColor::fromRgb(0x008000));
    m_escapeFormat.setFontWeight(QFont::Bold);
    m_preprocessorFormat.setForeground(QColor::fromRgb(0x003F3F));
    m_preprocessorFormat.setFontWeight(QFont::Bold);
    m_commentFormat.setForeground(QColor::fromRgb(0x808080));
    m_keywordFormat.setForeground(QColor::fromRgb(0x3F3FFF));
    m_keywordFormat.setFontWeight(QFont::Bold);
    m_operatorFormat.setFontWeight(QFont::Bold);
    m_errorFormat.setBackground(QColor::fromRgb(0xFF3F3F));
}

void SourcesWidget::selectDebugFile() {
    QString debugName = QFileDialog::getOpenFileName(this, tr("Open Debug File"), "/home/jacob/Programming/calc/ez80/c/debug/bin", tr("Debug File (*.dbg)"));
    if (debugName.isNull()) {
        return;
    }

    QStringList paths;
    QVector<Source> sources;
    QStringList stringList;
    QHash<QString, quint32> stringMap;
    DebugFile debugFile(debugName);
    QDir dir = QFileInfo(debugFile).dir();
    dir.cdUp();
    Line prevLine{0, 0};
    bool eof = false;
    quint32 lang = 0;
    QStack<Scope *> scopes;
    scopes.reserve(3);
    bool inFunction = false;
    Symbol *curSymbol = Q_NULLPTR;
    while (debugFile.success && !debugFile.atEnd()) {
        switch (DebugTokenKind(debugFile.readULEB128())) {
            case DebugTokenKind::Eof: {
                if (!debugFile.atEnd() || !scopes.isEmpty()) {
                    debugFile.setErrorString(tr("Unexpected EOF token"));
                    break;
                }
                eof = true;
                break;
            }
            case DebugTokenKind::Alias: {
                if (!curSymbol) {
                    debugFile.setErrorString(tr("Unexpected alias token"));
                    break;
                }
                Scope &scope = *scopes.top();
                curSymbol->alias = debugFile.readULEB128();
                scope.symbolMap.insert(curSymbol->alias, scope.symbolList.count() - 1);
                break;
            }
            case DebugTokenKind::BegFunc: {
                if (scopes.count() != 1 || curSymbol) {
                    debugFile.setErrorString(tr("Unexpected function token"));
                    break;
                }
                Source &source = sources.last();
                QVector<Function> &functionList = source.functionList;
                QMultiHash<quint32, int> &functionMap = source.functionMap;
                int functionIndex = functionList.count();
                Function function;
                function.name = debugFile.readULEB128();
                functionMap.insert(function.name, functionIndex);
                functionMap.insert(debugFile.readULEB128(), functionIndex);
                prevLine.num += debugFile.readULEB128();
                prevLine.addr += debugFile.readULEB128();
                function.lines.append(prevLine);
                functionList.append(function);
                scopes.push(&functionList.last());
                inFunction = true;
                break;
            }
            case DebugTokenKind::BegRec: {
                if (scopes.isEmpty() || curSymbol) {
                    debugFile.setErrorString(tr("Unexpected record begin token"));
                    break;
                }
                Scope &scope = *scopes.top();
                Record record;
                record.name = debugFile.readULEB128();
                record.size = debugFile.readULEB128();
                scope.recordMap.insert(record.name, scope.recordList.count());
                scope.recordList.append(record);
                scopes.push(&scope.recordList.last());
                break;
            }
            case DebugTokenKind::Class: {
                if (!curSymbol) {
                    debugFile.setErrorString(tr("Unexpected class begin token"));
                    break;
                }
                curSymbol->kind = SymbolKind(debugFile.readULEB128());
                break;
            }
            case DebugTokenKind::Debug: {
                if (lang ? lang != debugFile.readULEB128()
                         : !(lang = debugFile.readULEB128())) {
                    debugFile.setErrorString(tr("Mixed source languages"));
                    break;
                }
                break;
            }
            case DebugTokenKind::Define: {
                if (scopes.isEmpty() || curSymbol) {
                    debugFile.setErrorString(tr("Unexpected define symbol begin token"));
                    break;
                }
                Scope &scope = *scopes.top();
                Symbol symbol;
                symbol.name = debugFile.readULEB128();
                scope.symbolMap.insert(symbol.name, scope.symbolList.count());
                scope.symbolList.append(symbol);
                curSymbol = &scope.symbolList.last();
                break;
            }
            case DebugTokenKind::Dim: {
                if (!curSymbol) {
                    debugFile.setErrorString(tr("Unexpected dimension token"));
                    break;
                }
                curSymbol->dims.append(debugFile.readULEB128());
                break;
            }
            case DebugTokenKind::End: {
                if (scopes.count() != 1 || curSymbol) {
                    debugFile.setErrorString(tr("Unexpected source file end token"));
                    break;
                }
                sources.last().endAddr = prevLine.addr += debugFile.readULEB128();
                scopes.pop();
                break;
            }
            case DebugTokenKind::Endef: {
                if (!curSymbol) {
                    debugFile.setErrorString(tr("Unexpected define symbol end token"));
                    break;
                }
                curSymbol = Q_NULLPTR;
                break;
            }
            case DebugTokenKind::EndFunc: {
                if (scopes.count() != 2 || !inFunction || curSymbol) {
                    debugFile.setErrorString(tr("Unexpected function end token"));
                    break;
                }
                debugFile.readULEB128();
                debugFile.readULEB128();
                prevLine.num += debugFile.readULEB128();
                prevLine.addr += debugFile.readULEB128();
                sources.last().functionList.last().lines.append(prevLine);
                scopes.pop();
                inFunction = false;
                break;
            }
            case DebugTokenKind::EndRec: {
                if (scopes.count() <= 1 + inFunction || curSymbol) {
                    debugFile.setErrorString(tr("Unexpected record end token"));
                    break;
                }
                debugFile.readULEB128();
                scopes.pop();
                break;
            }
            case DebugTokenKind::File: {
                if (!scopes.isEmpty()) {
                    debugFile.setErrorString(tr("Unexpected source file begin token"));
                    break;
                }
                paths << dir.path();
                QStringList components = debugFile.readString().split('\\');
                for (QStringList::iterator i = components.begin(), e = components.end();
                     debugFile.success && i != e; ) {
                    QStringList component(*i++);
                    QDirIterator matches(paths.last(), component, QDir::Readable |
                                         (i != e ? QDir::Dirs : QDir::Files));
                    if (matches.hasNext()) {
                        paths.last() = matches.next();
                    } else {
                        debugFile.setErrorString(tr("Unable to find source file \"%0\"").
                                                 arg(components.
                                                     join(QDir::separator())));
                    }
                }
                if (debugFile.success) {
                    sources.resize(sources.count() + 1);
                    sources.last().startAddr = prevLine.addr += debugFile.readULEB128();
                    prevLine.num = 0;
                    scopes.push(&sources.last());
                }
                break;
            }
            case DebugTokenKind::Length: {
                if (!curSymbol) {
                    debugFile.setErrorString(tr("Unexpected length token"));
                    break;
                }
                curSymbol->length = debugFile.readULEB128();
                break;
            }
            case DebugTokenKind::Line: {
                if (scopes.count() != 2 || !inFunction || curSymbol) {
                    debugFile.setErrorString(tr("Unexpected line token"));
                    break;
                }
                prevLine.num += debugFile.readULEB128();
                prevLine.addr += debugFile.readULEB128();
                sources.last().functionList.last().lines.append(prevLine);
                break;
            }
            case DebugTokenKind::Strings: {
                if (!scopes.isEmpty()) {
                    debugFile.setErrorString(tr("Unexpected strings token"));
                    break;
                }
                quint32 count = debugFile.readULEB128();
                stringList.reserve(count);
                stringMap.reserve(count);
                for (quint32 index = 0; debugFile.success && index != count; ++index) {
                    QString string = debugFile.readString();
                    if (string.isEmpty()) {
                        debugFile.setErrorString(tr("Invalid string in string table"));
                    } else {
                        stringList << string;
                        stringMap[string] = index;
                    }
                }
                break;
            }
            case DebugTokenKind::Tag: {
                if (!curSymbol) {
                    debugFile.setErrorString(tr("Unexpected tag token"));
                    break;
                }
                curSymbol->tag = debugFile.readULEB128();
                break;
            }
            case DebugTokenKind::Type: {
                if (!curSymbol) {
                    debugFile.setErrorString(tr("Unexpected value token"));
                    break;
                }
                curSymbol->type = debugFile.readSLEB128();
                break;
            }
            case DebugTokenKind::Value: {
                if (!curSymbol) {
                    debugFile.setErrorString(tr("Unexpected value token"));
                    break;
                }
                curSymbol->value = debugFile.readSLEB128();
                break;
            }
            default: debugFile.setErrorString(tr("Unknown debug token kind"));
        }
    }
    if (sources.isEmpty()) {
        debugFile.setErrorString(tr("No source files"));
    }
    if (!eof) {
        debugFile.setErrorString(tr("Missing EOF token"));
    }
    if (stringList.value(lang - 1) != "C") {
        debugFile.setErrorString(tr("Unknown source language \"%0\"").
                                 arg(stringList.value(lang - 1)));
    }
    if (debugFile.success) {
        for (int i = 0; i != m_tabs->count(); i++) {
            m_tabs->widget(i)->deleteLater();
        }
        m_tabs->clear();
        for (int i = 0; i != sources.count(); i++) {
            QFile sourceFile(paths.at(i));
            if (sourceFile.open(QIODevice::ReadOnly)) {
                QPlainTextEdit *sourceView = new QPlainTextEdit(sourceFile.readAll());
                sourceView->setObjectName(QStringLiteral("sourceView%0").arg(i));
                sourceView->setReadOnly(true);
                sourceView->setCenterOnScroll(true);
                sourceView->setContextMenuPolicy(Qt::CustomContextMenu);
                connect(sourceView, &QWidget::customContextMenuRequested,
                        this, &SourcesWidget::sourceContextMenu);
                m_tabs->addTab(sourceView, QFileInfo(sourceFile).baseName());
                new CDebugHighlighter(this, sourceView);
            }
        }
        m_sources = std::move(sources);
        m_stringList = std::move(stringList);
        m_stringMap = std::move(stringMap);
        m_globalModel->init(paths);
        m_stackModel->init();
    } else {
        QMessageBox dialog(QMessageBox::Critical,
                           findParent<MainWindow *>(this)->MSG_ERROR,
                           tr("Error loading debug file:"), QMessageBox::Ok, this);
        dialog.setInformativeText(debugFile.errorString());
        dialog.exec();
    }
}

bool SourcesWidget::lookupAddr(quint32 addr, int *sourceIndex, int *functionIndex,
                               int *lineIndex) const {
    if (!sourceIndex && !functionIndex && !lineIndex) {
        return true;
    }
    const SourceBase sourceKey{addr, addr};
    auto source = std::upper_bound(m_sources.begin(), m_sources.end(), sourceKey,
                                   [](const SourceBase &first,
                                      const SourceBase &second) {
                                       return first.endAddr < second.startAddr;
                                   });
    if (source == m_sources.begin() ||
        addr < (--source)->startAddr || addr >= source->endAddr) {
        return false;
    }
    if (sourceIndex) {
        *sourceIndex = std::distance(m_sources.begin(), source);
        if (!functionIndex && !lineIndex) {
            return true;
        }
    }
    const Line lineKey{0, addr};
    const FunctionBase functionKey{{lineKey}};
    auto function = std::upper_bound(source->functionList.begin(),
                                     source->functionList.end(), functionKey,
                                     [](const FunctionBase &first,
                                        const FunctionBase &second) {
                                         return first.lines.last().addr <
                                             second.lines.first().addr;
                                     });
    if (function == source->functionList.begin() ||
        addr < (--function)->lines.first().addr ||
        addr >= function->lines.last().addr) {
        return false;
    }
    if (functionIndex) {
        *functionIndex = std::distance(source->functionList.begin(), function);
        if (!lineIndex) {
            return true;
        }
    }
    auto line =
        std::prev(std::upper_bound(std::next(function->lines.begin()),
                                   std::prev(function->lines.end()), lineKey,
                                   [](const Line &first, const Line &second) {
                                       return first.addr < second.addr;
                                   }));
    *lineIndex = std::distance(function->lines.begin(), line);
    return true;
}

void SourcesWidget::updateFormats() {
    for (QSyntaxHighlighter *highlighter :
             m_tabs->findChildren<QSyntaxHighlighter *>()) {
        highlighter->rehighlight();
    }
}

void SourcesWidget::updatePC(quint32 pc) {
    m_globalModel->update();
    m_stackModel->update();
    for (auto &sourceView : m_tabs->findChildren<QPlainTextEdit *>()) {
        sourceView->setExtraSelections({});
    }
    int sourceIndex, functionIndex, lineIndex;
    if (!lookupAddr(pc, &sourceIndex, &functionIndex, &lineIndex)) {
        return;
    }
    QPlainTextEdit *sourceView = m_tabs->findChild<QPlainTextEdit *>(QStringLiteral("sourceView%0").arg(sourceIndex));
    QTextBlock block = sourceView->document()->findBlockByNumber(m_sources.at(sourceIndex).functionList.at(functionIndex).lines.at(lineIndex).num - 1);
    if (!block.isValid()) {
        return;
    }
    QTextEdit::ExtraSelection pcSelection;
    pcSelection.cursor = QTextCursor(block);
    if (pcSelection.cursor.movePosition(QTextCursor::EndOfWord)) {
        pcSelection.cursor.movePosition(QTextCursor::StartOfBlock);
    } else {
        pcSelection.cursor.movePosition(QTextCursor::NextWord);
    }
    pcSelection.cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    pcSelection.cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
    pcSelection.cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    pcSelection.format.setBackground(QColor::fromRgb(0xFFFF99));
    sourceView->setExtraSelections({pcSelection});
    m_tabs->setCurrentWidget(sourceView);
}

void SourcesWidget::updateAddr(quint32 addr, unsigned type, bool state) {
    (void)type; // TODO: implement
    int sourceIndex, functionIndex, lineIndex;
    if (!lookupAddr(addr, &sourceIndex, &functionIndex, &lineIndex)) {
        return;
    }
    auto sourceView = m_tabs->findChild<QPlainTextEdit *>(QStringLiteral("sourceView%0").arg(sourceIndex));
    QTextBlock block = sourceView->document()->findBlockByNumber(m_sources.at(sourceIndex).functionList.at(functionIndex).lines.at(lineIndex).num - 1);
    if (!block.isValid()) {
        return;
    }
    // TODO: This is obviously incorrect, but who cares for now.
    QTextBlockFormat format;
    if (state) {
        format.setBackground(QColor::fromRgb(0xFF6050));
    }
    QTextCursor(block).setBlockFormat(format);
}

void SourcesWidget::sourceContextMenu(const QPoint &pos) {
    QPlainTextEdit *sourceView = static_cast<QPlainTextEdit *>(m_tabs->currentWidget());
    const QVector<Function> &functions = m_sources.at(sourceView->objectName().midRef(QStringLiteral("sourceView").count()).toInt()).functionList;
    quint32 num = sourceView->cursorForPosition(pos).block().blockNumber() + 1;
    const Line lineKey{num, 0};
    const FunctionBase functionKey{{lineKey}};
    auto function = std::upper_bound(functions.begin(), functions.end(),
                                     functionKey,
                                     [](const FunctionBase &first,
                                        const FunctionBase &second) {
                                         return first.lines.last().num <
                                             second.lines.first().num;
                                     });
    if (function == functions.begin()) {
        return;
    }
    const QVector<Line> &lines = (--function)->lines;
    if (lineKey.num < lines.first().num || lineKey.num > lines.last().num) {
        return;
    }
    auto line =
        std::prev(std::upper_bound(std::next(lines.begin()), std::prev(lines.end()),
                                   lineKey,
                                   [](const Line &first, const Line &second) {
                                       return first.num < second.num;
                                   }));

    QMenu menu;
    QAction runUntilAction(tr("Run Until"));
    menu.addAction(&runUntilAction);
    menu.addSeparator();
    QAction toggleBreakAction(tr("Toggle Breakpoint"));
    menu.addAction(&toggleBreakAction);

    QAction *action = menu.exec(sourceView->mapToGlobal(pos));
    if (action == &runUntilAction) {
        emit runUntilTriggered(line->addr);
    } else if (action == &toggleBreakAction) {
        emit breakToggled(line->addr);
    }
}

void SourcesWidget::setSourceFont(const QFont &font) {
    for (auto *format : { &m_defaultFormat, &m_operatorFormat, &m_literalFormat, &m_escapeFormat,
                          &m_preprocessorFormat, &m_commentFormat, &m_keywordFormat, &m_identifierFormat, &m_errorFormat }) {
        QFont::Weight weight = QFont::Weight(format->fontWeight());
        format->setFont(font);
        format->setFontWeight(weight);
    }
    updateFormats();
    for (auto *child : { m_global, m_stack }) {
        child->setFont(font);
    }
}

quint32 SourcesWidget::VariableModel::sizeOfSymbol(const Symbol &symbol, Context context) {
    quint32 size = 1, type = symbol.type;
    if (type & 0xE0000000u) {
        type |= 0xFFFFFFE0u;
    }
    if ((type & 0x1C) == 0xC) {
        type -= 10;
    }
    for (int dim = 0; ; ++dim) {
        switch (type >> 5 & 7) {
            case 0:
                switch (type) {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                        return size * (type - 1);
                    case 6:
                        return size * 4;
                    case 8:
                        for (auto &scope : context) {
                            auto record = scope.recordMap.find(symbol.tag);
                            if (record != scope.recordMap.end()) {
                                return size * scope.recordList.at(*record).size;
                            }
                        }
                        [[fallthrough]];
                    default:
                        return 0;
                }
            case 1:
            case 6:
                return size * 3;
            case 2:
            default:
                return 0;
            case 3:
                size *= symbol.dims.value(dim);
                break;
        }
        type = (type >> 3 & ~0x1F) | (type & 0x1F);
    }
    return 0;
}
QString SourcesWidget::VariableModel::variableTypeToString(const Variable &variable) const {
    QString name = stringList().value(variable.symbol.name - 1);
    quint32 type = variable.symbol.type;
    QString prefix, tag = "struct ", base, suffix = name;
    if (type & 0xE0000000u) {
        type |= 0xFFFFFFE0u;
    }
    if ((type & 0x1C) == 0xC) {
        base = "unsigned ";
        type -= 10;
    }
    switch (type & 0x1F) {
        case 1: base  = "void";  break;
        case 2: base += "char";  break;
        case 3: base += "short"; break;
        case 4: base += "int";   break;
        case 5: base += "long";  break;
        case 6: base  = "float"; break;
        case 8:
            for (auto &scope : variable.context) {
                auto record = scope.recordMap.find(variable.symbol.tag);
                if (record == scope.recordMap.end()) {
                    continue;
                }
                bool first = true;
                for (auto &symbol : scope.recordList.at(*record).symbolList) {
                    switch (symbol.kind) {
                        case SymbolKind::BitField:
                            if (!first && symbol.length) {
                                if (!symbol.value) {
                                    [[fallthrough]];
                        case SymbolKind::UnionField:
                                    tag = "union ";
                                }
                                [[fallthrough]];
                        case SymbolKind::StructField:
                                break;
                            }
                            if (symbol.length) {
                                first = false;
                            }
                            [[fallthrough]];
                        default:
                            continue;
                    }
                    break;
                }
                break;
            }
            base = tag + stringList().value(variable.symbol.tag - 1);
            break;
        default: base = "<unknown base type>"; break;
    }
    base += ' ';
    int dim = 0;
    for (type >>= 5; ; type >>= 3) {
        switch (type & 7) {
            default:
                base += "<unknown modifier>";
                [[fallthrough]];
            case 0:
                return prefix + base + suffix;
            case 1:
                suffix = '*' + prefix + suffix;
                prefix = "";
                break;
            case 2:
                if (suffix.startsWith('*')) {
                    suffix = '(' + prefix + suffix + ')';
                    prefix = "";
                }
                suffix += "(...)";
                break;
            case 3:
                if (suffix.startsWith('*')) {
                    suffix = '(' + prefix + suffix + ')';
                    prefix = "";
                }
                suffix += '[' + QString::number(variable.symbol.dims.value(dim++)) + ']';
                break;
            case 6:
                suffix = '*' + prefix + suffix;
                prefix = "const ";
                break;
        }
    }
}
QString SourcesWidget::VariableModel::variableValueToString(const Variable &variable) const {
    qint32 addr = variable.base + variable.symbol.value;
    bool isFloat = false, isBitField = false, isSigned = true;
    quint32 type = variable.symbol.type;
    if (type & 0xE0000000u) {
        return {};
    }
    switch (type >> 5 & 7) {
        case 0:
            if (type == 6) {
                isFloat = true;
                type = 5;
            } else if (type == 16) {
                isBitField = true;
                isSigned = false; // We have no way of knowing if it is signed :(
                type = 4;
                addr = variable.base + 3*(variable.symbol.value / 24);
            } else if ((type & ~3) == 0xC) {
                isSigned = false;
                type -= 10;
            }
            if (type >= 2 && type <= 5) {
                union {
                    quint32 u;
                    qint32 s;
                    float f;
                } value = { 0 };
                for (unsigned offset = 0; offset != type - 1; ++offset) {
                    value.u |= mem_peek_byte(addr++) << 8*offset;
                }
                if (isFloat) {
                    return QString::number(value.f);
                }
                if (isBitField) {
                    int shift = variable.symbol.value % 24, extend = 32 - variable.symbol.length;
                    if (shift < 0) {
                        shift += 24;
                    }
                    value.u <<= 8 + shift;
                    if (isSigned) {
                        return QString::number(value.s >> extend);
                    }
                    return QString::number(value.u >> unsigned(extend));
                }
                if (isSigned) {
                    if (type == 2) {
                        QString valueStr;
                        switch (value.u) {
                            case '\'': valueStr = "'\\''"; break;
                            case '\"': valueStr = "'\\\"'"; break;
                            case '\\': valueStr = "'\\\\'"; break;
                            case '\a': valueStr = "'\\a'"; break;
                            case '\b': valueStr = "'\\b'"; break;
                            case '\e': valueStr = "'\\e'"; break;
                            case '\f': valueStr = "'\\f'"; break;
                            case '\n': valueStr = "'\\n'"; break;
                            case '\r': valueStr = "'\\r'"; break;
                            case '\t': valueStr = "'\\t'"; break;
                            case '\v': valueStr = "'\\v'"; break;
                            default:
                                valueStr += '\'';
                                if (value.u >= ' ' && value.u <= '~') {
                                    valueStr += char(value.u);
                                } else {
                                    valueStr += '\\' + QString::number(value.u, 8)
                                        .rightJustified(3, '0');
                                }
                                valueStr += '\'';
                                break;
                        }
                        return valueStr;
                    }
                    int extend = 32 - 8*(type - 1);
                    return QString::number(value.s << extend >> extend);
                }
                return QString::number(value.u);
            }
            [[fallthrough]];
        default:
            return {};
        case 1:
        case 6:
            if (quint32 value = mem_peek_long(addr)) {
                int sourceIndex, functionIndex;
                if (lookupAddr(value, &sourceIndex, &functionIndex)) {
                    return '&' + stringList().value(sources().at(sourceIndex).functionList.at(functionIndex).name - 1);
                } else {
                    return "0x" + QString::number(value, 16).rightJustified(6, '0');
                }
            }
            return "NULL";
    }
}

int SourcesWidget::VariableModel::createVariable(int parent, int parentIndex, int childIndex,
                                                 const Symbol &symbol, Context context, qint32 base,
                                                 const QString &name) {
    int id = m_freeVariable;
    if (id == -1) {
        id = m_variables.count();
        m_variables.resize(id + 1);
    } else {
        m_freeVariable = m_variables[id].parent;
    }
    auto &variable = m_variables[id];
    variable.parent = parent;
    variable.parentIndex = parentIndex;
    variable.childIndex = childIndex;
    variable.symbol = symbol;
    variable.base = base;
    variable.context = context;
    variable.flags = VariableFlag::None;
    variable.data[0] = name.isNull() ? variableTypeToString(variable) : name;
    variable.data[1] = variableValueToString(variable);
    qDebug().noquote() << '\t' << stringList().value(m_variables.value(variable.parent).symbol.name - 1) << "→" << stringList().value(variable.symbol.name - 1) << variable.symbol << QString::number(variable.base, 16);
    return id;
}
void SourcesWidget::VariableModel::createVariable(const QModelIndex &parent, const Symbol &symbol,
                                                  qint32 base, const QString &name) {
    Q_ASSERT(parent.isValid() && parent.model() == this);
    auto &variable = m_variables.at(parent.internalId());
    int child = createVariable(parent.internalId(), parent.row(), variable.children.count(),
                               symbol, variable.context, base, name);
    m_variables[parent.internalId()].children << child;
}
void SourcesWidget::VariableModel::deleteVariable(int id) {
    for (int child : m_variables.at(id).children) {
        deleteVariable(child);
    }
    m_variables[id].parent = m_freeVariable;
    m_variables[id].children.clear();
    m_variables[id].flags |= VariableFlag::Deleted;
    m_freeVariable = id;
}
void SourcesWidget::VariableModel::removeTopLevels(int first, int last) {
    if (first > last) {
        return;
    }
    beginRemoveRows(QModelIndex(), first, last);
    for (int topLevel = first; topLevel <= last; ++topLevel) {
        for (int id : m_topLevelChildren.at(topLevel)) {
            deleteVariable(id);
        }
    }
    m_topLevelData[0].erase(m_topLevelData[0].begin() + first,
                            m_topLevelData[0].begin() + last + 1);
    m_topLevelData[1].erase(m_topLevelData[1].begin() + first,
                            m_topLevelData[1].begin() + last + 1);
    m_topLevelChildren.erase(m_topLevelChildren.begin() + first,
                             m_topLevelChildren.begin() + last + 1);
    endRemoveRows();
}
void SourcesWidget::VariableModel::update() {
    for (int id = 0; id < m_variables.count(); ++id) {
        auto &variable = m_variables[id];
        if (variable.flags.testFlag(VariableFlag::Deleted)) {
            continue;
        }
        if (variable.parent != s_topLevelParent) {
            auto &parent = m_variables.at(variable.parent);
            if (parent.symbol.kind > SymbolKind::StaticFunction) {
                variable.base = parent.base + parent.symbol.value;
                if ((parent.symbol.type >> 5 & 7) == 1) {
                    variable.base = mem_peek_long(variable.base);
                }
            }
        }
        QString valueStr = variableValueToString(variable);
        bool valueChanged = variable.data[1] != valueStr;
        int firstChangedColumn = 1;
        QVector<int> changedRoles;
        changedRoles.reserve(2);
        if (valueChanged) {
            variable.data[1] = valueStr;
            changedRoles << Qt::DisplayRole;
        }
        if (valueChanged != variable.flags.testFlag(VariableFlag::Changed)) {
            variable.flags.setFlag(VariableFlag::Changed, valueChanged);
            firstChangedColumn = 0;
            changedRoles << Qt::BackgroundRole;
        }
        if (!changedRoles.isEmpty()) {
            emit dataChanged(createIndex(variable.childIndex, firstChangedColumn, id),
                             createIndex(variable.childIndex, 1, id), changedRoles);
        }
        qDebug().noquote() << '\t' << stringList().value(m_variables.value(variable.parent).symbol.name - 1) << "→" << stringList().value(variable.symbol.name - 1) << variable.symbol << QString::number(variable.base, 16);
    }
}
QModelIndex SourcesWidget::VariableModel::parent(const QModelIndex &child) const {
    Q_ASSERT(!child.isValid() || child.model() == this);
    if (!child.isValid() || child.internalId() == s_topLevelId) {
        return QModelIndex();
    }
    auto &variable = m_variables.at(child.internalId());
    Q_ASSERT(variable.parent >= s_topLevelParent &&
             variable.parent < m_variables.count());
    int index = variable.parentIndex;
    if (index < 0) {
        Q_ASSERT(variable.parent == s_topLevelParent);
        index += m_topLevelChildren.count();
    }
    return createIndex(index, 0, variable.parent);
}
QModelIndex SourcesWidget::VariableModel::index(int row, int column,
                                                const QModelIndex &parent) const {
    Q_ASSERT(!parent.isValid() || parent.model() == this);
    if (row < 0 || row >= rowCount(parent) ||
        column < 0 || column >= columnCount(parent)) {
        return {};
    }
    if (!parent.isValid()) {
        return createIndex(row, column, s_topLevelId);
    }
    if (parent.column()) {
        return {};
    }
    if (parent.internalId() == s_topLevelId) {
        if (parent.row() < 0 || parent.row() >= m_topLevelChildren.count()) {
            return {};
        }
        return createIndex(row, column, m_topLevelChildren.at(parent.row()).at(row));
    }
    if (parent.internalId() >= unsigned(m_variables.count())) {
        return {};
    }
    return createIndex(row, column,
                       m_variables.at(parent.internalId()).children.at(row));
}
int SourcesWidget::VariableModel::childCount(const QModelIndex &parent) const {
    Q_ASSERT(!parent.isValid() || parent.model() == this);
    if (parent.column() || parent.internalId() == s_topLevelId) {
        return rowCount(parent);
    }
    auto &variable = m_variables.at(parent.internalId());
    switch (variable.symbol.type >> 5 & 7) {
        case 1:
        case 6:
            return (variable.symbol.type >> 5 >> 3 & 7) != 2 && variable.data[1] != "NULL";
        case 2: {
            int count = 0;
            auto &source = *variable.context.global;
            auto function = source.functionMap.find(variable.symbol.name);
            if (function != source.functionMap.end()) {
                for (auto &symbol : source.functionList.at(*function).symbolList) {
                    if (symbol.kind == SymbolKind::StackSlot) {
                        continue;
                    }
                    ++count;
                }
            }
            return count;
        }
        case 3:
            return variable.symbol.dims.value(0);
        case 0:
            if (variable.symbol.type == 8) {
                for (auto &scope : variable.context) {
                    auto record = scope.recordMap.find(variable.symbol.tag);
                    if (record != scope.recordMap.end()) {
                        return scope.recordList.at(*record).symbolList.count();
                    }
                }
            }
            [[fallthrough]];
        default:
            return 0;
    }
}
bool SourcesWidget::VariableModel::hasChildren(const QModelIndex &parent) const {
    return childCount(parent);
}
int SourcesWidget::VariableModel::rowCount(const QModelIndex &parent) const {
    Q_ASSERT(!parent.isValid() || parent.model() == this);
    if (!parent.isValid()) {
        return m_topLevelChildren.count();
    }
    if (parent.column()) {
        return {};
    }
    if (parent.internalId() == s_topLevelId) {
        return m_topLevelChildren.value(parent.row()).count();
    }
    if (parent.internalId() >= unsigned(m_variables.count())) {
        return {};
    }
    return m_variables.at(parent.internalId()).children.count();
}
int SourcesWidget::VariableModel::columnCount(const QModelIndex &parent) const {
    Q_ASSERT(!parent.isValid() || parent.model() == this);
    return 2;
}
QVariant SourcesWidget::VariableModel::data(const QModelIndex &index, int role) const {
    Q_ASSERT(!index.isValid() || index.model() == this);
    if (!index.isValid() || index.column() < 0 || index.column() >= columnCount(index)) {
        return {};
    }
    switch (role) {
        case Qt::DisplayRole:
            if (index.internalId() == s_topLevelId) {
                return m_topLevelData[index.column()].at(index.row());
            }
            return m_variables.at(index.internalId()).data[index.column()];
        case Qt::TextAlignmentRole:
            return int((index.column() ? Qt::AlignLeft : Qt::AlignRight) |
                       Qt::AlignVCenter);
        case Qt::BackgroundRole:
            if (index.internalId() != s_topLevelId && index.column() == 1 &&
                m_variables.at(index.internalId()).flags.testFlag(VariableFlag::Changed)) {
                return QColor::fromRgb(0xFFFF99);
            }
            break;
    }
    return {};
}
Qt::ItemFlags SourcesWidget::VariableModel::flags(const QModelIndex &index) const {
    Q_ASSERT(!index.isValid() || index.model() == this);
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (index.column()) {
        flags |= Qt::ItemNeverHasChildren;
    } else if (index.isValid() && index.internalId() != s_topLevelId) {
        qint32 type = m_variables.at(index.internalId()).symbol.type;
        if (!(type & ~0x1F) && type != 8) {
            flags |= Qt::ItemNeverHasChildren;
        }
    }
    return flags;
}
bool SourcesWidget::VariableModel::canFetchMore(const QModelIndex &parent) const {
    return rowCount(parent) < childCount(parent);
}
void SourcesWidget::VariableModel::fetchMore(const QModelIndex &parent) {
    Q_ASSERT(!parent.isValid() || parent.model() == this);
    if (!parent.isValid()) {
        return;
    }
    auto &variable = m_variables[parent.internalId()];
    auto symbol = variable.symbol;
    qint32 addr = variable.base + symbol.value;
    symbol.value = 0;
    if (symbol.type & 0xE0000000u) {
        symbol.type |= 0xFFFFFFE0u;
    }
    if (symbol.kind == SymbolKind::StackSlot) {
        symbol.kind = SymbolKind::Uninitialized;
    }
    QString name = variable.data[0];
    if (variable.parent == s_topLevelParent ||
        m_variables.at(variable.parent).symbol.kind <= SymbolKind::StaticFunction) {
        name = stringList().value(variable.symbol.name - 1);
    }
    if (symbol.type & ~0x1F) {
        quint8 mod = symbol.type >> 5 & 7;
        symbol.type = (symbol.type >> 3 & ~0x1F) | (symbol.type & 0x1F);
        if (mod == 1 || mod == 6) {
            if ((addr = mem_peek_long(addr))) {
                createVariable(parent, symbol, addr, '*' + name);
            }
        } else if (mod == 2) {
            auto &source = *variable.context.global;
            auto function = source.functionMap.find(variable.symbol.name);
            if (function != source.functionMap.end()) {
                for (auto &symbol : source.functionList.at(*function).symbolList) {
                    if (symbol.kind == SymbolKind::StackSlot) {
                        continue;
                    }
                    createVariable(parent, symbol);
                }
            }
        } else if (mod == 3 && !symbol.dims.isEmpty()) {
            quint32 elements = symbol.dims.takeFirst(),
                elementSize = sizeOfSymbol(symbol, variable.context);
            if (name.startsWith('*')) {
                name = '(' + name + ')';
            }
            for (quint32 index = 0; index != elements; ++index) {
                createVariable(parent, symbol, addr,
                               name + '[' + QString::number(index) + ']');
                symbol.value += elementSize;
            }
        }
    } else if (symbol.type == 8) {
        for (auto &scope : variable.context) {
            auto record = scope.recordMap.find(variable.symbol.tag);
            if (record == scope.recordMap.end()) {
                continue;
            }
            if (name.startsWith('*')) {
                name = name.midRef(1) + "->";
            } else {
                name = name + '.';
            }
            for (auto &member : scope.recordList.at(*record).symbolList) {
                createVariable(parent, member, addr, name + stringList().value(member.name - 1));
            }
            break;
        }
    }
}

int SourcesWidget::GlobalModel::commonPrefixLength(const QStringList &paths) {
    int maxPrefixLength = 0;
    if (!paths.count()) {
        return maxPrefixLength;
    }
    forever {
        int prefixLength = paths.first().indexOf('/', maxPrefixLength) + 1;
        if (!prefixLength) {
            return maxPrefixLength;
        }
        QStringRef prefix = paths.first().leftRef(prefixLength);
        for (int i = 1; i < paths.count(); i++) {
            if (!paths.at(i).startsWith(prefix)) {
                return maxPrefixLength;
            }
        }
        maxPrefixLength = prefixLength;
    }
}
void SourcesWidget::GlobalModel::init(const QStringList &paths) {
    beginResetModel();
    m_freeVariable = -1;
    m_variables.clear();
    m_topLevelData[0].clear();
    m_topLevelData[1].clear();
    m_topLevelChildren.clear();
    int prefixLength = commonPrefixLength(paths);
    for (int parent = 0; parent < sources().count(); ++parent) {
        QStringRef path = paths.at(parent).midRef(prefixLength);
        int lastSeparator = path.lastIndexOf('/') + 1;
        m_topLevelData[0] << path.left(lastSeparator).toString();
        m_topLevelData[1] << path.mid(lastSeparator).toString();
        m_topLevelChildren.append(QList<int>());

        auto &source = sources().at(parent);
        auto &symbols = source.symbolList;
        Context context = { nullptr, &source };
        for (int child = 0; child < symbols.count(); ++child) {
            auto &symbol = symbols.at(child);
            QString name = stringList().value(symbol.name - 1);
            m_topLevelChildren.last() <<
                createVariable(s_topLevelParent, parent, child, symbol, context);
        }
    }
    endResetModel();
}

void SourcesWidget::StackModel::update() {
    QVector<StackEntry> stack;
    quint32 ix = cpu.registers.IX, pc = cpu.registers.PC;
    bool top = true;
    forever {
        int sourceIndex, functionIndex;
        if (!lookupAddr(pc, &sourceIndex, &functionIndex, nullptr)) {
            break;
        }
        const Source &source = sources().at(sourceIndex);
        const Function &function = source.functionList.at(functionIndex);
        static const quint8 prolog[2 + 5 + 2] = {0xDD,0xE5, 0xDD,0x21,0,0,0, 0xDD,0x39},
            epilog[2 + 2 + 1] = {0xDD,0xF9, 0xDD,0xE1, 0xC9};
        static const quint32 stackTop = 0xD1A87E, stackBot = 0xD1987E;
        quint32 prologStartAddr = function.lines.first().addr,
            prologEndAddr = function.lines.at(1).addr,
            epilogStartAddr = function.lines.last().addr - sizeof(epilog);
        void *prologPtr = phys_mem_ptr(prologStartAddr, sizeof(prolog)),
            *epilogPtr = phys_mem_ptr(epilogStartAddr, sizeof(epilog));
        if (!prologPtr || memcmp(prologPtr, prolog, sizeof(prolog)) ||
            !epilogPtr || memcmp(epilogPtr, epilog, sizeof(epilog))) {
            break;
        }
        if (pc >= prologEndAddr && pc <= epilogStartAddr) {
            stack << StackEntry{ ix, pc, { mem_peek_long(ix), mem_peek_long(ix + 3) },
                                 &source, &function };
        }
        quint32 ixAddr, pcAddr;
        if (pc < prologStartAddr + sizeof(prolog)) {
            if (!top) {
                break;
            }
            if (pc < prologStartAddr + 2) { // PUSH IX
                ixAddr = ~0u;
                pcAddr = cpu.registers.SPL;
            } else { // LD IX,0 \ ADD IX,SP
                ixAddr = cpu.registers.SPL;
                pcAddr = cpu.registers.SPL + 3;
            }
        } else if (pc >= epilogStartAddr + 2 + 2) { // LD SP,IX \ POP IX
            if (!top) {
                break;
            }
            ixAddr = ~0u;
            pcAddr = cpu.registers.SPL;
        } else {
            ixAddr = ix;
            pcAddr = ix + 3;
        }
        if (ixAddr > stackBot + 3 && ixAddr <= stackTop - 3) {
            ix = mem_peek_long(ixAddr);
        } else if (ixAddr != ~0u) {
            break;
        }
        if (pcAddr > stackBot + 3 && pcAddr <= stackTop - 3) {
            pc = mem_peek_long(pcAddr);
        } else {
            break;
        }
        top = false;
    }
    if (stack.isEmpty()) {
        ix = cpu.registers.SPL;
    } else {
        ix = stack.last().ix;
    }
    int aliveIndex = m_stack.count();
    while (aliveIndex--) {
        auto &entry = m_stack.at(aliveIndex);
        if (entry.ix <= ix ||
            mem_peek_long(entry.ix) != entry.cookie[0] ||
            mem_peek_long(entry.ix + 3) != entry.cookie[1]) {
            break;
        }
    }
    int commonSuffix = m_stack.count() - aliveIndex++;
    stack << m_stack.mid(aliveIndex);
    for (; commonSuffix <= stack.count() && commonSuffix <= m_stack.count() &&
             stack.at(stack.count() - commonSuffix).function ==
             m_stack.at(m_stack.count() - commonSuffix).function; ++commonSuffix);
    removeTopLevels(0, m_stack.count() - commonSuffix);
    VariableModel::update();
    if (commonSuffix <= stack.count()) {
        beginInsertRows(QModelIndex(), 0, stack.count() - commonSuffix);
        for (int parent = stack.count() - commonSuffix; parent >= 0; --parent) {
            auto &entry = stack.at(parent);
            qDebug().nospace().noquote() << stringList().at(entry.function->name - 1) << ':';
            m_topLevelData[0].prepend(stringList().value(entry.function->name - 1));
            m_topLevelData[1].prepend("()");
            m_topLevelChildren.prepend({});
            Context context = { entry.function, entry.source };
            auto &symbols = entry.function->symbolList;
            for (int child = 0; child < symbols.count(); ++child) {
                auto &symbol = symbols.at(child);
                QString name = stringList().value(symbol.name - 1);
                m_topLevelChildren.first() <<
                    createVariable(s_topLevelParent, parent - stack.count(), child, symbol,
                                   context, symbol.kind == SymbolKind::StackSlot ? entry.ix : 0);
            }
        }
        qDebug() << "";
        endInsertRows();
    }
    int firstParentChanged = -1;
    const QVector<int> changedRoles{ Qt::DisplayRole };
    for (int parent = 0; parent < stack.count(); ++parent) {
        auto &symbols = stack.at(parent).function->symbolList;
        QStringList argumentList;
        for (int child = 0; child < symbols.count(); ++child) {
            auto &symbol = symbols.at(child);
            if (symbol.kind != SymbolKind::StackSlot) {
                continue;
            }
            if (symbol.value < 0) {
                break;
            }
            argumentList <<
                m_variables.at(m_topLevelChildren.at(parent).at(child)).data[1];
            if (argumentList.last().isEmpty()) {
                argumentList.last() = '?';
            }
        }
        QString arguments = '(' + argumentList.join(", ") + ')';
        bool argumentsChanged = m_topLevelData[1].at(parent) != arguments;
        m_topLevelData[1][parent] = arguments;
        if (argumentsChanged ^ (firstParentChanged == -1)) {
            if (argumentsChanged) {
                emit dataChanged(createIndex(firstParentChanged, 1, s_topLevelId),
                                 createIndex(parent - 1, 1, s_topLevelId), changedRoles);
                firstParentChanged = -1;
            } else {
                firstParentChanged = parent;
            }
        }
    }
    if (firstParentChanged != -1) {
        emit dataChanged(createIndex(firstParentChanged, 1, s_topLevelId),
                         createIndex(stack.count() - 1, 1, s_topLevelId), changedRoles);
    }
    m_stack = stack;
}
void SourcesWidget::StackModel::init() {
    beginResetModel();
    m_freeVariable = -1;
    m_variables.clear();
    m_topLevelData[0].clear();
    m_topLevelData[1].clear();
    m_topLevelChildren.clear();
    m_stack.clear();
    endResetModel();
    update();
}
