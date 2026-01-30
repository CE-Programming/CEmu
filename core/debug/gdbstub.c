#include "gdbstub.h"

#if defined(DEBUG_SUPPORT) && !defined(__EMSCRIPTEN__)

#include "debug.h"
#include "../cpu.h"
#include "../emu.h"
#include "../mem.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#define GDB_BUF_MAX 4096
#define GDB_DEFAULT_PORT 1234

static int listen_socket_fd = -1;
static int socket_fd = -1;
static bool gdb_connected = false;
static bool gdb_handshake_complete = false;
static bool gdb_in_debugger = false;
static bool gdb_no_ack = false;

static char remcomInBuffer[GDB_BUF_MAX];
static char remcomOutBuffer[GDB_BUF_MAX];

static const char hexchars[] = "0123456789abcdef";

static void gdbstub_disconnect(void);

static void log_socket_error(const char *msg) {
#ifdef __MINGW32__
    int errCode = WSAGetLastError();
    LPSTR errString = NULL;  // allocated by FormatMessage
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   0, errCode, 0, (LPSTR)&errString, 0, 0);
    gui_console_err_printf("[CEmu][GDB] %s: %s (%i)\n", msg, errString, errCode);
    LocalFree(errString);
#else
    gui_console_err_printf("[CEmu][GDB] %s: %s\n", msg, strerror(errno));
#endif
}

static void set_nonblocking(int socket, bool nonblocking) {
#ifdef __MINGW32__
    u_long mode = nonblocking;
    ioctlsocket(socket, FIONBIO, &mode);
#else
    int ret = fcntl(socket, F_GETFL, 0);
    fcntl(socket, F_SETFL, nonblocking ? (ret | O_NONBLOCK) : (ret & ~O_NONBLOCK));
    ret = fcntl(socket, F_GETFD, 0);
    fcntl(socket, F_SETFD, ret | FD_CLOEXEC);
#endif
}

static int socket_can_read(int fd, int timeout_ms) {
#ifdef __MINGW32__
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    struct timeval timeout = {
        .tv_sec = timeout_ms / 1000,
        .tv_usec = (timeout_ms % 1000) * 1000,
    };
    return select(fd + 1, &rfds, NULL, NULL, &timeout);
#else
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    return poll(&pfd, 1, timeout_ms);
#endif
}

static char sockbuf[4096];
static char *sockbufptr = sockbuf;

static bool flush_out_buffer(void) {
#ifndef MSG_NOSIGNAL
    #ifdef __APPLE__
        #define MSG_NOSIGNAL SO_NOSIGPIPE
    #else
        #define MSG_NOSIGNAL 0
    #endif
#endif
    char *p = sockbuf;
    while (p != sockbufptr) {
        int n = send(socket_fd, p, (int)(sockbufptr - p), MSG_NOSIGNAL);
        if (n == -1) {
#ifdef __MINGW32__
            if (WSAGetLastError() == WSAEWOULDBLOCK)
#else
            if (errno == EAGAIN)
#endif
                continue;
            log_socket_error("Failed to send to GDB socket");
            return false;
        }
        p += n;
    }
    sockbufptr = sockbuf;
    return true;
}

static bool put_debug_char(char c) {
    if (sockbufptr == sockbuf + sizeof sockbuf) {
        if (!flush_out_buffer()) {
            return false;
        }
    }
    *sockbufptr++ = c;
    return true;
}

static char get_debug_char(void) {
    while (true) {
        if (socket_fd == -1) {
            return -1;
        }
        if (cpu_check_signals() & CPU_SIGNAL_EXIT) {
            return -1;
        }
        int p = socket_can_read(socket_fd, 100);
        if (p == -1) {
            log_socket_error("Failed to poll GDB socket");
            return -1;
        }
        if (p) {
            break;
        }
    }

    char c;
    int r = recv(socket_fd, &c, 1, 0);
    if (r <= 0) {
        return -1;
    }
    return c;
}

