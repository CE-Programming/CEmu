#include "armgdbclient.h"

#include <QtNetwork/QAbstractSocket>

#include <array>
#include <limits>

namespace {

char hexDigit(unsigned int value) {
    return "0123456789abcdef"[value & 15];
}

int hexValue(char value) {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

bool parseStopSignal(const QByteArray &payload, unsigned int &signal) {
    if (payload.size() < 3 || (payload[0] != 'S' && payload[0] != 'T')) return false;
    const int high = hexValue(payload[1]);
    const int low = hexValue(payload[2]);
    if (high < 0 || low < 0) return false;
    signal = static_cast<unsigned int>(high << 4 | low);
    return true;
}

} // namespace

ArmGdbClient::ArmGdbClient(QObject *parent) : QObject(parent) {
    m_responseTimer.setSingleShot(true);
    m_responseTimer.setInterval(3000);
    connect(&m_responseTimer, &QTimer::timeout, this, [this] {
        failRequests(tr("ARM GDB request timed out"));
        m_socket.abort();
    });
    connect(&m_socket, &QTcpSocket::connected, this, [this] { beginHandshake(); });
    connect(&m_socket, &QTcpSocket::readyRead, this, [this] { readSocket(); });
    connect(&m_socket, &QTcpSocket::disconnected, this, [this] {
        const bool wasConnected = m_ready || m_hasActive || !m_requests.isEmpty();
        m_ready = false;
        m_running = false;
        m_noAck = false;
        m_input.clear();
        failRequests({});
        if (wasConnected) emit disconnected();
    });
    connect(&m_socket, &QTcpSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError error) {
        if (error != QAbstractSocket::RemoteHostClosedError) {
            emit errorOccurred(m_socket.errorString());
        }
    });
}

void ArmGdbClient::connectToHost(const QString &host, quint16 port) {
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.abort();
    }
    m_ready = false;
    m_running = false;
    m_noAck = false;
    m_input.clear();
    m_requests.clear();
    m_hasActive = false;
    m_socket.connectToHost(host, port);
}

void ArmGdbClient::disconnectFromTarget() {
    if (m_socket.state() == QAbstractSocket::UnconnectedState) return;
    enqueue("D", [this](bool, const QByteArray &) { m_socket.disconnectFromHost(); });
}

bool ArmGdbClient::isActive() const {
    return m_socket.state() != QAbstractSocket::UnconnectedState;
}

bool ArmGdbClient::isReady() const {
    return m_ready;
}

bool ArmGdbClient::isRunning() const {
    return m_running;
}

void ArmGdbClient::beginHandshake() {
    enqueue("qSupported:PacketSize=1000;qXfer:features:read+;swbreak+;vContSupported+;QStartNoAckMode+",
            [this](bool success, const QByteArray &reply) {
        if (!success || !reply.contains("QStartNoAckMode+")) {
            failRequests(tr("ARM GDB server does not support the required protocol"));
            m_socket.abort();
            return;
        }
        enqueue("QStartNoAckMode", [this](bool noAckSuccess, const QByteArray &noAckReply) {
            if (!noAckSuccess || noAckReply != "OK") {
                failRequests(tr("Could not enable ARM GDB no-ack mode"));
                m_socket.abort();
                return;
            }
            m_noAck = true;
            m_ready = true;
            emit ready();
            enqueue("?", {});
        });
    });
}

void ArmGdbClient::enqueue(QByteArray payload, ReplyHandler handler, bool waitForStop) {
    m_requests.enqueue({std::move(payload), std::move(handler), waitForStop});
    sendNext();
}

void ArmGdbClient::sendNext() {
    if (m_hasActive || m_requests.isEmpty() ||
        m_socket.state() != QAbstractSocket::ConnectedState) return;
    m_active = m_requests.dequeue();
    m_hasActive = true;
    m_lastFrame = frame(m_active.payload);
    m_socket.write(m_lastFrame);
    if (!m_active.waitForStop) m_responseTimer.start();
}

