#include "tablewidget.h"
#include "util.h"

#include <QtWidgets/QHeaderView>
#include <QtGui/QKeyEvent>

TableWidget::TableWidget(int rows, int cols, QWidget *parent)
    : QTableWidget{rows, cols, parent}
{
    setItemDelegate(new TableWidgetItemFocusDelegate{this});
    verticalHeader()->setSectionsMovable(true);
    setFont(Util::monospaceFont());
}

void TableWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete)
    {
        emit deletePressed();
    }

    QTableWidget::keyPressEvent(event);
}

void TableWidgetItemFocusDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    option->state &= ~QStyle::State_HasFocus;

    if (option->features & QStyleOptionViewItem::HasDecoration)
    {
        QSize s{option->decorationSize};
        s.setWidth(option->rect.width());
        option->decorationSize = s;
    }
}

