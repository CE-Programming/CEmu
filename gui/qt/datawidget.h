#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QtWidgets/QPlainTextEdit>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
    class QPaintEvent;
    class QResizeEvent;
    class QSize;
    class QWidget;
QT_END_NAMESPACE

class DataWidget : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit DataWidget(QWidget *p = Q_NULLPTR);
    void clearAllHighlights();
    void updateAllHighlights();
    void addHighlight(const QColor& color);
    void highlightCurrentLine();
    void cursorState(bool movable);
    bool labelCheck();
    QString getSelectedAddr();

signals:
    void gotoDisasmAddress(uint32_t address);
    void gotoMemoryAddress(uint32_t address);

private:
    bool moveable;
    QList<QTextEdit::ExtraSelection> highlights;
};


#endif
