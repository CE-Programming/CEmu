#ifndef TABLEWIDGET_H
#define TABLEWIDGET_H

#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>

class TableWidget : public QTableWidget {
    Q_OBJECT

public:
    explicit TableWidget(QWidget *p = Q_NULLPTR);

protected:
    virtual void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
};


#endif
