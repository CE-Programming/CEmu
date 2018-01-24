#ifndef PROFILEWIDGET_H
#define PROFILEWIDGET_H

#include <QtWidgets/QWidget>
#include <QtCore/QTimer>
#include <QClipboard>

class ProfileWidget : public QWidget {
  Q_OBJECT

public:
    explicit ProfileWidget(QWidget *p = Q_NULLPTR);
    ~ProfileWidget();

protected:
    virtual void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;

public slots:
    QString setConfig(const QString &config);

private slots:
    void contextMenu(const QPoint &posa);

private:
    QImage image;

    int width = 300;
    int height = 200;

    uint32_t base = 0, top = 0;
};

#endif