static int hex_to_int(char ch) {
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

static bool hex_to_uint(const char **ptr, uint32_t *out) {
    uint32_t value = 0;
    int digit;
    const char *p = *ptr;
    if ((digit = hex_to_int(*p)) < 0) {
        return false;
    }
    while ((digit = hex_to_int(*p)) >= 0) {
        value = (value << 4) | (uint32_t)digit;
        p++;
    }
    *out = value;
    *ptr = p;
    return true;
}

static char *append_hex_byte(char *buf, uint8_t value) {
    *buf++ = hexchars[(value >> 4) & 0xF];
    *buf++ = hexchars[value & 0xF];
    return buf;
}

static char *append_hex_le(char *buf, uint32_t value, unsigned bytes) {
    for (unsigned i = 0; i < bytes; i++) {
        buf = append_hex_byte(buf, (uint8_t)(value & 0xFF));
        value >>= 8;
    }
    *buf = '\0';
    return buf;
}

static bool parse_hex_le(const char *buf, unsigned bytes, uint32_t *out_value) {
    uint32_t value = 0;
    for (unsigned i = 0; i < bytes; i++) {
        int hi = hex_to_int(*buf++);
        int lo = hex_to_int(*buf++);
        if (hi < 0 || lo < 0) {
            return false;
        }
        value |= (uint32_t)((hi << 4) | lo) << (i * 8);
    }
    *out_value = value;
    return true;
}

static char *getpacket(void) {
    char *buffer = remcomInBuffer;
    unsigned char checksum;
    unsigned char xmitcsum;
    int count;
    char ch;

    while (1) {
        do {
            ch = get_debug_char();
            if (ch == (char)-1) {
                return NULL;
            }
            if (ch == '\x03') {
                // async interrupt
                buffer[0] = 0;
                return buffer;
            }
        } while (ch != '$');

    retry:
        checksum = 0;
        count = 0;

        while (count < GDB_BUF_MAX - 1) {
            ch = get_debug_char();
            if (ch == (char)-1) {
                return NULL;
            }
            if (ch == '$') {
                goto retry;
            }
            if (ch == '#') {
                break;
            }
            buffer[count++] = ch;
            checksum += (unsigned char)ch;
        }

        if (ch == '#') {
            buffer[count] = 0;
            ch = get_debug_char();
            if (ch == (char)-1) {
                return NULL;
            }
            xmitcsum = (unsigned char)(hex_to_int(ch) << 4);
            ch = get_debug_char();
            if (ch == (char)-1) {
                return NULL;
            }
            xmitcsum += (unsigned char)hex_to_int(ch);

            if (checksum != xmitcsum) {
                if (!gdb_no_ack) {
                    if (!put_debug_char('-') || !flush_out_buffer()) {
                        return NULL;
                    }
                }
                continue;
            }

            if (!gdb_no_ack) {
                if (!put_debug_char('+') || !flush_out_buffer()) {
                    return NULL;
                }
            }
            return buffer;
        }
    }
}

static bool putpacket(const char *buffer) {
    unsigned char checksum;
    int count;
    char ch;

    do {
        if (!put_debug_char('$')) {
            return false;
        }

        checksum = 0;
        count = 0;

        while ((ch = buffer[count])) {
            if (!put_debug_char(ch)) {
                return false;
            }
            checksum += (unsigned char)ch;
            count++;
        }

        if (!put_debug_char('#')
            || !put_debug_char(hexchars[checksum >> 4])
            || !put_debug_char(hexchars[checksum & 0xF])
            || !flush_out_buffer()) {
            return false;
        }

        if (gdb_no_ack) {
            return true;
        }

        ch = get_debug_char();
    } while (ch != '+' && ch != (char)-1);

    return ch != (char)-1;
}

static char *mem2hex_addr(uint32_t addr, char *buf, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        uint8_t byte = mem_peek_byte((addr + i) & 0xFFFFFFu);
        buf = append_hex_byte(buf, byte);
    }
    *buf = '\0';
    return buf;
}

static bool hex2mem_addr(const char *buf, uint32_t addr, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        int hi = hex_to_int(*buf++);
        int lo = hex_to_int(*buf++);
        if (hi < 0 || lo < 0) {
            return false;
        }
        uint8_t byte = (uint8_t)((hi << 4) | lo);
        mem_poke_byte((addr + i) & 0xFFFFFFu, byte);
    }
    return true;
}

