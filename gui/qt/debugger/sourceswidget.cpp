#include "sourceswidget.h"
#include "cdebughighlighter.h"
#include "mainwindow.h"
#include "utils.h"

#include <QtCore/QDirIterator>
#include <QtGui/QTextBlock>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>

#include <algorithm>
#include <iterator>

QDebug operator<<(QDebug debug, const SourcesWidget::Line &line) {
    QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "Line(num=" << line.num << ", addr=" << QString::number(line.addr, 16) << ')';
    return debug.maybeSpace();
}
QDebug operator<<(QDebug debug, const SourcesWidget::Symbol &symbol) {
    QDebugStateSaver saver(debug);
    debug.nospace() << "Symbol(value=" << symbol.value << ", type=" << symbol.type << ", tag=" << symbol.tag << ", kind=" << symbol.kind << ", length=" << symbol.length << ", dims=" << symbol.dims << ')';
    return debug.maybeSpace();
}
QDebug operator<<(QDebug debug, const SourcesWidget::Record &source) {
    QDebugStateSaver saver(debug);
    debug << "Record(symbols=" << source.symbols << ", records=" << source.records << ", size=" << source.size << ')';
    return debug.maybeSpace();
}
QDebug operator<<(QDebug debug, const SourcesWidget::Function &function) {
    QDebugStateSaver saver(debug);
    debug << "Function(symbols=" << function.symbols << ", records=" << function.records << ", lines=" << function.lines << ')';
    return debug.maybeSpace();
}
QDebug operator<<(QDebug debug, const SourcesWidget::Source &source) {
    QDebugStateSaver saver(debug);
    debug << "Source(symbols=" << source.symbols << ", records=" << source.records << ", functionList=" << source.functionList << ", functionMap=" << source.functionMap << ')';
    return debug.maybeSpace();
}

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
    m_button = new QPushButton(tr("Select Debug File"));
    m_tabs = new QTabWidget;
    m_tabs->setTabPosition(QTabWidget::South);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_button);
    layout->addWidget(m_tabs);
    setLayout(layout);
    connect(m_button, &QPushButton::clicked, this, &SourcesWidget::selectDebugFile);
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

#include <QtCore/QDebug>
void SourcesWidget::clear() {
   for (int i = 0; i < m_tabs->count(); i++) {
        m_tabs->widget(i)->deleteLater();
    }
    m_tabs->clear();
    m_sources.clear();
    m_stringList.clear();
    m_stringMap.clear();
 }

