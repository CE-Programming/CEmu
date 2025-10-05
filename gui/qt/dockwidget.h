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
    void makeCloseableFloat(bool state);
    void setState(bool visible);
    bool isClosable() const { return m_closable; }
    void setClosable(bool closable) { m_closable = closable; }
    bool isExpandable() const { return m_expandable; }
    void setExpandable(bool expandable) { m_expandable = expandable; }

signals:
    void closed();

protected:
    virtual bool event(QEvent *event) Q_DECL_OVERRIDE;
    QList<DockWidget *> tabs(DockWidget *without = Q_NULLPTR);
    virtual void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
    static void updateExpandability(const QList<DockWidget *> &tabs);
    virtual void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private:
    QWidget *m_titleHide;
    DockWidget *m_tabs;
    bool m_closable : 1, m_expandable : 1, m_closeablefloat : 1;
};

#endif
