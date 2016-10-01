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
    explicit DataWidget(QWidget *parent = Q_NULLPTR);
    void clearAllHighlights();
    void updateAllHighlights();
    void addHighlight(QColor);
    void highlightCurrentLine();
    void cursorState(bool movable);
    QString getSelectedAddress();

private:
    bool cursor_moveable;
    QList<QTextEdit::ExtraSelection> extraHighlights;
};


#endif