enum gdb_ez80_reg_index {
    GDB_REG_AF = 0,
    GDB_REG_BC,
    GDB_REG_DE,
    GDB_REG_HL,
    GDB_REG_SP,
    GDB_REG_PC,
    GDB_REG_IX,
    GDB_REG_IY,
    GDB_REG_AF_ALT,
    GDB_REG_BC_ALT,
    GDB_REG_DE_ALT,
    GDB_REG_HL_ALT,
    GDB_REG_IR,
    GDB_REG_SPS,
    GDB_EZ80_REG_COUNT
};

static const uint8_t gdb_ez80_reg_bytes[GDB_EZ80_REG_COUNT] = {
    [GDB_REG_AF] = 3,
    [GDB_REG_BC] = 3,
    [GDB_REG_DE] = 3,
    [GDB_REG_HL] = 3,
    [GDB_REG_SP] = 3,
    [GDB_REG_PC] = 3,
    [GDB_REG_IX] = 3,
    [GDB_REG_IY] = 3,
    [GDB_REG_AF_ALT] = 3,
    [GDB_REG_BC_ALT] = 3,
    [GDB_REG_DE_ALT] = 3,
    [GDB_REG_HL_ALT] = 3,
    [GDB_REG_IR] = 3,
    [GDB_REG_SPS] = 3,
};

static uint32_t gdb_get_reg(unsigned reg) {
    switch (reg) {
        case GDB_REG_AF: return ((uint32_t)cpu.registers.MBASE << 16) | cpu.registers.AF;
        case GDB_REG_BC: return cpu.registers.BC & 0xFFFFFFu;
        case GDB_REG_DE: return cpu.registers.DE & 0xFFFFFFu;
        case GDB_REG_HL: return cpu.registers.HL & 0xFFFFFFu;
        case GDB_REG_SP:
            return cpu_address_mode(cpu.registers.stack[cpu.L].hl, cpu.L) & 0xFFFFFFu;
        case GDB_REG_PC:
            return cpu_address_mode(cpu.registers.PC, cpu.ADL) & 0xFFFFFFu;
        case GDB_REG_IX: return cpu.registers.IX & 0xFFFFFFu;
        case GDB_REG_IY: return cpu.registers.IY & 0xFFFFFFu;
        case GDB_REG_AF_ALT: return cpu.registers._AF & 0xFFFFu;
        case GDB_REG_BC_ALT: return cpu.registers._BC & 0xFFFFFFu;
        case GDB_REG_DE_ALT: return cpu.registers._DE & 0xFFFFFFu;
        case GDB_REG_HL_ALT: return cpu.registers._HL & 0xFFFFFFu;
        case GDB_REG_IR:
            return ((uint32_t)cpu.registers.I << 8) | (cpu.registers.R & 0xFFu);
        case GDB_REG_SPS:
            return cpu.registers.SPS & 0xFFFFu;
        default: return 0;
    }
}

static void gdb_write_reg(unsigned reg, uint32_t value) {
    switch (reg) {
        case GDB_REG_AF:
            cpu.registers.AF = (uint16_t)value;
            cpu.registers.MBASE = (uint8_t)((value >> 16) & 0xFFu);
            break;
        case GDB_REG_BC:
            cpu.registers.BC = value & 0xFFFFFFu;
            break;
        case GDB_REG_DE:
            cpu.registers.DE = value & 0xFFFFFFu;
            break;
        case GDB_REG_HL:
            cpu.registers.HL = value & 0xFFFFFFu;
            break;
        case GDB_REG_SP:
            cpu.registers.SPL = value & 0xFFFFFFu;
            cpu.registers.SPS = (uint16_t)(value & 0xFFFFu);
            break;
        case GDB_REG_PC:
            debug_set_pc(value & 0xFFFFFFu);
            break;
        case GDB_REG_IX:
            cpu.registers.IX = value & 0xFFFFFFu;
            break;
        case GDB_REG_IY:
            cpu.registers.IY = value & 0xFFFFFFu;
            break;
        case GDB_REG_AF_ALT:
            cpu.registers._AF = (uint16_t)value;
            break;
        case GDB_REG_BC_ALT:
            cpu.registers._BC = value & 0xFFFFFFu;
            break;
        case GDB_REG_DE_ALT:
            cpu.registers._DE = value & 0xFFFFFFu;
            break;
        case GDB_REG_HL_ALT:
            cpu.registers._HL = value & 0xFFFFFFu;
            break;
        case GDB_REG_IR:
            cpu.registers.R = (uint8_t)(value & 0xFFu);
            cpu.registers.I = (uint16_t)((value >> 8) & 0xFFFFu);
            break;
        case GDB_REG_SPS:
            cpu.registers.SPS = (uint16_t)(value & 0xFFFFu);
            break;
        default:
            break;
    }
}

