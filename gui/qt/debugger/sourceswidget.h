#ifndef SOURCESWIDGET_H
#define SOURCESWIDGET_H

#include <QtCore/QMap>
#include <QtGui/QFont>
#include <QtGui/QTextCursor>
#include <QtWidgets/QWidget>
class QPushButton;
class QTabWidget;

class SourcesWidget : public QWidget {
  Q_OBJECT

public:
  SourcesWidget(QWidget * = Q_NULLPTR);
  void setSourceFont(const QFont &font);

signals:
  void breakToggled(quint32 addr, bool gui = true);

public slots:
  void selectDebugFile();
  void updatePC(quint32 pc);

private slots:
  void sourceContextMenu(const QPoint &pos);

private:
  QPushButton *m_button;
  QTabWidget *m_tabs;
  QTextCursor m_pcCursor;
  QFont m_sourceFont;
  QMap<quint32, quint32> m_addrToLoc, m_locToAddr;
  static const quint32 s_lastLine = (1 << 24) - 1;
};

#endif
