#include "armgdbserver.h"

#include "../../core/coproc.h"

#include <QtNetwork/QHostAddress>

#include <array>
#include <limits>

namespace {

constexpr qsizetype MaxPacketSize = 0x1000;
constexpr qsizetype RegisterCount = 17;

const QByteArray TargetXml =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
    "<target>"
    "<architecture>arm</architecture>"
    "<feature name=\"org.gnu.gdb.arm.m-profile\">"
    "<reg name=\"r0\" bitsize=\"32\"/>"
    "<reg name=\"r1\" bitsize=\"32\"/>"
    "<reg name=\"r2\" bitsize=\"32\"/>"
    "<reg name=\"r3\" bitsize=\"32\"/>"
    "<reg name=\"r4\" bitsize=\"32\"/>"
    "<reg name=\"r5\" bitsize=\"32\"/>"
    "<reg name=\"r6\" bitsize=\"32\"/>"
    "<reg name=\"r7\" bitsize=\"32\"/>"
    "<reg name=\"r8\" bitsize=\"32\"/>"
    "<reg name=\"r9\" bitsize=\"32\"/>"
    "<reg name=\"r10\" bitsize=\"32\"/>"
    "<reg name=\"r11\" bitsize=\"32\"/>"
    "<reg name=\"r12\" bitsize=\"32\"/>"
    "<reg name=\"sp\" bitsize=\"32\" type=\"data_ptr\"/>"
    "<reg name=\"lr\" bitsize=\"32\"/>"
    "<reg name=\"pc\" bitsize=\"32\" type=\"code_ptr\"/>"
    "<reg name=\"xpsr\" bitsize=\"32\"/>"
    "</feature>"
    "</target>";

char hexDigit(unsigned int value) {
    return "0123456789abcdef"[value & 15];
}

int hexValue(char value) {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

QByteArray encodeBytes(const uint8_t *data, qsizetype size) {
    QByteArray result(size * 2, Qt::Uninitialized);
    for (qsizetype index = 0; index != size; ++index) {
        result[index * 2] = hexDigit(data[index] >> 4);
        result[index * 2 + 1] = hexDigit(data[index]);
    }
    return result;
}

bool decodeBytes(const QByteArray &text, uint8_t *data, qsizetype size) {
    if (text.size() != size * 2) return false;
    for (qsizetype index = 0; index != size; ++index) {
        const int high = hexValue(text[index * 2]);
        const int low = hexValue(text[index * 2 + 1]);
        if (high < 0 || low < 0) return false;
        data[index] = static_cast<uint8_t>(high << 4 | low);
    }
    return true;
}

void appendWord(QByteArray &output, quint32 value) {
    const std::array<uint8_t, 4> bytes = {
        static_cast<uint8_t>(value),
        static_cast<uint8_t>(value >> 8),
        static_cast<uint8_t>(value >> 16),
        static_cast<uint8_t>(value >> 24),
    };
    output += encodeBytes(bytes.data(), static_cast<qsizetype>(bytes.size()));
}

bool decodeWord(const QByteArray &text, quint32 &value) {
    std::array<uint8_t, 4> bytes{};
    if (!decodeBytes(text, bytes.data(), static_cast<qsizetype>(bytes.size()))) return false;
    value = static_cast<quint32>(bytes[0]) |
            static_cast<quint32>(bytes[1]) << 8 |
            static_cast<quint32>(bytes[2]) << 16 |
            static_cast<quint32>(bytes[3]) << 24;
    return true;
}

bool parseHex(const QByteArray &text, quint32 &value) {
    if (text.isEmpty() || text.size() > 8) return false;
    bool ok = false;
    const qulonglong parsed = text.toULongLong(&ok, 16);
    if (!ok || parsed > (std::numeric_limits<quint32>::max)()) return false;
    value = static_cast<quint32>(parsed);
    return true;
}

bool parseAddressLength(const QByteArray &text, quint32 &address, quint32 &length) {
    const qsizetype comma = text.indexOf(',');
    return comma > 0 && parseHex(text.first(comma), address) &&
           parseHex(text.sliced(comma + 1), length);
}

unsigned int stopSignal(arm_debug_stop_reason_t reason) {
    return reason == ARM_DEBUG_STOP_INTERRUPT ? 2 : 5;
}

class CoprocGuard {
public:
    CoprocGuard() : m_arm(coproc_acquire()) {}
    ~CoprocGuard() { coproc_release(); }

    [[nodiscard]] arm_t *get() const { return m_arm; }

private:
    arm_t *m_arm;
};

} // namespace

ArmGdbServer::ArmGdbServer(Logger logger, QObject *parent)
    : QObject(parent), m_logger(std::move(logger)) {
    connect(&m_server, &QTcpServer::newConnection, this, [this] { acceptConnection(); });
    m_pollTimer.setInterval(5);
    connect(&m_pollTimer, &QTimer::timeout, this, [this] { pollTarget(); });
}

ArmGdbServer::~ArmGdbServer() {
    stop();
}

bool ArmGdbServer::start(quint16 requestedPort) {
    stop();
    if (!m_server.listen(QHostAddress::LocalHost, requestedPort)) {
        log(tr("Could not listen on 127.0.0.1:%1: %2")
                .arg(requestedPort).arg(m_server.errorString()), true);
        return false;
    }
    log(tr("ARM GDB server listening on 127.0.0.1:%1").arg(m_server.serverPort()));
    return true;
}

void ArmGdbServer::stop() {
    m_pollTimer.stop();
    if (m_client) {
        CoprocGuard guard;
        if (guard.get()) arm_debug_detach(guard.get());
        disconnect(m_client, nullptr, this, nullptr);
        m_client->disconnectFromHost();
        m_client->deleteLater();
        m_client = nullptr;
    }
    m_server.close();
    m_input.clear();
    m_noAck = false;
    m_waitingForStop = false;
}

quint16 ArmGdbServer::port() const {
    return m_server.serverPort();
}

bool ArmGdbServer::hasClient() const {
    return !m_client.isNull();
}

void ArmGdbServer::acceptConnection() {
    while (QTcpSocket *connection = m_server.nextPendingConnection()) {
        if (m_client) {
            connection->disconnectFromHost();
            connection->deleteLater();
            continue;
        }
        m_client = connection;
        m_input.clear();
        m_noAck = false;
        m_waitingForStop = false;
        m_lastSignal = 5;
        connect(connection, &QTcpSocket::readyRead, this, [this] { readClient(); });
        connect(connection, &QTcpSocket::disconnected, this, [this] { clientDisconnected(); });
        CoprocGuard guard;
        ensureAttached(guard.get());
        m_pollTimer.start();
        log(tr("ARM GDB client connected"));
    }
}

void ArmGdbServer::clientDisconnected() {
    CoprocGuard guard;
    if (guard.get()) arm_debug_detach(guard.get());
    if (m_client) m_client->deleteLater();
    m_client = nullptr;
    m_pollTimer.stop();
    m_input.clear();
    m_waitingForStop = false;
    log(tr("ARM GDB client disconnected"));
}

void ArmGdbServer::readClient() {
    if (!m_client) return;
    m_input += m_client->readAll();

    while (!m_input.isEmpty()) {
        if (m_input[0] == '+' || m_input[0] == '-') {
            m_input.remove(0, 1);
            continue;
        }
        if (static_cast<unsigned char>(m_input[0]) == 3) {
            m_input.remove(0, 1);
            CoprocGuard guard;
            if (ensureAttached(guard.get()) && arm_debug_interrupt(guard.get())) {
                m_waitingForStop = false;
                m_lastSignal = 2;
                sendStop(m_lastSignal);
            }
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
        if (high < 0 || low < 0 || (checksum & 0xFF) != static_cast<unsigned int>(high << 4 | low)) {
            if (!m_noAck) m_client->write("-", 1);
            continue;
        }
        if (!m_noAck) m_client->write("+", 1);
        processPacket(payload);
    }
}

void ArmGdbServer::pollTarget() {
    if (!m_client || !m_waitingForStop) return;
    CoprocGuard guard;
    arm_t *arm = guard.get();
    if (!ensureAttached(arm)) return;
    bool stopped = false;
    arm_debug_stop_reason_t reason = ARM_DEBUG_STOP_NONE;
    if (arm_debug_status(arm, &stopped, &reason) && stopped) {
        m_waitingForStop = false;
        m_lastSignal = stopSignal(reason);
        sendStop(m_lastSignal);
    }
}

void ArmGdbServer::processPacket(const QByteArray &packet) {
    CoprocGuard guard;
    arm_t *arm = guard.get();
    if (packet == "qSupported" || packet.startsWith("qSupported:")) {
        sendPacket("PacketSize=1000;qXfer:features:read+;swbreak+;vContSupported+;QStartNoAckMode+");
    } else if (packet == "QStartNoAckMode") {
        sendPacket("OK");
        m_noAck = true;
    } else if (packet.startsWith("qXfer:features:read:")) {
        const QByteArray request = packet.sliced(sizeof("qXfer:features:read:") - 1);
        const qsizetype separator = request.indexOf(':');
        quint32 offset = 0, length = 0;
        if (separator < 0 || request.first(separator) != "target.xml" ||
            !parseAddressLength(request.sliced(separator + 1), offset, length) ||
            length >= static_cast<quint32>(MaxPacketSize)) {
            sendError();
            return;
        }
        if (offset >= static_cast<quint32>(TargetXml.size())) {
            sendPacket("l");
            return;
        }
        const quint32 available = static_cast<quint32>(TargetXml.size()) - offset;
        const quint32 count = qMin(length, available);
        QByteArray reply(1, count == available ? 'l' : 'm');
        reply += TargetXml.sliced(offset, count);
        sendPacket(reply);
    } else if (packet == "qAttached") {
        sendPacket("1");
    } else if (packet == "qC") {
        sendPacket("QC1");
    } else if (packet == "qfThreadInfo") {
        sendPacket("m1");
    } else if (packet == "qsThreadInfo") {
        sendPacket("l");
    } else if (packet.startsWith("qSymbol")) {
        sendPacket("OK");
    } else if (packet == "qOffsets") {
        sendPacket("Text=0;Data=0;Bss=0");
    } else if (packet.startsWith("qRcmd,")) {
        const QByteArray encoded = packet.sliced(6);
        QByteArray command(encoded.size() / 2, Qt::Uninitialized);
        if ((encoded.size() & 1) || !decodeBytes(encoded,
                reinterpret_cast<uint8_t *>(command.data()), command.size())) {
            sendError();
        } else if (command == "help") {
            sendConsole("CEmu ARM monitor commands: help, info, reset\n");
            sendPacket("OK");
        } else if (command == "info") {
            if (!ensureAttached(arm)) {
                sendConsole("No ARM coprocessor is active.\n");
            } else {
                arm_cpu_snapshot_t snapshot{};
                char description[ARM_BOOTLOADER_DESCRIPTION_SIZE]{};
                arm_get_cpu_snapshot(arm, &snapshot);
                arm_get_bootloader_info(arm, description, sizeof(description));
                sendConsole(tr("SAMD21E18A, %1, cycles=%2\n")
                                .arg(QString::fromUtf8(description)).arg(snapshot.cycles));
            }
            sendPacket("OK");
        } else if (command == "reset") {
            if (!ensureAttached(arm)) {
                sendError();
            } else {
                arm_reset(arm);
                arm_debug_interrupt(arm);
                m_lastSignal = 5;
                sendPacket("OK");
            }
        } else {
            sendConsole("Unknown command; use 'monitor help'.\n");
            sendError();
        }
    } else if (packet == "vCont?") {
        sendPacket("vCont;c;s");
    } else if (packet.startsWith("vCont;")) {
        const char action = packet.size() > 6 ? packet[6] : '\0';
        if (action == 'c' || action == 's') resume(arm, action == 's');
        else sendPacket("");
    } else if (packet == "?") {
        ensureAttached(arm);
        sendStop(m_lastSignal);
    } else if (packet == "g") {
        arm_debug_registers_t registers{};
        if (!ensureAttached(arm) || !arm_debug_get_registers(arm, &registers)) {
            sendError();
            return;
        }
        QByteArray reply;
        reply.reserve(RegisterCount * 8);
        for (quint32 value : registers.registers) appendWord(reply, value);
        appendWord(reply, registers.xpsr);
        sendPacket(reply);
    } else if (packet.startsWith('G')) {
        if (!ensureAttached(arm) || packet.size() != 1 + RegisterCount * 8) {
            sendError();
            return;
        }
        arm_debug_registers_t registers{};
        bool valid = true;
        for (qsizetype index = 0; index != RegisterCount; ++index) {
            quint32 value = 0;
            valid &= decodeWord(packet.sliced(1 + index * 8, 8), value);
            if (index < 16) registers.registers[index] = value;
            else registers.xpsr = value;
        }
        if (!valid || !arm_debug_set_registers(arm, &registers)) sendError();
        else sendPacket("OK");
    } else if (packet.startsWith('p')) {
        quint32 index = 0;
        arm_debug_registers_t registers{};
        if (!parseHex(packet.sliced(1), index) || index >= RegisterCount ||
            !ensureAttached(arm) || !arm_debug_get_registers(arm, &registers)) {
            sendError();
            return;
        }
        QByteArray reply;
        appendWord(reply, index < 16 ? registers.registers[index] : registers.xpsr);
        sendPacket(reply);
    } else if (packet.startsWith('P')) {
        const qsizetype equal = packet.indexOf('=');
        quint32 index = 0, value = 0;
        arm_debug_registers_t registers{};
        if (equal < 2 || !parseHex(packet.sliced(1, equal - 1), index) ||
            index >= RegisterCount || !decodeWord(packet.sliced(equal + 1), value) ||
            !ensureAttached(arm) || !arm_debug_get_registers(arm, &registers)) {
            sendError();
            return;
        }
        if (index < 16) registers.registers[index] = value;
        else registers.xpsr = value;
        if (!arm_debug_set_registers(arm, &registers)) sendError();
        else sendPacket("OK");
    } else if (packet.startsWith('m')) {
        quint32 address = 0, length = 0;
        if (!parseAddressLength(packet.sliced(1), address, length) ||
            length > static_cast<quint32>(MaxPacketSize / 2) || !ensureAttached(arm)) {
            sendError();
            return;
        }
        QByteArray data(length, Qt::Uninitialized);
        if (!arm_debug_read_memory(arm, address,
                reinterpret_cast<uint8_t *>(data.data()), length)) {
            sendError();
        } else {
            sendPacket(encodeBytes(reinterpret_cast<const uint8_t *>(data.constData()), data.size()));
        }
    } else if (packet.startsWith('M')) {
        const qsizetype colon = packet.indexOf(':');
        quint32 address = 0, length = 0;
        if (colon < 0 || !parseAddressLength(packet.sliced(1, colon - 1), address, length) ||
            length > static_cast<quint32>(MaxPacketSize / 2) ||
            packet.size() - colon - 1 != static_cast<qsizetype>(length) * 2 ||
            !ensureAttached(arm)) {
            sendError();
            return;
        }
        QByteArray data(length, Qt::Uninitialized);
        if (!decodeBytes(packet.sliced(colon + 1),
                reinterpret_cast<uint8_t *>(data.data()), data.size()) ||
            !arm_debug_write_memory(arm, address,
                reinterpret_cast<const uint8_t *>(data.constData()), data.size())) {
            sendError();
        } else {
            sendPacket("OK");
        }
    } else if (packet.startsWith('c') || packet.startsWith('s')) {
        resume(arm, packet[0] == 's', packet.sliced(1));
    } else if (packet.startsWith("Z0,") || packet.startsWith("z0,")) {
        const bool add = packet[0] == 'Z';
        quint32 address = 0, kind = 0;
        if (!parseAddressLength(packet.sliced(3), address, kind) || !ensureAttached(arm) ||
            !(add ? arm_debug_add_breakpoint(arm, address)
                  : arm_debug_remove_breakpoint(arm, address))) {
            sendError();
        } else {
            sendPacket("OK");
        }
    } else if (packet.startsWith('H')) {
        sendPacket("OK");
    } else if (packet.startsWith('T')) {
        sendPacket(packet == "T1" ? "OK" : "E01");
    } else if (packet == "D") {
        if (arm) arm_debug_detach(arm);
        m_waitingForStop = false;
        sendPacket("OK");
    } else if (packet == "k") {
        if (arm) arm_debug_detach(arm);
        m_waitingForStop = false;
    } else if (packet == "!") {
        sendPacket("OK");
    } else {
        sendPacket("");
    }
}

void ArmGdbServer::sendPacket(const QByteArray &payload) {
    if (!m_client) return;
    unsigned int checksum = 0;
    for (const char value : payload) checksum += static_cast<unsigned char>(value);
    QByteArray packet;
    packet.reserve(payload.size() + 4);
    packet += '$';
    packet += payload;
    packet += '#';
    packet += hexDigit(checksum >> 4);
    packet += hexDigit(checksum);
    m_client->write(packet);
}

void ArmGdbServer::sendError(unsigned int code) {
    QByteArray error = "E00";
    error[1] = hexDigit(code >> 4);
    error[2] = hexDigit(code);
    sendPacket(error);
}

void ArmGdbServer::sendStop(unsigned int signal) {
    QByteArray reply = "T00thread:1;";
    reply[1] = hexDigit(signal >> 4);
    reply[2] = hexDigit(signal);
    sendPacket(reply);
}

void ArmGdbServer::sendConsole(const QString &text) {
    const QByteArray bytes = text.toUtf8();
    sendPacket("O" + encodeBytes(reinterpret_cast<const uint8_t *>(bytes.constData()), bytes.size()));
}

bool ArmGdbServer::ensureAttached(arm_t *arm) {
    if (!arm) return false;
    bool stopped = false;
    arm_debug_stop_reason_t reason = ARM_DEBUG_STOP_NONE;
    return arm_debug_status(arm, &stopped, &reason) || arm_debug_attach(arm);
}

bool ArmGdbServer::setProgramCounter(arm_t *arm, quint32 address) {
    arm_debug_registers_t registers{};
    if (!ensureAttached(arm) || !arm_debug_get_registers(arm, &registers)) return false;
    registers.registers[15] = address;
    return arm_debug_set_registers(arm, &registers);
}

void ArmGdbServer::resume(arm_t *arm, bool step, const QByteArray &address) {
    quint32 requestedAddress = 0;
    if ((!address.isEmpty() && (!parseHex(address, requestedAddress) ||
                                !setProgramCounter(arm, requestedAddress))) ||
        !ensureAttached(arm) || !arm_debug_resume(arm, step)) {
        sendError();
        return;
    }
    m_waitingForStop = true;
}

void ArmGdbServer::log(const QString &text, bool error) const {
    if (m_logger) m_logger(text, error);
}
