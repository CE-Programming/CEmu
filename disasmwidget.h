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

class LineNumberArea;

class DisasmWidget : public QPlainTextEdit
{
    Q_OBJECT

public:
    DisasmWidget(QWidget *parent = 0);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

private slots:
    void updateLineNumberAreaWidth();
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);

private:
    QWidget *addressArea;
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(DisasmWidget *editor) : QWidget(editor) {
        disasmView = editor;
    }

    QSize sizeHint() const Q_DECL_OVERRIDE {
        return QSize(disasmView->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *e) Q_DECL_OVERRIDE {
        disasmView->lineNumberAreaPaintEvent(e);
    }

private:
    DisasmWidget *disasmView;
};

#endif
