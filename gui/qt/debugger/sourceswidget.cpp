#include "sourceswidget.h"

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
}

void SourcesWidget::selectDebugFile() {
    QString debugName = QFileDialog::getOpenFileName(this, tr("Open Debug File"), "", tr("Debug File (*.dbg)"));
    if (debugName.isNull()) {
        return;
    }
    m_tabs->clear();
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
                source->setFont(m_sourceFont);
                source->setCenterOnScroll(true);
                source->setContextMenuPolicy(Qt::CustomContextMenu);
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

void SourcesWidget::updatePC(quint32 pc) {
    QTextBlockFormat format;
    if (!m_pcCursor.isNull()) {
        format.setBackground(Qt::white);
        m_pcCursor.mergeBlockFormat(format);
    }
    if (m_addrToLoc.empty()) {
        return;
    }
    auto i = m_addrToLoc.upperBound(pc);
    if (i == m_addrToLoc.begin() || i == m_addrToLoc.end()) {
        return;
    }
    quint32 loc = *--i, line = loc & 0xFFFFFF;
    quint8 index = loc >> 24;
    QPlainTextEdit *source = static_cast<QPlainTextEdit *>(m_tabs->widget(index));
    if (line) {
        line--;
    }
    format.setBackground(Qt::red);
    m_pcCursor = QTextCursor(source->document()->findBlockByLineNumber(line));
    m_pcCursor.mergeBlockFormat(format);
    source->setTextCursor(m_pcCursor);
    m_tabs->setCurrentIndex(index);
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
    for (int i = 0; i != m_tabs->count(); i++) {
        m_tabs->widget(i)->setFont(font);
    }
    m_sourceFont = font;
}
