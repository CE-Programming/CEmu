#ifndef DOCKWIDGET_H
#define DOCKWIDGET_H

#include <QtWidgets/QDockWidget>

QT_BEGIN_NAMESPACE
class QTabWidget;
QT_END_NAMESPACE

class DockWidget : public QDockWidget {
    Q_OBJECT
    Q_PROPERTY(bool closable READ isClosable WRITE setClosable)
    Q_PROPERTY(bool expandable READ isExpandable WRITE setExpandable)

public:
    explicit DockWidget(QWidget *parent = Q_NULLPTR);
    DockWidget(QTabWidget *tabs, QWidget *parent = Q_NULLPTR);
    DockWidget(const QString &title, QWidget *parent = Q_NULLPTR);
    void toggleState(bool visible);
    bool isClosable() const { return m_closable; }
    void setClosable(bool closable) { m_closable = closable; }
    bool isExpandable() const { return m_expandable; }
    void setExpandable(bool expandable) { m_expandable = expandable; }

protected slots:
    QList<DockWidget *> tabs(DockWidget *without = Q_NULLPTR);
    void showEvent(QShowEvent *event);
    void updateExpandability(const QList<DockWidget *> &tabs);

private:
    QWidget *m_titleHide;
    DockWidget *m_tabs;
    bool m_closable : 1, m_expandable : 1;
};

#endif
