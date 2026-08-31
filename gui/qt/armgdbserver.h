#ifndef ARMGDBSERVER_H
#define ARMGDBSERVER_H

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include <functional>

typedef struct arm arm_t;

class ArmGdbServer : public QObject {
public:
    using Logger = std::function<void(const QString &, bool)>;

    explicit ArmGdbServer(Logger logger, QObject *parent = nullptr);
    ~ArmGdbServer() override;

    bool start(quint16 port);
    void stop();
    [[nodiscard]] quint16 port() const;

private:
    void acceptConnection();
    void clientDisconnected();
    void readClient();
    void pollTarget();
    void processPacket(const QByteArray &packet);
    void sendPacket(const QByteArray &payload);
    void sendError(unsigned int code = 1);
    void sendStop(unsigned int signal);
    void sendConsole(const QString &text);
    static bool ensureAttached(arm_t *arm);
    static bool setProgramCounter(arm_t *arm, quint32 address);
    void resume(arm_t *arm, bool step, const QByteArray &address = {});
    void log(const QString &text, bool error = false) const;

    QTcpServer m_server;
    QPointer<QTcpSocket> m_client;
    QTimer m_pollTimer;
    QByteArray m_input;
    Logger m_logger;
    bool m_noAck = false;
    bool m_waitingForStop = false;
    unsigned int m_lastSignal = 5;
};

#endif