static const char gdb_target_xml[] =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
    "<target>"
    "<architecture>ez80-adl</architecture>"
    "<feature name=\"org.gnu.gdb.z80.cpu\">"
    "<reg name=\"af\" bitsize=\"24\" regnum=\"0\"/>"
    "<reg name=\"bc\" bitsize=\"24\"/>"
    "<reg name=\"de\" bitsize=\"24\"/>"
    "<reg name=\"hl\" bitsize=\"24\"/>"
    "<reg name=\"sp\" bitsize=\"24\" type=\"data_ptr\"/>"
    "<reg name=\"pc\" bitsize=\"24\" type=\"code_ptr\"/>"
    "<reg name=\"ix\" bitsize=\"24\"/>"
    "<reg name=\"iy\" bitsize=\"24\"/>"
    "<reg name=\"af'\" bitsize=\"24\"/>"
    "<reg name=\"bc'\" bitsize=\"24\"/>"
    "<reg name=\"de'\" bitsize=\"24\"/>"
    "<reg name=\"hl'\" bitsize=\"24\"/>"
    "<reg name=\"ir\" bitsize=\"24\"/>"
    "<reg name=\"sps\" bitsize=\"24\"/>"
    "</feature>"
    "</target>";

static bool send_stop_reply(int signal, const char *reason, uint32_t addr) {
    char *ptr = remcomOutBuffer;
    ptr += sprintf(ptr, "T%02x", signal);
    if (reason) {
        ptr += sprintf(ptr, "%s:%x;", reason, addr & 0xFFFFFFu);
    }
    *ptr = '\0';
    return putpacket(remcomOutBuffer);
}

