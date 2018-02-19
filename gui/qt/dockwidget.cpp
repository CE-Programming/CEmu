#include "dockwidget.h"

#include <QtWidgets/QTabWidget>
#include <QtWidgets/QApplication>
#include <QtGui/QHoverEvent>

DockWidget::DockWidget(QWidget *parent) : DockWidget{tr("Screen"), parent} { }

DockWidget::DockWidget(QTabWidget *tabs, QWidget *parent) : DockWidget{tabs->tabText(0), parent} {
    setObjectName(tabs->widget(0)->objectName() + QStringLiteral("_dock"));
    setWindowIcon(tabs->tabIcon(0));
    setWidget(tabs->widget(0));
    QSizePolicy::Policy vertPolicy = QSizePolicy::Preferred;
    if (objectName() == QStringLiteral("debugAutoTesterWidget_dock") ||
        objectName() == QStringLiteral("debugCpuStatusWidget_dock") ||
        objectName() == QStringLiteral("debugControlWidget_dock") ||
        objectName() == QStringLiteral("debugTimerWidget_dock") ||
        objectName() == QStringLiteral("debugMiscWidget_dock") ||
        objectName() == QStringLiteral("settingsWidget_dock") ||
        objectName() == QStringLiteral("captureWidget_dock")) {
        vertPolicy = QSizePolicy::Maximum;
    }
    widget()->setSizePolicy(QSizePolicy::Preferred, vertPolicy);
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
