#ifndef SENDINGHANDLER_H
#define SENDINGHANDLER_H

#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtCore/QObject>

class SendingHandler {
public:
    explicit SendingHandler();
    void sendFiles(QStringList fileNames, unsigned location);
    bool dragOccured(QDragEnterEvent *e);
    void dropOccured(QDropEvent *e, unsigned location);
};

// Used as global
extern SendingHandler sending_handler;

#endif