static void gdbstub_loop(void) {
    gdb_in_debugger = true;

    while (1) {
        char *ptr = getpacket();
        if (!ptr) {
            gdbstub_disconnect();
            gdb_in_debugger = false;
            return;
        }

        // async break character
        if (ptr[0] == '\0') {
            send_stop_reply(5, NULL, 0);
            continue;
        }

        remcomOutBuffer[0] = '\0';
        bool reply = true;

        switch (*ptr++) {
            case '?':
                if (!send_stop_reply(5, NULL, 0)) {
                    gdbstub_disconnect();
                    gdb_in_debugger = false;
                    return;
                }
                reply = false;
                break;
            case 'g': {
                char *out = remcomOutBuffer;
                for (unsigned i = 0; i < GDB_EZ80_REG_COUNT; i++) {
                    uint32_t value = gdb_get_reg(i);
                    out = append_hex_le(out, value, gdb_ez80_reg_bytes[i]);
                }
                break;
            }
            case 'G': {
                const char *in = ptr;
                for (unsigned i = 0; i < GDB_EZ80_REG_COUNT; i++) {
                    uint32_t value;
                    if (!parse_hex_le(in, gdb_ez80_reg_bytes[i], &value)) {
                        strcpy(remcomOutBuffer, "E01");
                        goto reply_packet;
                    }
                    gdb_write_reg(i, value);
                    in += gdb_ez80_reg_bytes[i] * 2u;
                }
                strcpy(remcomOutBuffer, "OK");
                break;
            }
            case 'p': {
                uint32_t regno;
                if (hex_to_uint((const char **)&ptr, &regno) && regno < GDB_EZ80_REG_COUNT) {
                    append_hex_le(remcomOutBuffer, gdb_get_reg(regno), gdb_ez80_reg_bytes[regno]);
                } else {
                    strcpy(remcomOutBuffer, "E01");
                }
                break;
            }
            case 'P': {
                uint32_t regno;
                char *eq = strchr(ptr, '=');
                if (!eq) {
                    strcpy(remcomOutBuffer, "E01");
                    break;
                }
                *eq = '\0';
                if (hex_to_uint((const char **)&ptr, &regno) && regno < GDB_EZ80_REG_COUNT) {
                    uint32_t value;
                    if (parse_hex_le(eq + 1, gdb_ez80_reg_bytes[regno], &value)) {
                        gdb_write_reg(regno, value);
                        strcpy(remcomOutBuffer, "OK");
                    } else {
                        strcpy(remcomOutBuffer, "E01");
                    }
                } else {
                    strcpy(remcomOutBuffer, "E01");
                }
                break;
            }
            case 'm': {
                uint32_t addr, length;
                if (hex_to_uint((const char **)&ptr, &addr) && *ptr++ == ',' &&
                    hex_to_uint((const char **)&ptr, &length)) {
                    if (length > (GDB_BUF_MAX - 1) / 2) {
                        strcpy(remcomOutBuffer, "E01");
                        break;
                    }
                    mem2hex_addr(addr, remcomOutBuffer, length);
                } else {
                    strcpy(remcomOutBuffer, "E01");
                }
                break;
            }
            case 'M': {
                uint32_t addr, length;
                if (hex_to_uint((const char **)&ptr, &addr) && *ptr++ == ',' &&
                    hex_to_uint((const char **)&ptr, &length) && *ptr++ == ':') {
                    if (hex2mem_addr(ptr, addr, length)) {
                        strcpy(remcomOutBuffer, "OK");
                    } else {
                        strcpy(remcomOutBuffer, "E02");
                    }
                } else {
                    strcpy(remcomOutBuffer, "E01");
                }
                break;
            }
            case 's':
            case 'S': {
                char *semi = strchr(ptr, ';');
                if (semi) {
                    ptr = semi + 1;
                }
                if (*ptr) {
                    uint32_t addr;
                    if (hex_to_uint((const char **)&ptr, &addr)) {
                        debug_set_pc(addr & 0xFFFFFFu);
                    }
                }
                debug_step(DBG_STEP_IN, 0);
                gdb_in_debugger = false;
                return;
            }
            case 'c':
            case 'C': {
                char *semi = strchr(ptr, ';');
                if (semi) {
                    ptr = semi + 1;
                }
                if (*ptr) {
                    uint32_t addr;
                    if (hex_to_uint((const char **)&ptr, &addr)) {
                        debug_set_pc(addr & 0xFFFFFFu);
                    }
                }
                gdb_in_debugger = false;
                return;
            }
            case 'H':
                strcpy(remcomOutBuffer, "OK");
                break;
            case 'q': {
                if (!strncmp(ptr, "Supported", 9)) {
                    snprintf(remcomOutBuffer, sizeof remcomOutBuffer,
                             "PacketSize=%x;qXfer:features:read+;QStartNoAckMode+;vContSupported+",
                             GDB_BUF_MAX - 1);
                } else if (!strncmp(ptr, "Xfer:features:read:target.xml:", 30)) {
                    const char *args = ptr + 30;
                    uint32_t offset, length;
                    if (hex_to_uint(&args, &offset) && *args == ',' && (args++, hex_to_uint(&args, &length))) {
                        size_t xml_len = strlen(gdb_target_xml);
                        if (offset >= xml_len) {
                            remcomOutBuffer[0] = 'l';
                            remcomOutBuffer[1] = '\0';
                        } else {
                            size_t remaining = xml_len - offset;
                            size_t chunk = remaining < length ? remaining : length;
                            remcomOutBuffer[0] = (chunk < remaining) ? 'm' : 'l';
                            memcpy(remcomOutBuffer + 1, gdb_target_xml + offset, chunk);
                            remcomOutBuffer[1 + chunk] = '\0';
                        }
                    } else {
                        strcpy(remcomOutBuffer, "E01");
                    }
                } else if (!strcmp(ptr, "C")) {
                    strcpy(remcomOutBuffer, "QC1");
                } else if (!strcmp(ptr, "fThreadInfo")) {
                    strcpy(remcomOutBuffer, "m1");
                } else if (!strcmp(ptr, "sThreadInfo")) {
                    strcpy(remcomOutBuffer, "l");
                } else if (!strcmp(ptr, "Attached")) {
                    strcpy(remcomOutBuffer, "1");
                } else if (!strcmp(ptr, "Offsets")) {
                    strcpy(remcomOutBuffer, "Text=0;Data=0;Bss=0");
                } else if (!strcmp(ptr, "Symbol::")) {
                    strcpy(remcomOutBuffer, "OK");
                } else {
                    remcomOutBuffer[0] = '\0';
                }
                break;
            }
            case 'Q':
                if (!strcmp(ptr, "StartNoAckMode")) {
                    gdb_no_ack = true;
                    strcpy(remcomOutBuffer, "OK");
                } else {
                    remcomOutBuffer[0] = '\0';
                }
                break;
            case 'v':
                if (!strcmp(ptr, "Cont?")) {
                    strcpy(remcomOutBuffer, "vCont;c;s");
                } else {
                    remcomOutBuffer[0] = '\0';
                }
                break;
            case 'Z':
            case 'z': {
                bool set = (*(ptr - 1) == 'Z');
                char type = *ptr++;
                if (*ptr++ != ',') {
                    strcpy(remcomOutBuffer, "E01");
                    break;
                }
                uint32_t addr, length = 1;
                if (!hex_to_uint((const char **)&ptr, &addr)) {
                    strcpy(remcomOutBuffer, "E01");
                    break;
                }
                if (*ptr == ',') {
                    ptr++;
                    hex_to_uint((const char **)&ptr, &length);
                    if (length == 0) {
                        length = 1;
                    }
                }
                addr &= 0xFFFFFFu;
                if (addr >= DBG_ADDR_SIZE) {
                    strcpy(remcomOutBuffer, "E01");
                    break;
                }
                int mask = DBG_MASK_NONE;
                switch (type) {
                    case '0':
                    case '1':
                        mask = DBG_MASK_EXEC;
                        break;
                    case '2':
                        mask = DBG_MASK_WRITE;
                        break;
                    case '3':
                        mask = DBG_MASK_READ;
                        break;
                    case '4':
                        mask = DBG_MASK_RW;
                        break;
                    default:
                        strcpy(remcomOutBuffer, "E01");
                        goto reply_packet;
                }
                for (uint32_t i = 0; i < length; i++) {
                    if ((addr + i) >= DBG_ADDR_SIZE) {
                        break;
                    }
                    debug_watch(addr + i, mask, set);
                }
                strcpy(remcomOutBuffer, "OK");
                break;
            }
            case 'D':
                strcpy(remcomOutBuffer, "OK");
                putpacket(remcomOutBuffer);
                gdbstub_disconnect();
                gdb_in_debugger = false;
                return;
            case 'k':
                gdbstub_disconnect();
                gdb_in_debugger = false;
                return;
            default:
                remcomOutBuffer[0] = '\0';
                break;
        }

    reply_packet:
        if (reply && !putpacket(remcomOutBuffer)) {
            gdbstub_disconnect();
            gdb_in_debugger = false;
            return;
        }
    }
}

