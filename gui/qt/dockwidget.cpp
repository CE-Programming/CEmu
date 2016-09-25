#include "dockwidget.h"

#include <QtWidgets/QTabWidget>
#include <QtGui/QHoverEvent>

DockWidget::DockWidget(QWidget *parent) : DockWidget{"Screen", parent} { }
DockWidget::DockWidget(QTabWidget *tabs, QWidget *parent) : DockWidget{tabs->tabText(0), parent} {
    setWindowIcon(tabs->tabIcon(0));
    setWidget(tabs->widget(0));
}
DockWidget::DockWidget(const QString &title, QWidget *parent)
    : QDockWidget{title, parent}, m_hide_title{new QWidget{this}}, m_title_height{-1} {
    setObjectName(windowTitle());
    setAttribute(Qt::WA_Hover);
    connect(this, &DockWidget::changeTitleState, this, &DockWidget::titleStateChanged, Qt::QueuedConnection);
    emit changeTitleState(false);
}

bool DockWidget::event(QEvent *event) {
    switch (event->type()) {
        case QEvent::HoverMove:
            emit changeTitleState(static_cast<QHoverEvent *>(event)->pos().y() <= m_title_height*.75);
            break;
        case QEvent::HoverLeave:
            emit changeTitleState(false);
            break;
        default:
            break;
    }
    return QDockWidget::event(event);
}

void DockWidget::titleStateChanged(bool visible) {
    visible |= isWindow();
    if ((visible) ^ (titleBarWidget() == Q_NULLPTR)) {
        if (!visible) {
            m_title_height = widget()->y();
        }
        setTitleBarWidget(visible ? Q_NULLPTR : m_hide_title);
    }
}
