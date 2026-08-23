#include "ipc.h"
#include "utils.h"

#include <QtCore/QDir>
#include <QtCore/QLockFile>
#include <QtCore/QSaveFile>

namespace {

QString readPid(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const QString pid = QTextStream(&file).readLine();
    bool ok;
    const qlonglong value = pid.toLongLong(&ok);
    return ok && value > 0 ? pid : QString{};
}

bool writePid(const QString &fileName, const QString &pid) {
    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    QTextStream(&file) << pid << Qt::endl;
    return file.commit();
} // namespace

}

InterCom::InterCom(QObject *parent) : QObject{parent} {
    m_server = new QLocalServer();
    m_socket = new QLocalSocket();
    connect(m_server, &QLocalServer::newConnection, this, &InterCom::accepted);
}

InterCom::~InterCom() {
    idClose();
    delete m_socket;
    delete m_server;
}

void InterCom::idClose() {
    m_socket->disconnectFromServer();
    m_server->close();
    if (m_ownsId) {
        QLockFile lock(m_lockFileName);
        if (lock.tryLock(5000) && readPid(m_file.fileName()) == m_ownerPid) {
            m_file.remove();
        }
    }
    m_ownsId = false;
    m_ownerPid.clear();
    m_clientPid.clear();
    m_clientName.clear();
    m_serverName.clear();
}

bool InterCom::serverListen() const {
    if (m_serverName.isEmpty()) {
        return false;
    }
    m_server->close();
    if (m_server->listen(m_serverName)) {
        return true;
    }
    QLocalServer::removeServer(m_serverName);
    if (m_server->listen(m_serverName)) {
        return true;
    }
    qDebug() << "err: " << m_server->errorString();
    return false;
}

void InterCom::accepted() {
    m_socket = m_server->nextPendingConnection();
    if (m_socket->waitForReadyRead()) {
        m_data = m_socket->readAll();
        m_socket->disconnectFromServer();
        emit readDone();
    } else {
        qDebug() << "err: receiving packet";
    }
}

QByteArray InterCom::getData() {
    return m_data;
}

bool InterCom::send(const QByteArray &pkt) const {
    if (m_clientName.isEmpty()) {
        return false;
    }
    m_socket->disconnectFromServer();
    m_socket->connectToServer(m_clientName);
    if (!m_socket->waitForConnected()) {
        qDebug() << "err: connection timed out";
        return false;
    }
    if (m_socket->write(pkt) != pkt.size()) {
        qDebug() << "err: writing packet";
        return false;
    }
    while (m_socket->bytesToWrite() && m_socket->waitForBytesWritten()) {
    }
    if (m_socket->bytesToWrite()) {
        qDebug() << "err: writing packet";
        return false;
    }
    if (m_socket->state() != QLocalSocket::UnconnectedState &&
        !m_socket->waitForDisconnected()) {
        qDebug() << "err: sending packet";
        return false;
    }
    return true;
}

void InterCom::clientSetup(const QString &name) {
    m_clientName = "cemu-" + name;
    m_clientPid = name;
}

void InterCom::serverSetup(const QString &name) {
    m_serverName = "cemu-" + name;
}

QString InterCom::getClientName() {
    return m_clientName;
}

QString InterCom::getServerName() {
    return m_serverName;
}

bool InterCom::idOpen(const QString &name) {
    QString idPath = configPath + QStringLiteral("/id/");
    QString idFile = idPath + name;
    return QFile(idFile).exists();
}

InterCom::SetupResult InterCom::ipcSetup(const QString &id, const QString &pid) {
    // find the default configuration path
    const QString idPath = configPath + QStringLiteral("/id/");
    const QString idFile = idPath + id;
    const QString lockPath = configPath + QStringLiteral("/id-lock/");

    QDir config;
    if (!config.mkpath(idPath) || !config.mkpath(lockPath)) {
        return SetupResult::Error;
    }

    m_file.setFileName(idFile);
    m_lockFileName = lockPath + id;
    QLockFile lock(m_lockFileName);
    if (!lock.tryLock(5000)) {
        return SetupResult::Error;
    }

    const QString existingPid = readPid(idFile);
    if (!existingPid.isEmpty() && existingPid != pid) {
        clientSetup(existingPid);
        return SetupResult::RemoteServer;
    }

    return createLocalServer(pid);
}

InterCom::SetupResult InterCom::recover(const QString &pid) {
    QLockFile lock(m_lockFileName);
    if (!lock.tryLock(5000)) {
        return SetupResult::Error;
    }

    const QString existingPid = readPid(m_file.fileName());
    if (!existingPid.isEmpty() && existingPid != m_clientPid && existingPid != pid) {
        clientSetup(existingPid);
        return SetupResult::RemoteServer;
    }

    return createLocalServer(pid);
}

InterCom::SetupResult InterCom::createLocalServer(const QString &pid) {
    serverSetup(pid);
    if (!serverListen() || !writePid(m_file.fileName(), pid)) {
        m_server->close();
        return SetupResult::Error;
    }
    m_clientName.clear();
    m_clientPid.clear();
    m_ownerPid = pid;
    m_ownsId = true;
    return SetupResult::LocalServer;
}