void SourcesWidget::selectDebugFile() {
    QString debugName = QFileDialog::getOpenFileName(this, tr("Open Debug File"), "/home/jacob/Programming/calc/ez80/c/debug/bin", tr("Debug File (*.dbg)"));
    if (debugName.isNull()) {
        return;
    }
    clear();
    DebugFile debugFile(debugName);
    QString dirPath = QFileInfo(debugFile).dir().path();
    Line prevLine{0, 0};
    bool eof = false;
    quint32 lang = 0;
    QStack<Scope *> scopes;
    scopes.reserve(3);
    bool inFunction = false;
    Symbol *symbol = Q_NULLPTR;
    quint32 alias = 0;
    while (debugFile.success && !debugFile.atEnd()) {
        switch (static_cast<DebugTokenKind>(debugFile.readULEB128())) {
            case DebugTokenKind::Eof: {
                if (!debugFile.atEnd() || !scopes.isEmpty()) {
                    debugFile.setErrorString(tr("Unexpected EOF token"));
                    break;
                }
                eof = true;
                break;
            }
            case DebugTokenKind::Alias: {
                if (!symbol || alias) {
                    debugFile.setErrorString(tr("Unexpected alias token"));
                    break;
                }
                alias = debugFile.readULEB128();
                break;
            }
            case DebugTokenKind::BegFunc: {
                if (scopes.count() != 1 || symbol) {
                    debugFile.setErrorString(tr("Unexpected function token"));
                    break;
                }
                Source &source = m_sources.last();
                QVector<Function> &functionList = source.functionList;
                QMultiHash<quint32, int> &functionMap = source.functionMap;
                int functionIndex = functionList.count();
                functionMap.insert(debugFile.readULEB128(), functionIndex);
                functionMap.insert(debugFile.readULEB128(), functionIndex);
                functionList.resize(functionIndex + 1);
                Line line = prevLine;
                line.num += debugFile.readULEB128();
                line.addr += debugFile.readULEB128();
                prevLine = line;
                functionList.last().lines.append(line);
                scopes.push(&functionList.last());
                inFunction = true;
                break;
            }
            case DebugTokenKind::BegRec: {
                if (scopes.isEmpty() || symbol) {
                    debugFile.setErrorString(tr("Unexpected record begin token"));
                    break;
                }
                quint32 name = debugFile.readULEB128();
                Record record;
                record.size = debugFile.readULEB128();
                scopes.push(&*scopes.top()->records.insert(name, record));
                break;
            }
            case DebugTokenKind::Class: {
                if (!symbol) {
                    debugFile.setErrorString(tr("Unexpected class begin token"));
                    break;
                }
                symbol->kind = debugFile.readULEB128();
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
                if (scopes.isEmpty() || symbol) {
                    debugFile.setErrorString(tr("Unexpected define symbol begin token"));
                    break;
                }
                quint32 name = debugFile.readULEB128();
                symbol = &*scopes.top()->symbols.insert(name, Symbol());
                break;
            }
            case DebugTokenKind::Dim: {
                if (!symbol) {
                    debugFile.setErrorString(tr("Unexpected dimension token"));
                    break;
                }
                symbol->dims.append(debugFile.readULEB128());
                break;
            }
            case DebugTokenKind::End: {
                if (scopes.count() != 1 || symbol) {
                    debugFile.setErrorString(tr("Unexpected source file end token"));
                    break;
                }
                m_sources.last().endAddr = prevLine.addr += debugFile.readULEB128();
                scopes.pop();
                break;
            }
            case DebugTokenKind::Endef: {
                if (!symbol) {
                    debugFile.setErrorString(tr("Unexpected define symbol end token"));
                    break;
                }
                if (alias) {
                    Symbol temp = *symbol;
                    scopes.top()->symbols.insert(alias, temp);
                    alias = 0;
                }
                symbol = Q_NULLPTR;
                break;
            }
            case DebugTokenKind::EndFunc: {
                if (scopes.count() != 2 || !inFunction || symbol) {
                    debugFile.setErrorString(tr("Unexpected function end token"));
                    break;
                }
                debugFile.readULEB128();
                debugFile.readULEB128();
                Line line = prevLine;
                line.num += debugFile.readULEB128();
                line.addr += debugFile.readULEB128();
                prevLine = line;
                m_sources.last().functionList.last().lines.append(line);
                scopes.pop();
                inFunction = false;
                break;
            }
            case DebugTokenKind::EndRec: {
                if (scopes.count() <= 1 + inFunction || symbol) {
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
                QString sourcePath = dirPath;
                QStringList components = debugFile.readString().split('\\');
                for (QStringList::iterator i = components.begin(), e = components.end(); debugFile.success && i != e; ) {
                    QStringList component(*i++);
                    QDirIterator matches(sourcePath, component, (i != e ? QDir::Dirs : QDir::Files) | QDir::Readable);
                    if (matches.hasNext()) {
                        sourcePath = matches.next();
                    } else {
                        debugFile.setErrorString(tr("Unable to find source file \"%0\"").arg(components.join(QDir::separator())));
                    }
                }
                QFile sourceFile(sourcePath);
                if (debugFile.success && sourceFile.open(QIODevice::ReadOnly)) {
                    QPlainTextEdit *sourceView = new QPlainTextEdit(sourceFile.readAll());
                    sourceView->setObjectName(QStringLiteral("sourceView%0").arg(m_sources.count()));
                    sourceView->setReadOnly(true);
                    sourceView->setCenterOnScroll(true);
                    sourceView->setContextMenuPolicy(Qt::CustomContextMenu);
                    connect(sourceView, &QWidget::customContextMenuRequested, this, &SourcesWidget::sourceContextMenu);
                    m_tabs->addTab(sourceView, QFileInfo(sourceFile).baseName());
                    new CDebugHighlighter(this, sourceView);
                    m_sources.resize(m_sources.count() + 1);
                    m_sources.last().startAddr = prevLine.addr += debugFile.readULEB128();
                    prevLine.num = 0;
                    scopes.push(&m_sources.last());
                } else {
                    debugFile.setErrorString(tr("Error opening source file \"%0\"").arg(components.join(QDir::separator())));
                }
                break;
            }
            case DebugTokenKind::Length: {
                if (!symbol) {
                    debugFile.setErrorString(tr("Unexpected length token"));
                    break;
                }
                symbol->length = debugFile.readULEB128();
                break;
            }
            case DebugTokenKind::Line: {
                if (scopes.count() != 2 || !inFunction || symbol) {
                    debugFile.setErrorString(tr("Unexpected line token"));
                    break;
                }
                Line line = prevLine;
                line.num += debugFile.readULEB128();
                line.addr += debugFile.readULEB128();
                prevLine = line;
                m_sources.last().functionList.last().lines.append(line);
                break;
            }
            case DebugTokenKind::Strings: {
                if (!scopes.isEmpty()) {
                    debugFile.setErrorString(tr("Unexpected strings token"));
                    break;
                }
                quint32 count = debugFile.readULEB128();
                m_stringList.reserve(count);
                m_stringMap.reserve(count);
                for (quint32 index = 0; debugFile.success && index != count; ++index) {
                    QString string = debugFile.readString();
                    if (string.isEmpty()) {
                        debugFile.setErrorString(tr("Invalid string in string table"));
                    } else {
                        m_stringList << string;
                        m_stringMap[string] = index;
                    }
                }
                break;
            }
            case DebugTokenKind::Tag: {
                if (!symbol) {
                    debugFile.setErrorString(tr("Unexpected tag token"));
                    break;
                }
                symbol->tag = debugFile.readULEB128();
                break;
            }
            case DebugTokenKind::Type: {
                if (!symbol) {
                    debugFile.setErrorString(tr("Unexpected value token"));
                    break;
                }
                symbol->type = debugFile.readSLEB128();
                break;
            }
            case DebugTokenKind::Value: {
                if (!symbol) {
                    debugFile.setErrorString(tr("Unexpected value token"));
                    break;
                }
                symbol->value = debugFile.readSLEB128();
                break;
            }
            default: debugFile.setErrorString(tr("Unknown debug token kind"));
        }
    }
    if (m_sources.isEmpty()) {
        debugFile.setErrorString(tr("No source files"));
    }
    if (!eof) {
        debugFile.setErrorString(tr("Missing EOF token"));
    }
    if (m_stringList.value(lang - 1) != QStringLiteral("C")) {
        debugFile.setErrorString(tr("Unknown source language \"%0\"").arg(m_stringList.value(lang - 1)));
    }
    if (debugFile.success) {
        qDebug() << m_sources;
        qDebug() << m_stringList;
        qDebug() << m_stringMap;
        dumpObjectTree();
    } else {
        clear();
        QMessageBox dialog(QMessageBox::Critical, findParent<MainWindow *>(this)->MSG_ERROR, tr("Error loading debug file:"), QMessageBox::Ok, this);
        dialog.setInformativeText(debugFile.errorString());
        dialog.exec();
    }
}

void SourcesWidget::update(quint32 addr, bool pc) {
    qDebug() << addr;
    const QVector<Source> &sources = m_sources;
    const SourceBase sourceKey{addr, addr};
    auto source = std::upper_bound(sources.begin(), sources.end(), sourceKey,
                                   [](const SourceBase &first,
                                      const SourceBase &second) {
                                       return first.endAddr < second.startAddr;
                                   });
    if (source == sources.begin() ||
        addr < (--source)->startAddr || addr >= source->endAddr) {
        return;
    }
    qDebug() << *source;
    auto sourceView = m_tabs->findChild<QPlainTextEdit *>(QStringLiteral("sourceView%0").arg(std::distance(sources.begin(), source)));
    qDebug() << sourceView;
    const QVector<Function> &functions = source->functionList;
    const Line lineKey{0, addr};
    const FunctionBase functionKey{{lineKey}};
    auto function = std::upper_bound(functions.begin(), functions.end(),
                                     functionKey,
                                     [](const FunctionBase &first,
                                        const FunctionBase &second) {
                                         return first.lines.last().addr <
                                             second.lines.first().addr;
                                     });
    if (function == functions.begin()) {
        return;
    }
    const QVector<Line> &lines = (--function)->lines;
    qDebug() << *function;
    if (addr < lines.first().addr || addr >= lines.last().addr) {
        return;
    }
    auto line =
        std::prev(std::upper_bound(std::next(lines.begin()), std::prev(lines.end()),
                                   lineKey,
                                   [](const Line &first, const Line &second) {
                                       return first.addr < second.addr;
                                   }));
    qDebug() << *line;
    QTextBlock block = sourceView->document()->findBlockByLineNumber(line->num - 1);
    if (!block.isValid()) {
        return;
    }
    qDebug() << block.text();
    if (pc) {
        m_tabs->setCurrentWidget(sourceView);
        QTextCursor cursor(block);
        cursor.select(QTextCursor::LineUnderCursor);
        sourceView->setTextCursor(cursor);
    }
}

void SourcesWidget::updateAll() {
    for (QSyntaxHighlighter *highlighter : m_tabs->findChildren<QSyntaxHighlighter *>()) {
        highlighter->rehighlight();
    }
}

void SourcesWidget::updatePC(quint32 pc) {
    update(pc, true);
}

void SourcesWidget::updateAddr(quint32 addr) {
    update(addr, false);
}

void SourcesWidget::sourceContextMenu(const QPoint &pos) {
    QPlainTextEdit *sourceView = static_cast<QPlainTextEdit *>(m_tabs->currentWidget());
    const QVector<Function> &functions = m_sources.at(sourceView->objectName().midRef(QStringLiteral("sourceView").count()).toInt()).functionList;
    quint32 num = sourceView->cursorForPosition(pos).block().firstLineNumber() + 1;
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
    qDebug() << *function;
    if (lineKey.num < lines.first().num || lineKey.num >= lines.last().num) {
        return;
    }
    auto line =
        std::prev(std::upper_bound(std::next(lines.begin()), std::prev(lines.end()),
                                   lineKey,
                                   [](const Line &first, const Line &second) {
                                       return first.num < second.num;
                                   }));
    qDebug() << *line;

    QMenu menu;
    QAction toggleBreak(tr("Toggle Breakpoint"));
    menu.addAction(&toggleBreak);

    QAction *action = menu.exec(sourceView->mapToGlobal(pos));
    if (action == &toggleBreak) {
        emit breakToggled(line->addr);
    }
}

void SourcesWidget::setSourceFont(const QFont &font) {
    for (auto *format : { &m_defaultFormat, &m_operatorFormat, &m_literalFormat, &m_escapeFormat,
                          &m_preprocessorFormat, &m_commentFormat, &m_keywordFormat, &m_identifierFormat, &m_errorFormat }) {
        QFont::Weight weight = static_cast<QFont::Weight>(format->fontWeight());
        format->setFont(font);
        format->setFontWeight(weight);
    }
    updateAll();
}
