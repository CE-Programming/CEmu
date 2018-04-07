#include "dockwidget.h"
#include "utils.h"

#include <QtWidgets/QStyle>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QDesktopWidget>
#include <QtGui/QHoverEvent>

DockWidget::DockWidget(QWidget *parent)
    : QDockWidget{parent}, m_titleHide{new QWidget{this}}, m_tabs{this},
      m_closable{true}, m_expandable{true} {}

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

void DockWidget::setState(bool edit) {
    m_showTitle = edit;
    if (edit) {
        if (isClosable()) {
            setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        } else {
            setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        }
        setAllowedAreas(Qt::AllDockWidgetAreas);
    } else {
        setFeatures(QDockWidget::NoDockWidgetFeatures);
        setAllowedAreas(Qt::NoDockWidgetArea);
    }
    setTitleBarWidget(isFloating() || edit ? Q_NULLPTR : m_titleHide);
}

QList<DockWidget *> DockWidget::tabs(DockWidget *without) {
    QList<DockWidget *> tabs;
    if (this != without) {
        tabs << this;
    }
    if (QMainWindow *window = findParent<QMainWindow *>(this)) {
        for (QDockWidget *tab : window->tabifiedDockWidgets(this)) {
            if (tab != without) {
                tabs << qobject_cast<DockWidget *>(tab);
            }
        }
    }
    return tabs;
}

void DockWidget::showEvent(QShowEvent *event) {
    if (event->spontaneous()) {
        return;
    }
    if (m_tabs != this) {
        m_tabs->updateExpandability(m_tabs->tabs(this));
    }
    updateExpandability(tabs());
}

void DockWidget::updateExpandability(const QList<DockWidget *> &tabs) {
    bool expandable = false;
    for (DockWidget *tab : tabs) {
        if (tab->isExpandable()) {
            expandable = true;
            break;
        }
    }
    DockWidget *other = tabs.last();
    for (DockWidget *tab : tabs) {
        if (QWidget *widget = tab->widget()) {
            if (expandable || tab->isExpandable()) {
                widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            } else {
                widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
            }
        }
        tab->m_tabs = other;
        other = tab;
    }
}

void DockWidget::closeEvent(QCloseEvent *event) {
    setFloating(true);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, minimumSize(), qApp->desktop()->availableGeometry()));
    emit closed();
    event->accept();
    QDockWidget::closeEvent(event);
}

