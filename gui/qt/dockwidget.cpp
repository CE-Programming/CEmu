#include "dockwidget.h"
#include "utils.h"

#include <QtWidgets/QStyle>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStackedLayout>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QDesktopWidget>
#include <QtGui/QHoverEvent>

DockWidget::DockWidget(QWidget *parent)
    : QDockWidget{parent}, m_titleHide{new QWidget{this}}, m_tabs{this},
      m_closable{true}, m_expandable{true} {
    // If we just use a vanilla new QWidget for m_titleHide, as a Qt source
    // comment tells us to, then m_titleHide->sizeHint() will return {-1, -1}
    // which, as the exact same Qt comment hints at, doesn't work and causes,
    // at the very least, one pixel to be cropped off of the top of the dock's
    // child widget.  Therefore, our two choices are to subclass QWidget and
    // override sizeHint and minimumSizeHint to both return {0, 0}, or to just
    // set a dummy layout as we do below.
    m_titleHide->setLayout(new QStackedLayout{m_titleHide});
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

void DockWidget::setState(bool edit) {
    QDockWidget::DockWidgetFeatures features;
    if (edit) {
        setAllowedAreas(Qt::AllDockWidgetAreas);
        features = QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable;
        if (isClosable()) {
            features |= QDockWidget::DockWidgetClosable;
        }
    } else {
        setAllowedAreas(Qt::NoDockWidgetArea);
        features = QDockWidget::NoDockWidgetFeatures;
        if (isFloating()) {
            features |= QDockWidget::DockWidgetFloatable;
        }
    }
    setFeatures(features);
    setTitleBarWidget(isFloating() || edit ? Q_NULLPTR : m_titleHide);
}

bool DockWidget::event(QEvent *event) {
    if (event->type() == QEvent::MouseButtonDblClick &&
        allowedAreas() == Qt::NoDockWidgetArea) {
        return QWidget::event(event);
    }
    return QDockWidget::event(event);
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

