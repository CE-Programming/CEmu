#ifndef SENDINGHANDLER_H
#define SENDINGHANDLER_H

#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtCore/QObject>

class SendingHandler {
public:
    explicit SendingHandler();

    void sendFiles(QStringList, unsigned);
    bool dragOccured(QDragEnterEvent*);
    void dropOccured(QDropEvent*, unsigned);
};

// Used as global
extern SendingHandler sendingHandler;

#endif
