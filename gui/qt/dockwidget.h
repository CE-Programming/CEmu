#ifndef DOCKWIDGET_H
#define DOCKWIDGET_H

#include <QtWidgets/QDockWidget>

QT_BEGIN_NAMESPACE
class QTabWidget;
QT_END_NAMESPACE

class DockWidget : public QDockWidget {
  Q_OBJECT

public:
  explicit DockWidget(QWidget *parent = Q_NULLPTR);
  DockWidget(QTabWidget *tabs, QWidget *parent = Q_NULLPTR);
  DockWidget(const QString &title, QWidget *parent = Q_NULLPTR);

  void toggleState(bool visible);

private:
  QWidget *m_hide_title;
  int m_title_height;
};

#endif
