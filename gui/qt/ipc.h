#ifndef IPC_H
#define IPC_H

#include <QtCore/QFileInfo>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

// different types of available streams
enum {
    IPC_NONE=0,
    IPC_COMMANDLINEOPTIONS
};

class ipc : public QObject {
    Q_OBJECT
public:
    explicit ipc(QObject *p = Q_NULLPTR);
    void serverSetup(QString);
    void clientSetup(QString);
    void serverListen();
    void idClose();
    void send(QByteArray);

    QString getServerName();
    QString getHostName();

    bool ipcSetup(QString id, QString pid);
    static bool idOpen(QString name);

    QByteArray getData();

signals:
    void readDone();

private:
    void accepted();

    // server
    QLocalServer *server;
    bool serverSet = false;
    bool clientSet = false;
    QString serverName;

    QFile file;

    // client
    QLocalSocket *socket;
    QString hostName;
    quint32 blockSize;

    QByteArray data;
};

#endif
