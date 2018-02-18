#ifndef DOCKWIDGET_H
#define DOCKWIDGET_H

#include <QtWidgets/QDockWidget>

QT_BEGIN_NAMESPACE
class QTabWidget;
QT_END_NAMESPACE

class DockWidget : public QDockWidget {
    Q_OBJECT

public:
    explicit DockWidget(QWidget *p = Q_NULLPTR);
    DockWidget(QTabWidget *tabs, QWidget *p = Q_NULLPTR);
    DockWidget(const QString &title, QWidget *p = Q_NULLPTR);
    void toggleState(bool visible);

private:
    QWidget *titleHide;
};

#endif
