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
    uint32_t m_baseAddr = 0, m_scroll = 0;
};

#endif