bool gdbstub_init(unsigned int port) {
    struct sockaddr_in sockaddr;
    int r;

#ifdef __MINGW32__
    WORD wVersionRequested = MAKEWORD(2, 0);
    WSADATA wsaData;
    if (WSAStartup(wVersionRequested, &wsaData)) {
        log_socket_error("WSAStartup failed");
        return false;
    }
#endif

    listen_socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (listen_socket_fd == -1) {
        log_socket_error("Failed to create GDB socket");
        return false;
    }
    set_nonblocking(listen_socket_fd, true);

    memset(&sockaddr, 0, sizeof sockaddr);
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons((uint16_t)port);
    sockaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    r = bind(listen_socket_fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (r == -1) {
        log_socket_error("Failed to bind GDB socket");
        gdbstub_shutdown();
        return false;
    }
    r = listen(listen_socket_fd, 0);
    if (r == -1) {
        log_socket_error("Failed to listen on GDB socket");
        gdbstub_shutdown();
        return false;
    }

    gui_console_printf("[CEmu] GDB stub listening on 127.0.0.1:%u\n", port);
    return true;
}

bool gdbstub_init_from_env(void) {
    const char *env = getenv("CEMU_GDB_PORT");
    if (!env || !*env) {
        return false;
    }
    char *end = NULL;
    long port = strtol(env, &end, 10);
    if (end == env || port <= 0 || port > 65535) {
        gui_console_err_printf("[CEmu][GDB] Invalid CEMU_GDB_PORT: %s\n", env);
        return false;
    }
    return gdbstub_init((unsigned int)port);
}

void gdbstub_shutdown(void) {
    if (listen_socket_fd != -1) {
#ifdef __MINGW32__
        closesocket(listen_socket_fd);
#else
        close(listen_socket_fd);
#endif
        listen_socket_fd = -1;
    }
    if (socket_fd != -1) {
#ifdef __MINGW32__
        closesocket(socket_fd);
#else
        close(socket_fd);
#endif
        socket_fd = -1;
    }
    gdb_connected = false;
    gdb_handshake_complete = false;
    gdb_no_ack = false;
    gdb_in_debugger = false;
}

bool gdbstub_is_connected(void) {
    return gdb_connected;
}

void gdbstub_poll(void) {
    if (listen_socket_fd == -1) {
        return;
    }

    if (socket_fd == -1) {
        socket_fd = accept(listen_socket_fd, NULL, NULL);
        if (socket_fd == -1) {
            return;
        }
        set_nonblocking(socket_fd, true);
        int on = 1;
#ifdef __MINGW32__
        setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(on));
#else
        setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
#endif
        gdb_connected = true;
        gdb_handshake_complete = false;
        gdb_no_ack = false;
        gui_console_printf("[CEmu] GDB connected.\n");
    }

    if (!gdb_connected || gdb_in_debugger || debug_is_open()) {
        return;
    }

    int ready = socket_can_read(socket_fd, 0);
    if (ready == -1) {
        gdbstub_disconnect();
        return;
    }
    if (!ready) {
        return;
    }

    char peek = 0;
    int peeked = recv(socket_fd, &peek, 1, MSG_PEEK);
    if (peeked == 1 && peek == '\x03') {
        recv(socket_fd, &peek, 1, 0);
    }

    if (!gdb_handshake_complete) {
        gdb_handshake_complete = true;
    }

    debug_open(DBG_USER, cpu.registers.PC);
}

