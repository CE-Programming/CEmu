#ifndef IPC_H
#define IPC_H

#include <QtCore/QFileInfo>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

// different types of available streams
enum {
    IPC_NONE=0,
    IPC_CLI,
    IPC_CLOSE
};

class InterCom : public QObject {
    Q_OBJECT

public:
    explicit InterCom(QObject *p = Q_NULLPTR);
    ~InterCom();

    void serverSetup(const QString &name);
    void clientSetup(const QString &name);
    void serverListen();
    void idClose();
    void send(const QByteArray &pkt);

    QString getServerName();
    QString getClientName();

    bool ipcSetup(const QString &id, const QString &pid);
    static bool idOpen(const QString &name);

    QByteArray getData();

signals:
    void readDone();

private:
    void accepted();

    // server
    QLocalServer *m_server;
    QString m_serverName;

    // client
    QLocalSocket *m_socket;
    QString m_clientName;

    // id / storage
    QFile m_file;
    QByteArray m_data;
};

#endif
