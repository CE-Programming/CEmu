#ifndef ARMGDBCLIENT_H
#define ARMGDBCLIENT_H

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QTimer>
#include <QtCore/QVector>
#include <QtNetwork/QTcpSocket>

#include <functional>

class ArmGdbClient : public QObject {
    Q_OBJECT

public:
    using ReplyHandler = std::function<void(bool, const QByteArray &)>;

    explicit ArmGdbClient(QObject *parent = nullptr);

    void connectToHost(const QString &host, quint16 port);
    void disconnectFromTarget();
    [[nodiscard]] bool isActive() const;
    [[nodiscard]] bool isReady() const;
    [[nodiscard]] bool isRunning() const;

    void requestRegisters();
    void writeRegister(unsigned int index, quint32 value);
    void readMemory(quint32 address, quint32 size);
    void writeMemory(quint32 address, const QByteArray &data);
    void continueTarget();
    void stepTarget();
    void interruptTarget();
    void resetTarget();
    void addBreakpoint(quint32 address);
    void removeBreakpoint(quint32 address);

signals:
    void ready();
    void disconnected();
    void errorOccurred(const QString &message);
    void targetRunning();
    void targetStopped(unsigned int signal);
    void registersReceived(const QVector<quint32> &registers);
    void registerWritten(unsigned int index, bool success);
    void memoryReceived(quint32 address, const QByteArray &data);
    void memoryWritten(quint32 address, bool success);
    void breakpointChanged(quint32 address, bool inserted, bool success);
    void consoleOutput(const QString &text);

private:
    struct Request {
        QByteArray payload;
        ReplyHandler handler;
        bool waitForStop = false;
    };

    void beginHandshake();
    void enqueue(QByteArray payload, ReplyHandler handler = {}, bool waitForStop = false);
    void sendNext();
    void readSocket();
    void processPacket(const QByteArray &payload);
    void failRequests(const QString &message);
    static QByteArray frame(const QByteArray &payload);
    static QByteArray encodeBytes(const QByteArray &data);
    static bool decodeBytes(const QByteArray &text, QByteArray &data);
    static bool decodeWord(const QByteArray &text, quint32 &value);
    static QByteArray encodeWord(quint32 value);

    QTcpSocket m_socket;
    QTimer m_responseTimer;
    QByteArray m_input;
    QByteArray m_lastFrame;
    QQueue<Request> m_requests;
    Request m_active;
    bool m_hasActive = false;
    bool m_noAck = false;
    bool m_ready = false;
    bool m_running = false;
};

#endif