bool gdbstub_on_debug(int reason, uint32_t addr) {
    if (!gdb_connected || socket_fd == -1) {
        return false;
    }

    switch (reason) {
        case DBG_WATCHPOINT_WRITE:
            send_stop_reply(5, "watch", addr);
            break;
        case DBG_WATCHPOINT_READ:
            send_stop_reply(5, "rwatch", addr);
            break;
        default:
            send_stop_reply(5, NULL, 0);
            break;
    }

    gdbstub_loop();
    return true;
}

static void gdbstub_disconnect(void) {
    gui_console_printf("[CEmu] GDB disconnected.\n");
#ifdef __MINGW32__
    closesocket(socket_fd);
#else
    close(socket_fd);
#endif
    socket_fd = -1;
    gdb_connected = false;
    gdb_handshake_complete = false;
    gdb_no_ack = false;
}

#else

bool gdbstub_init(unsigned int port) {
    (void)port;
    return false;
}

bool gdbstub_init_from_env(void) {
    return false;
}

void gdbstub_shutdown(void) {}

void gdbstub_poll(void) {}

bool gdbstub_on_debug(int reason, uint32_t addr) {
    (void)reason;
    (void)addr;
    return false;
}

bool gdbstub_is_connected(void) {
    return false;
}

#endif
