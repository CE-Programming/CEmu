#include "dockwidget.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QTabWidget>
#include <QtGui/QHoverEvent>

DockWidget::DockWidget(QWidget *parent) : QDockWidget{parent}, m_titleHide{new QWidget},
                                          m_closable{true}, m_expandable{false} {
    connect(this, &QDockWidget::topLevelChanged, this, &DockWidget::dockLocationChange);
}

DockWidget::DockWidget(const QString &title, QWidget *parent) : DockWidget{parent} {
    setWindowTitle(title);
    setObjectName(title + QStringLiteral("_dock"));
}

DockWidget::DockWidget(QTabWidget *tabs, QWidget *parent) : DockWidget{tabs->tabText(0), parent} {
    setObjectName(tabs->widget(0)->objectName() + QStringLiteral("_dock"));
    setWindowIcon(tabs->tabIcon(0));
    setWidget(tabs->widget(0));
    setExpandable(!(objectName() == QStringLiteral("debugAutoTesterWidget_dock") ||
                    objectName() == QStringLiteral("debugCpuStatusWidget_dock") ||
                    objectName() == QStringLiteral("debugControlWidget_dock") ||
                    objectName() == QStringLiteral("debugTimerWidget_dock") ||
                    objectName() == QStringLiteral("debugMiscWidget_dock") ||
                    objectName() == QStringLiteral("settingsWidget_dock") ||
                    objectName() == QStringLiteral("captureWidget_dock"))); // TODO: lolz fixme plz
}

void DockWidget::toggleState(bool visible) {
    if (visible) {
        if (isClosable()) {
            setFeatures(QDockWidget::AllDockWidgetFeatures);
        } else {
            setFeatures(QDockWidget::DockWidgetFeatures(QDockWidget::AllDockWidgetFeatures & ~QDockWidget::DockWidgetClosable));
        }
        setAllowedAreas(Qt::AllDockWidgetAreas);
    } else {
        setFeatures(QDockWidget::NoDockWidgetFeatures);
        setAllowedAreas(Qt::NoDockWidgetArea);
    }
    setTitleBarWidget(isFloating() || visible ? Q_NULLPTR : new QWidget(this));
}

bool DockWidget::isAnyTabExpandable() {
    if (isExpandable()) {
        return true;
    }
    for (QDockWidget *tab : qobject_cast<QMainWindow *>(parentWidget())->tabifiedDockWidgets(this)) {
        if (qobject_cast<DockWidget *>(tab)->isExpandable()) {
            return true;
        }
    }
    return false;
}

void DockWidget::dockLocationChange() {
    if (isAnyTabExpandable()) {
        widget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    } else {
        widget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    }
}