void ArmGdbClient::readSocket() {
    m_input += m_socket.readAll();
    while (!m_input.isEmpty()) {
        if (m_input[0] == '+' || m_input[0] == '-') {
            const bool resend = m_input[0] == '-';
            m_input.remove(0, 1);
            if (resend && !m_lastFrame.isEmpty()) m_socket.write(m_lastFrame);
            continue;
        }
        const qsizetype start = m_input.indexOf('$');
        if (start < 0) {
            m_input.clear();
            return;
        }
        if (start) m_input.remove(0, start);
        const qsizetype hash = m_input.indexOf('#', 1);
        if (hash < 0 || m_input.size() < hash + 3) return;

        const QByteArray payload = m_input.sliced(1, hash - 1);
        const int high = hexValue(m_input[hash + 1]);
        const int low = hexValue(m_input[hash + 2]);
        unsigned int checksum = 0;
        for (const char value : payload) checksum += static_cast<unsigned char>(value);
        m_input.remove(0, hash + 3);
        if (high < 0 || low < 0 || (checksum & 0xFF) !=
            static_cast<unsigned int>(high << 4 | low)) {
            if (!m_noAck) m_socket.write("-", 1);
            continue;
        }
        if (!m_noAck) m_socket.write("+", 1);
        processPacket(payload);
    }
}

void ArmGdbClient::processPacket(const QByteArray &payload) {
    if (payload.startsWith('O')) {
        QByteArray output;
        if (decodeBytes(payload.sliced(1), output)) {
            emit consoleOutput(QString::fromUtf8(output));
            return;
        }
    }

    unsigned int signal = 0;
    const bool stopped = parseStopSignal(payload, signal);
    if (stopped) {
        m_running = false;
        emit targetStopped(signal);
    }

    if (!m_hasActive) return;
    m_responseTimer.stop();
    ReplyHandler handler = std::move(m_active.handler);
    m_hasActive = false;
    if (handler) handler(!payload.startsWith('E'), payload);
    sendNext();
}

void ArmGdbClient::failRequests(const QString &message) {
    m_responseTimer.stop();
    ReplyHandler activeHandler;
    if (m_hasActive) activeHandler = std::move(m_active.handler);
    m_hasActive = false;
    QQueue<Request> requests = std::move(m_requests);
    m_requests.clear();
    if (activeHandler) activeHandler(false, {});
    while (!requests.isEmpty()) {
        Request request = requests.dequeue();
        if (request.handler) request.handler(false, {});
    }
    if (!message.isEmpty()) emit errorOccurred(message);
}

QByteArray ArmGdbClient::frame(const QByteArray &payload) {
    unsigned int checksum = 0;
    for (const char value : payload) checksum += static_cast<unsigned char>(value);
    QByteArray result;
    result.reserve(payload.size() + 4);
    result += '$';
    result += payload;
    result += '#';
    result += hexDigit(checksum >> 4);
    result += hexDigit(checksum);
    return result;
}

QByteArray ArmGdbClient::encodeBytes(const QByteArray &data) {
    QByteArray result(data.size() * 2, Qt::Uninitialized);
    for (qsizetype index = 0; index != data.size(); ++index) {
        const auto value = static_cast<unsigned char>(data[index]);
        result[index * 2] = hexDigit(value >> 4);
        result[index * 2 + 1] = hexDigit(value);
    }
    return result;
}

bool ArmGdbClient::decodeBytes(const QByteArray &text, QByteArray &data) {
    if (text.size() & 1) return false;
    data.resize(text.size() / 2);
    for (qsizetype index = 0; index != data.size(); ++index) {
        const int high = hexValue(text[index * 2]);
        const int low = hexValue(text[index * 2 + 1]);
        if (high < 0 || low < 0) return false;
        data[index] = static_cast<char>(high << 4 | low);
    }
    return true;
}

