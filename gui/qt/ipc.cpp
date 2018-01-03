#include <QtCore/QDir>
#include <sys/types.h>

#include "ipc.h"
#include "utils.h"

ipc::ipc(QObject *p) : QObject(p) {
    server = new QLocalServer();
    socket = new QLocalSocket();
    connect(server, &QLocalServer::newConnection, this, &ipc::accepted);
}

ipc::~ipc() {
    delete socket;
    delete server;
}

void ipc::idClose() {
    socket->disconnectFromServer();
    server->close();
    file.remove();
}

void ipc::serverListen() {
    if (serverName.isEmpty()) {
        return;
    }
    server->close();
    if (!server->listen(serverName)) {
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
    if (clientName.isEmpty()) {
        return;
    }
    socket->disconnectFromServer();
    socket->connectToServer(clientName);
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
    clientName = "cemu-" + name;
}

void ipc::serverSetup(const QString& name) {
    serverName = "cemu-" + name;
}

QString ipc::getClientName() {
    return clientName;
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
