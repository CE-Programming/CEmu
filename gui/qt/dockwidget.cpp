#include "dockwidget.h"

#include <QtWidgets/QTabWidget>
#include <QtGui/QHoverEvent>

DockWidget::DockWidget(QWidget *parent) : DockWidget{"Screen", parent} { }
DockWidget::DockWidget(QTabWidget *tabs, QWidget *parent) : DockWidget{tabs->tabText(0), parent} {
    setWindowIcon(tabs->tabIcon(0));
    setWidget(tabs->widget(0));
}

DockWidget::DockWidget(const QString &title, QWidget *parent)
    : QDockWidget{title, parent}, titleHide{new QWidget{this}}, titleHeight{-1} {
    setObjectName(windowTitle());
}

void DockWidget::toggleState(bool visible) {
    visible |= isWindow();
    if ((visible) ^ (titleBarWidget() == Q_NULLPTR)) {
        if (!visible) {
            titleHeight = widget()->y();
        }
        setTitleBarWidget(visible ? Q_NULLPTR : titleHide);
    }
}
