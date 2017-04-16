#ifndef SENDINGHANDLER_H
#define SENDINGHANDLER_H

#include <QtWidgets/QProgressBar>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtCore/QObject>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>

class SendingHandler : public QObject {
    Q_OBJECT

public:
    explicit SendingHandler(QObject *p = Q_NULLPTR, QProgressBar *bar = Q_NULLPTR, QTableWidget *t = Q_NULLPTR);
    ~SendingHandler();

    void sendFiles(const QStringList&, unsigned);
    bool dragOccured(QDragEnterEvent*);
    void dropOccured(QDropEvent*, unsigned);
    void resendSelected();

public slots:
    void sentFile(const QString&, bool);

signals:
    void send(const QStringList&, unsigned int);

private:
    QProgressBar *progress;
    QTableWidget *table;
};

// Used as global
extern SendingHandler *sendingHandler;

#endif
