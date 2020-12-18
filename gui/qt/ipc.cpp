#include "ipc.h"
#include "utils.h"

#include <QtCore/QDir>
#include <sys/types.h>

InterCom::InterCom(QObject *parent) : QObject{parent} {
    m_server = new QLocalServer();
    m_socket = new QLocalSocket();
    connect(m_server, &QLocalServer::newConnection, this, &InterCom::accepted);
}

InterCom::~InterCom() {
    delete m_socket;
    delete m_server;
}

void InterCom::idClose() {
    m_socket->disconnectFromServer();
    m_server->close();
    m_file.remove();
}

void InterCom::serverListen() {
    if (m_serverName.isEmpty()) {
        return;
    }
    m_server->close();
    if (!m_server->listen(m_serverName)) {
        qDebug() << "err: " << m_server->errorString();
    }
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

void InterCom::send(const QByteArray &pkt) {
    if (m_clientName.isEmpty()) {
        return;
    }
    m_socket->disconnectFromServer();
    m_socket->connectToServer(m_clientName);
    if (m_socket->waitForConnected()) {
        m_socket->write(pkt);
        if (!m_socket->waitForDisconnected()) {
            qDebug() << "err: sending packet";
        }
    } else {
        qDebug() << "err: connection timed out";
    }
}

void InterCom::clientSetup(const QString &name) {
    m_clientName = "cemu-" + name;
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

bool InterCom::ipcSetup(const QString &id, const QString &pid) {
    bool ret = true;

    // find the default configuration path
    QString idPath = configPath + QStringLiteral("/id/");
    QString idFile = idPath + id;

    QDir config;
    config.mkpath(idPath);

    m_file.setFileName(idFile);
    if (m_file.exists()) {
        // send to alternate id
        if (m_file.open(QIODevice::ReadOnly)) {
            QTextStream stream(&m_file);
            QString pidtest = stream.readLine();
            if (!isProcRunning(static_cast<pid_t>(pidtest.toLongLong()))) {
                m_file.close();
                m_file.remove();
                goto create_id;
            }
            clientSetup(pidtest);
            ret = false;
        }
    } else {
    // create a local id
create_id:
        if (m_file.open(QIODevice::WriteOnly)) {
            QTextStream stream(&m_file);
            stream << pid <<
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
              Qt::endl;
#else
              endl;
#endif

            serverSetup(pid);
            serverListen();
        }
    }
    m_file.close();
    return ret;
}
