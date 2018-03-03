#ifndef DISASMWIDGET_H
#define DISASMWIDGET_H

#include <QtWidgets/QWidget>

class DisasmWidget : public QWidget {
    Q_OBJECT

public:
    explicit DisasmWidget(QWidget *parent = Q_NULLPTR);
    virtual ~DisasmWidget() { }

    QSize sizeHint() const Q_DECL_OVERRIDE;

protected:
    virtual void wheelEvent(QWheelEvent *) Q_DECL_OVERRIDE;
    virtual void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

private:
    typedef enum { None, Label, Inst } addr_type_t;
    typedef struct { uint32_t addr : 24; addr_type_t type : 8; } addr_t;
    addr_t next(addr_t addr);

    addr_t m_baseAddr = { 0, None };
    int m_scroll = 0;
    QHash<uint32_t, QString> m_labels;
};

#endif
