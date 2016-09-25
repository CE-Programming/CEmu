#ifndef SENDINGHANDLER_H
#define SENDINGHANDLER_H

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QObject>

class SendingHandler {
public:
    SendingHandler();
    void sendFiles(QStringList fileNames, unsigned location);
    bool dragOccured(QDragEnterEvent *e);
    void dropOccured(QDropEvent *e, unsigned location);
};

// Used as global
extern SendingHandler sending_handler;

#endif