bool ArmGdbClient::decodeWord(const QByteArray &text, quint32 &value) {
    QByteArray data;
    if (!decodeBytes(text, data) || data.size() != 4) return false;
    value = static_cast<unsigned char>(data[0]) |
            static_cast<quint32>(static_cast<unsigned char>(data[1])) << 8 |
            static_cast<quint32>(static_cast<unsigned char>(data[2])) << 16 |
            static_cast<quint32>(static_cast<unsigned char>(data[3])) << 24;
    return true;
}

QByteArray ArmGdbClient::encodeWord(quint32 value) {
    QByteArray data(4, Qt::Uninitialized);
    for (unsigned int index = 0; index != 4; ++index) {
        data[index] = static_cast<char>(value >> (index * 8));
    }
    return encodeBytes(data);
}

void ArmGdbClient::requestRegisters() {
    enqueue("g", [this](bool success, const QByteArray &reply) {
        QVector<quint32> registers;
        if (success && reply.size() == 17 * 8) {
            registers.resize(17);
            for (qsizetype index = 0; index != registers.size(); ++index) {
                if (!decodeWord(reply.sliced(index * 8, 8), registers[index])) {
                    registers.clear();
                    break;
                }
            }
        }
        if (registers.isEmpty()) emit errorOccurred(tr("Could not read ARM registers"));
        else emit registersReceived(registers);
    });
}

void ArmGdbClient::writeRegister(unsigned int index, quint32 value) {
    enqueue("P" + QByteArray::number(index, 16) + "=" + encodeWord(value),
            [this, index](bool success, const QByteArray &reply) {
        success = success && reply == "OK";
        emit registerWritten(index, success);
    });
}

void ArmGdbClient::readMemory(quint32 address, quint32 size) {
    enqueue("m" + QByteArray::number(address, 16) + "," + QByteArray::number(size, 16),
            [this, address, size](bool success, const QByteArray &reply) {
        QByteArray data;
        if (!success || !decodeBytes(reply, data) || data.size() != size) {
            emit errorOccurred(tr("Could not read ARM memory at 0x%1")
                               .arg(address, 8, 16, QLatin1Char('0')));
            return;
        }
        emit memoryReceived(address, data);
    });
}

void ArmGdbClient::writeMemory(quint32 address, const QByteArray &data) {
    enqueue("M" + QByteArray::number(address, 16) + "," +
            QByteArray::number(data.size(), 16) + ":" + encodeBytes(data),
            [this, address](bool success, const QByteArray &reply) {
        emit memoryWritten(address, success && reply == "OK");
    });
}

void ArmGdbClient::continueTarget() {
    m_running = true;
    emit targetRunning();
    enqueue("c", {}, true);
}

void ArmGdbClient::stepTarget() {
    m_running = true;
    emit targetRunning();
    enqueue("s", {}, true);
}

void ArmGdbClient::interruptTarget() {
    if (m_socket.state() == QAbstractSocket::ConnectedState && m_running) {
        m_socket.write(QByteArray(1, '\x03'));
    }
}

void ArmGdbClient::resetTarget() {
    enqueue("qRcmd," + encodeBytes("reset"), [this](bool success, const QByteArray &reply) {
        if (!success || reply != "OK") emit errorOccurred(tr("Could not reset ARM target"));
        else enqueue("?");
    });
}

void ArmGdbClient::addBreakpoint(quint32 address) {
    enqueue("Z0," + QByteArray::number(address, 16) + ",2",
            [this, address](bool success, const QByteArray &reply) {
        emit breakpointChanged(address, true, success && reply == "OK");
    });
}

void ArmGdbClient::removeBreakpoint(quint32 address) {
    enqueue("z0," + QByteArray::number(address, 16) + ",2",
            [this, address](bool success, const QByteArray &reply) {
        emit breakpointChanged(address, false, success && reply == "OK");
    });
}
