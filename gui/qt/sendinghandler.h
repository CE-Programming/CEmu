#ifndef SENDINGHANDLER_H
#define SENDINGHANDLER_H

#include <QtWidgets/QProgressBar>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtCore/QObject>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>

class SendingHandler {

public:
    explicit SendingHandler(QProgressBar *bar = Q_NULLPTR, QTableWidget *t = Q_NULLPTR);

    void sendFiles(QStringList, unsigned);
    bool dragOccured(QDragEnterEvent*);
    void dropOccured(QDropEvent*, unsigned);
    void resendSelected();

private:
    QProgressBar *progress;
    QTableWidget *table;
};

// Used as global
extern SendingHandler *sendingHandler;

#endif
