#include "dockwidget.h"

#include <QtWidgets/QTabWidget>
#include <QtWidgets/QApplication>
#include <QtGui/QHoverEvent>

DockWidget::DockWidget(QWidget *parent) : DockWidget{tr("Screen"), parent} { }

DockWidget::DockWidget(QTabWidget *tabs, QWidget *parent) : DockWidget{tabs->tabText(0), parent} {
    setWindowIcon(tabs->tabIcon(0));
    setWidget(tabs->widget(0));
    setObjectName(windowTitle());
}

DockWidget::DockWidget(const QString &title, QWidget *parent) : QDockWidget{title, parent}, titleHide{new QWidget{this}} {
    setObjectName(title);
}

void DockWidget::toggleState(bool visible) {
    if (visible) {
        if (objectName() == QStringLiteral("screenWidget")) {
            setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        } else {
            setFeatures(QDockWidget::AllDockWidgetFeatures);
        }
        setAllowedAreas(Qt::AllDockWidgetAreas);
    } else {
        setFeatures(features() & ~(QDockWidget::AllDockWidgetFeatures));
        setAllowedAreas(Qt::NoDockWidgetArea);
    }
    if (isFloating()) {
        setTitleBarWidget(Q_NULLPTR);
        return;
    } else {
        setTitleBarWidget(visible ? Q_NULLPTR : titleHide);
    }
}
