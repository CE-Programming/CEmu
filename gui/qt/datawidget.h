#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QObject>

QT_BEGIN_NAMESPACE
class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;
QT_END_NAMESPACE

class DataWidget : public QPlainTextEdit
{
    Q_OBJECT

public:
    DataWidget(QWidget *parent = 0);
    void clearAllHighlights();
    void updateAllHighlights();
    void addHighlight(QColor);
    void cursorState(bool movable);
    QString getSelectedAddress();

private:
    void highlightCurrentLine();

    bool cursor_state = false;
    QList<QTextEdit::ExtraSelection> extraHighlights;
};


#endif
