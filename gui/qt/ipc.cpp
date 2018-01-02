#include <QtCore/QDir>
#include <sys/types.h>

#include "ipc.h"
#include "utils.h"

ipc::ipc(QObject *p) : QObject(p) {
    server = new QLocalServer(this);
    socket = new QLocalSocket(this);
    connect(server, &QLocalServer::newConnection, this, &ipc::accepted);
}

void ipc::idClose() {
    socket->abort();
    socket->close();
    server->close();
    file.remove();
}

void ipc::serverListen() {
    if (!serverSet) {
        return;
    }
    server->close();
    if(!server->listen(serverName)) {
        qDebug() << "err: " << server->errorString();
    }
}

void ipc::accepted() {
    socket = server->nextPendingConnection();
    if (socket->waitForReadyRead()) {
        data = socket->readAll();
        socket->disconnectFromServer();
        emit readDone();
    } else {
        qDebug() << "err: receiving packet";
    }
}

QByteArray ipc::getData() {
    return data;
}

void ipc::send(const QByteArray &pkt) {
    if (!clientSet) {
        return;
    }
    socket->abort();
    socket->connectToServer(hostName);
    if (socket->waitForConnected()) {
        socket->write(pkt);
        if (!socket->waitForDisconnected()) {
            qDebug() << "err: sending packet";
        }
    } else {
        qDebug() << "err: connection timed out";
    }
}

void ipc::clientSetup(const QString& name) {
    hostName = QStringLiteral("cemu-") + name;
    clientSet = true;
}

void ipc::serverSetup(const QString& name) {
    serverName = QStringLiteral("cemu-") + name;
    serverSet = true;
}

QString ipc::getHostName() {
    return hostName;
}

QString ipc::getServerName() {
    return serverName;
}

bool ipc::idOpen(const QString& name) {
    QString idPath = configPath + "/id/";
    QString idFile = idPath + name;
    return QFile(idFile).exists();
}

bool ipc::ipcSetup(const QString& id, const QString& pid) {
    bool ret = true;

    // find the default configuration path
    QString idPath = configPath + "/id/";
    QString idFile = idPath + id;

    QDir config;
    config.mkpath(idPath);

    file.setFileName(idFile);
    if (file.exists()) {
    // send to alternate id
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream stream(&file);
            QString pidtest = stream.readLine();
            if (!isProcRunning(static_cast<pid_t>(pidtest.toLongLong()))) {
                file.close();
                file.remove();
                goto create_id;
            }
            clientSetup(pidtest);
            ret = false;
        }
    } else {
    // create a local id
    create_id:
        if (file.open(QIODevice::WriteOnly)) {
            QTextStream stream(&file);
            stream << pid << endl;

            serverSetup(pid);
            serverListen();
        }
    }
    file.close();
    return ret;
}
