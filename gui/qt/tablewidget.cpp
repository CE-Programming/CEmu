#include "tablewidget.h"

#include <QtGui/QDropEvent>

TableWidget::TableWidget(QWidget *parent) : QTableWidget{parent} {}

void TableWidget::dropEvent(QDropEvent *e) {
    if(e->source() != this) {
        e->ignore();
        return;
    }

    int newrow = indexAt(e->position().toPoint()).row();
    if (newrow < 0) {
        newrow = rowCount();
    }

    blockSignals(true);

    QList<QTableWidgetItem*> items = this->selectedItems();
    if (items.isEmpty()) {
        e->ignore();
        return;
    }

    insertRow(newrow);
    int oldrow = items.first()->row();
    foreach(QTableWidgetItem *item, items) {
        int col = item->column();
        takeItem(oldrow, col);
        setItem(newrow, col, item);
        setCellWidget(newrow, col, cellWidget(oldrow, col));
    }

    removeRow(oldrow);
    if (oldrow <= newrow) {
        newrow--;
    }
    if (newrow < 0) {
        newrow = 0;
    }

    blockSignals(false);
    setCurrentCell(newrow, 0);
}
