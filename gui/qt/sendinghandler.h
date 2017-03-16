#ifndef SENDINGHANDLER_H
#define SENDINGHANDLER_H

#include <QtWidgets/QProgressBar>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtCore/QObject>

class SendingHandler {
public:
    explicit SendingHandler(QProgressBar *bar = Q_NULLPTR);

    void sendFiles(QStringList, unsigned);
    bool dragOccured(QDragEnterEvent*);
    void dropOccured(QDropEvent*, unsigned);

private:
    QProgressBar *progress;
};

// Used as global
extern SendingHandler *sendingHandler;

#endif
