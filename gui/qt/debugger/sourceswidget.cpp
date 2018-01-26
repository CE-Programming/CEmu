#include "sourceswidget.h"
#include "cdebughighlighter.h"

#include <QtCore/QDirIterator>
#include <QtGui/QTextFrame>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>

namespace {
class DebugFile : public QFile {
public:
    DebugFile(QString name) : QFile(name) {
        open(QIODevice::ReadOnly);
    }
    quint32 readULEB128() {
        quint32 result = 0, shift = 0;
        char next;
        do {
            getChar(&next);
            result |= (next & 0x7F) << shift;
            shift += 7;
            if (shift >= 32) {
                setErrorString("shift overflow");
                return 0;
            }
        } while (next & 0x80);
        return result;
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

void SourcesWidget::selectDebugFile() {
    QString debugName = QFileDialog::getOpenFileName(this, tr("Open Debug File"), "/home/jacob/Programming/calc/ez80/c/debug/bin", tr("Debug File (*.dbg)"));
    if (debugName.isNull()) {
        return;
    }
    for (int i = 0; i < m_tabs->count(); i++) {
        m_tabs->widget(i)->deleteLater();
    }
    m_tabs->clear();
    m_highlighters.clear();
    m_addrToLoc.clear();
    m_locToAddr.clear();
    DebugFile debugFile(debugName);
    quint8 index = 0;
    while (!debugFile.atEnd()) {
        bool success = true;
        QString dir = QFileInfo(debugFile).dir().path();
        {
            QStringList components = QString::fromLocal8Bit(debugFile.read(debugFile.readULEB128())).split('\\');
            for (QStringList::iterator i = components.begin(), e = components.end(); success && i != e; ) {
                QStringList component(*i++);
                QDirIterator matches(dir, component, (i != e ? QDir::Dirs : QDir::Files) | QDir::Readable);
                if (matches.hasNext()) {
                    dir = matches.next();
                } else {
                    success = false;
                }
            }
        }
        {
            QFile file(dir);
            if (success && file.open(QIODevice::ReadOnly)) {
                QPlainTextEdit *source = new QPlainTextEdit(file.readAll());
                source->setReadOnly(true);
                source->setCenterOnScroll(true);
                source->setContextMenuPolicy(Qt::CustomContextMenu);
                m_highlighters << new CDebugHighlighter(this, source);
                connect(source, &QWidget::customContextMenuRequested, this, &SourcesWidget::sourceContextMenu);
                m_tabs->addTab(source, QFileInfo(file).baseName());
            }
        }
        {
            quint32 addr = debugFile.readULEB128(), line = 0, lineOff = ~0;
            while (true) {
                if (success) {
                    quint32 loc = index << 24 | (lineOff ? line : s_lastLine);
                    m_addrToLoc[addr] = loc;
                    m_locToAddr[loc] = addr;
                }
                if (!lineOff || debugFile.atEnd()) {
                    break;
                }
                addr += debugFile.readULEB128();
                line += lineOff = debugFile.readULEB128();
                if (line >= s_lastLine) {
                    return;
                }
            }
        }
        if (!++index) {
            return;
        }
    }
}

void SourcesWidget::update(quint32 addr, bool pc) {
    if (m_addrToLoc.empty()) {
        return;
    }
    auto i = m_addrToLoc.upperBound(addr);
    if (i == m_addrToLoc.begin() || i == m_addrToLoc.end()) {
        return;
    }
    quint32 loc = *--i, line = loc & 0xFFFFFF;
    quint8 index = loc >> 24;
    if (index >= m_highlighters.size()) {
        return;
    }
    QPlainTextEdit *source = static_cast<QPlainTextEdit *>(m_tabs->widget(index));
    if (!line) {
        return;
    }
    QTextBlock block = source->document()->findBlockByNumber(line);
    if (!block.isValid()) {
        return;
    }
    m_highlighters[index]->rehighlightBlock(block);
    if (pc) {
        m_tabs->setCurrentIndex(index);
        source->setTextCursor(QTextCursor(block));
    }
}

void SourcesWidget::updateAll() {
    for (auto *highlighter : m_highlighters) {
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
    QPlainTextEdit *source = static_cast<QPlainTextEdit *>(m_tabs->currentWidget());
    quint8 index = m_tabs->currentIndex();
    quint32 line = source->cursorForPosition(pos).block().blockNumber() + 1,
        loc = index << 24 | line;
    auto i = m_locToAddr.upperBound(loc);
    if (i != m_addrToLoc.begin()) {
        i--;
    }
    quint32 addr = *i;

    QMenu menu;
    QAction toggleBreak(tr("Toggle Breakpoint"));
    menu.addAction(&toggleBreak);

    QAction *action = menu.exec(source->mapToGlobal(pos));
    if (action == &toggleBreak) {
        emit breakToggled(addr);
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
