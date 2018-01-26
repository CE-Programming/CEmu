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
    void update(quint32 addr, bool pc);
    void updateAll();

    friend CDebugHighlighter;
    QPushButton *m_button;
    QTabWidget *m_tabs;
    QList<CDebugHighlighter *> m_highlighters;
    QTextCharFormat m_defaultFormat, m_operatorFormat, m_literalFormat, m_escapeFormat,
        m_preprocessorFormat, m_commentFormat, m_keywordFormat, m_identifierFormat, m_errorFormat;
    QMap<quint32, quint32> m_addrToLoc, m_locToAddr;
    static const quint32 s_lastLine = (1 << 24) - 1;
};

#endif
