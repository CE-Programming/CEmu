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
    enum class SetupResult {
        LocalServer,
        RemoteServer,
        Error,
    };

    explicit InterCom(QObject *p = Q_NULLPTR);
    ~InterCom();

    void serverSetup(const QString &name);
    void clientSetup(const QString &name);
    bool serverListen() const;
    void idClose();
    bool send(const QByteArray &pkt) const;

    QString getServerName();
    QString getClientName();

    SetupResult ipcSetup(const QString &id, const QString &pid);
    SetupResult recover(const QString &pid);
    static bool idOpen(const QString &name);

    QByteArray getData();

signals:
    void readDone();

private:
    void accepted();
    SetupResult createLocalServer(const QString &pid);

    // server
    QLocalServer *m_server;
    QString m_serverName;

    // client
    QLocalSocket *m_socket;
    QString m_clientName;

    // id / storage
    QFile m_file;
    QString m_lockFileName;
    QString m_ownerPid;
    QString m_clientPid;
    bool m_ownsId = false;
    QByteArray m_data;
};

#endif
