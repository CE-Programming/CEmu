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

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET gdb_socket_t;
typedef int gdb_io_count_t;
#define GDB_INVALID_SOCKET INVALID_SOCKET
static bool winsock_started;
#else
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
typedef int gdb_socket_t;
typedef ssize_t gdb_io_count_t;
#define GDB_INVALID_SOCKET (-1)
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#define GDB_BUF_MAX 4096
#define GDB_BREAKPOINT_MAX 256

static gdb_socket_t listen_socket_fd = GDB_INVALID_SOCKET;
static gdb_socket_t socket_fd = GDB_INVALID_SOCKET;
static bool gdb_in_debugger, gdb_no_ack, gdb_request_pending, gdb_interrupt;
static bool gdb_requested_step;
static int stop_signal = 5;
static const char *stop_reason;
static uint32_t stop_address;

static char remcomInBuffer[GDB_BUF_MAX];
static char remcomOutBuffer[GDB_BUF_MAX];
static const char hexchars[] = "0123456789abcdef";

typedef struct {
    uint32_t address, length;
    unsigned type;
    int mask;
} gdb_breakpoint_t;
static gdb_breakpoint_t breakpoints[GDB_BREAKPOINT_MAX];
static size_t breakpoint_count;
static uint32_t pending_watch_address;
static bool pending_watch_write, pending_watch_gdb;

static void gdbstub_disconnect(void);

static void log_socket_error(const char *msg) {
#ifdef _WIN32
    gui_console_err_printf("[CEmu][GDB] %s: Winsock error %d\n", msg, WSAGetLastError());
#else
    gui_console_err_printf("[CEmu][GDB] %s: %s\n", msg, strerror(errno));
#endif
}

static bool socket_interrupted(void) {
#ifdef _WIN32
    return WSAGetLastError() == WSAEINTR;
#else
    return errno == EINTR;
#endif
}

static bool socket_would_block(void) {
#ifdef _WIN32
    return WSAGetLastError() == WSAEWOULDBLOCK;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}

static void close_socket(gdb_socket_t *fd) {
    if (*fd != GDB_INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(*fd);
#else
        close(*fd);
#endif
        *fd = GDB_INVALID_SOCKET;
    }
}

static bool set_nonblocking(gdb_socket_t fd) {
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(fd, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return false;
    }
    flags = fcntl(fd, F_GETFD, 0);
    return flags >= 0 && fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == 0;
#endif
}

static int socket_ready(gdb_socket_t fd, bool write, int timeout_ms) {
#ifdef _WIN32
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    struct timeval timeout = { timeout_ms / 1000, (timeout_ms % 1000) * 1000 };
    return select(0, write ? NULL : &fds, write ? &fds : NULL, NULL, &timeout);
#else
    struct pollfd pfd = { fd, write ? POLLOUT : POLLIN, 0 };
    return poll(&pfd, 1, timeout_ms);
#endif
}

/* Both reads and writes must allow the emulator thread to exit while stopped. */
static bool wait_socket(bool write) {
    while (socket_fd != GDB_INVALID_SOCKET && !(cpu_check_signals() & CPU_SIGNAL_EXIT)) {
        int ready = socket_ready(socket_fd, write, 100);
        if (ready > 0) {
            return true;
        }
        if (ready < 0 && !socket_interrupted()) {
            log_socket_error("Failed to poll GDB socket");
            return false;
        }
    }
    return false;
}

static bool send_bytes(const char *buffer, size_t length) {
    while (length) {
        if (!wait_socket(true)) {
            return false;
        }
#ifdef _WIN32
        gdb_io_count_t count = send(socket_fd, buffer, (int)length, MSG_NOSIGNAL);
#else
        gdb_io_count_t count = send(socket_fd, buffer, length, MSG_NOSIGNAL);
#endif
        if (count < 0 && (socket_interrupted() || socket_would_block())) {
            continue;
        }
        if (count <= 0) {
            return false;
        }
        buffer += count;
        length -= (size_t)count;
    }
    return true;
}

/* Return a byte as an int, so binary 0xff cannot be confused with EOF. */
static int get_debug_char(void) {
    while (wait_socket(false)) {
        unsigned char c;
        gdb_io_count_t count = recv(socket_fd, (char *)&c, 1, 0);
        if (count < 0 && (socket_interrupted() || socket_would_block())) {
            continue;
        }
        return count == 1 ? c : -1;
    }
    return -1;
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
        if (value > (UINT32_MAX - (unsigned)digit) / 16) {
            return false;
        }
        value = value * 16 + (unsigned)digit;
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
    for (;;) {
        int ch;
        gdb_interrupt = false;
        do {
            ch = get_debug_char();
            if (ch < 0) {
                return NULL;
            }
            if (ch == 3) {
                gdb_interrupt = true;
                return remcomInBuffer;
            }
        } while (ch != '$');

        unsigned checksum = 0;
        size_t count = 0;
        bool invalid = false, escaped = false;
        for (;;) {
            ch = get_debug_char();
            if (ch < 0) {
                return NULL;
            }
            if (!escaped && ch == '$') {
                checksum = 0;
                count = 0;
                invalid = false;
                continue;
            }
            if (!escaped && ch == '#') {
                break;
            }
            checksum = (checksum + (unsigned)ch) & 0xff;
            if (!escaped && ch == '}') {
                escaped = true;
                continue;
            }
            if (escaped) {
                ch ^= 0x20;
                escaped = false;
            }
            if (ch == 0 || count == sizeof remcomInBuffer - 1) {
                invalid = true;
            } else {
                remcomInBuffer[count++] = (char)ch;
            }
        }
        int hi_char = get_debug_char(), lo_char = get_debug_char();
        if (hi_char < 0 || lo_char < 0) {
            return NULL;
        }
        int hi = hex_to_int((char)hi_char), lo = hex_to_int((char)lo_char);
        bool valid = !invalid && hi >= 0 && lo >= 0 && checksum == (unsigned)(hi * 16 + lo);
        if (!gdb_no_ack && !send_bytes(valid ? "+" : "-", 1)) {
            return NULL;
        }
        if (valid) {
            remcomInBuffer[count] = '\0';
            return remcomInBuffer;
        }
    }
}

static bool putpacket(const char *buffer) {
    char packet[GDB_BUF_MAX + 4];
    size_t length = strlen(buffer);
    unsigned checksum = 0;
    packet[0] = '$';
    for (size_t i = 0; i < length; i++) {
        packet[1 + i] = buffer[i];
        checksum = (checksum + (unsigned char)buffer[i]) & 0xff;
    }
    packet[1 + length] = '#';
    packet[2 + length] = hexchars[checksum >> 4];
    packet[3 + length] = hexchars[checksum & 0xf];
    for (;;) {
        if (!send_bytes(packet, length + 4)) {
            return false;
        }
        if (gdb_no_ack) {
            return true;
        }
        int ack;
        do {
            ack = get_debug_char();
            if (ack < 0) {
                return false;
            }
        } while (ack != '+' && ack != '-');
        if (ack == '+') {
            return true;
        }
    }
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
        case GDB_REG_AF: return cpu.registers.AF;
        case GDB_REG_BC: return cpu.registers.BC & 0xFFFFFFu;
        case GDB_REG_DE: return cpu.registers.DE & 0xFFFFFFu;
        case GDB_REG_HL: return cpu.registers.HL & 0xFFFFFFu;
        case GDB_REG_SP:
            return cpu.registers.SPL & 0xFFFFFFu;
        case GDB_REG_PC:
            return cpu_address_mode(cpu.registers.PC, cpu.ADL) & 0xFFFFFFu;
        case GDB_REG_IX: return cpu.registers.IX & 0xFFFFFFu;
        case GDB_REG_IY: return cpu.registers.IY & 0xFFFFFFu;
        case GDB_REG_AF_ALT: return cpu.registers._AF & 0xFFFFu;
        case GDB_REG_BC_ALT: return cpu.registers._BC & 0xFFFFFFu;
        case GDB_REG_DE_ALT: return cpu.registers._DE & 0xFFFFFFu;
        case GDB_REG_HL_ALT: return cpu.registers._HL & 0xFFFFFFu;
        case GDB_REG_IR:
            return ((uint32_t)cpu.registers.I << 8) | cpu.registers.R;
        case GDB_REG_SPS:
            return cpu.registers.SPS & 0xFFFFu;
        default: return 0;
    }
}

static void gdb_write_reg(unsigned reg, uint32_t value) {
    switch (reg) {
        case GDB_REG_AF:
            cpu.registers.AF = (uint16_t)value;
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

static bool send_stop_reply(void) {
    if (stop_reason) {
        snprintf(remcomOutBuffer, sizeof remcomOutBuffer, "T%02xthread:1;%s:%x;",
                 stop_signal, stop_reason, stop_address);
    } else {
        snprintf(remcomOutBuffer, sizeof remcomOutBuffer, "T%02xthread:1;", stop_signal);
    }
    return putpacket(remcomOutBuffer);
}

static uint32_t breakpoint_span(const gdb_breakpoint_t *bp) {
    /* The kind of an execution breakpoint describes an instruction, not a range. */
    return bp->type <= 1 ? 1 : bp->length;
}

static bool breakpoint_contains(const gdb_breakpoint_t *bp, uint32_t addr) {
    return addr >= bp->address && addr - bp->address < breakpoint_span(bp);
}

int gdbstub_watch_mask(uint32_t addr) {
    int mask = 0;
    for (size_t i = 0; i < breakpoint_count; i++) {
        if (breakpoint_contains(&breakpoints[i], addr)) {
            mask |= breakpoints[i].mask;
        }
    }
    return mask;
}

void gdbstub_watch_access(uint32_t addr, bool write) {
    if (gdb_in_debugger) {
        return;
    }
    int mask = write ? DBG_MASK_WRITE : DBG_MASK_READ;
    bool gdb_watch = (debug.addr[addr] & DBG_MASK_GDB) && (gdbstub_watch_mask(addr) & mask);
    bool gui_watch = (debug.addr[addr] & mask) && gdbstub_is_connected();
    /* All data stops sent to GDB need a completed instruction. Prefer the
       GDB-owned range if an earlier byte first triggered a wider GUI watch. */
    if ((gdb_watch || gui_watch) && (!debug.gdbWatch || (gdb_watch && !pending_watch_gdb))) {
        pending_watch_address = addr;
        pending_watch_write = write;
        pending_watch_gdb = gdb_watch;
        debug.gdbWatch = true;
    }
}

void gdbstub_handle_watch(void) {
    debug.gdbWatch = false;
    debug_open(pending_watch_write ? DBG_WATCHPOINT_WRITE : DBG_WATCHPOINT_READ,
               pending_watch_address);
}

static void mark_breakpoint(const gdb_breakpoint_t *bp, bool set) {
    for (uint32_t i = 0; i < breakpoint_span(bp); i++) {
        debug_watch(bp->address + i, DBG_MASK_GDB, set);
    }
}

static bool change_breakpoint(unsigned type, uint32_t addr, uint32_t length, bool set) {
    size_t index;
    for (index = 0; index < breakpoint_count; index++) {
        const gdb_breakpoint_t *bp = &breakpoints[index];
        if (bp->type == type && bp->address == addr && bp->length == length) {
            break;
        }
    }
    if (set) {
        if (index < breakpoint_count) {
            return true; /* RSP insertion is idempotent. */
        }
        if (breakpoint_count == GDB_BREAKPOINT_MAX) {
            return false;
        }
        static const int masks[] = {
            DBG_MASK_EXEC, DBG_MASK_EXEC, DBG_MASK_WRITE, DBG_MASK_READ, DBG_MASK_RW
        };
        gdb_breakpoint_t bp = { addr, length, type, masks[type] };
        breakpoints[breakpoint_count++] = bp;
        mark_breakpoint(&bp, true);
    } else if (index < breakpoint_count) {
        gdb_breakpoint_t removed = breakpoints[index];
        breakpoints[index] = breakpoints[--breakpoint_count];
        mark_breakpoint(&removed, false);
        /* Restore only the intersection with remaining GDB breakpoints. */
        uint32_t end = removed.address + breakpoint_span(&removed);
        for (size_t i = 0; i < breakpoint_count; i++) {
            const gdb_breakpoint_t *bp = &breakpoints[i];
            uint32_t first = bp->address > removed.address ? bp->address : removed.address;
            uint32_t last = bp->address + breakpoint_span(bp);
            if (last > end) {
                last = end;
            }
            for (uint32_t addr = first; addr < last; addr++) {
                debug_watch(addr, DBG_MASK_GDB, true);
            }
        }
    }
    return true;
}

static bool valid_range(uint32_t addr, uint32_t length) {
    return addr < DBG_ADDR_SIZE && length <= DBG_ADDR_SIZE - addr;
}

static bool valid_hex(const char *data, size_t bytes) {
    if (strlen(data) != bytes * 2) {
        return false;
    }
    for (size_t i = 0; i < bytes * 2; i++) {
        if (hex_to_int(data[i]) < 0) {
            return false;
        }
    }
    return true;
}

static bool parse_range(const char **ptr, uint32_t *addr, uint32_t *length) {
    if (!hex_to_uint(ptr, addr) || **ptr != ',') {
        return false;
    }
    ++*ptr;
    return hex_to_uint(ptr, length) && valid_range(*addr, *length);
}

static bool valid_thread(const char *thread) {
    return !strcmp(thread, "-1") || !strcmp(thread, "0") || !strcmp(thread, "1");
}

/* Signals have no delivery mechanism in the emulated CPU; accept and ignore them. */
static bool parse_resume(char action, const char *args) {
    if (action == 'C' || action == 'S') {
        uint32_t signal;
        if (!hex_to_uint(&args, &signal) || signal > 0xff) {
            return false;
        }
        if (*args == '\0') {
            return true;
        }
        if (*args++ != ';') {
            return false;
        }
    }
    if (*args) {
        uint32_t addr;
        if (!hex_to_uint(&args, &addr) || *args || !valid_range(addr, 1)) {
            return false;
        }
        debug_set_pc(addr);
    }
    return true;
}

static bool parse_vcont(char *args, bool *step) {
    char selected = 0, fallback = 0;
    while (*args) {
        if (*args++ != ';' || !*args) {
            return false;
        }
        char action = *args++;
        if (action != 'c' && action != 's' && action != 'C' && action != 'S') {
            return false;
        }
        if (action == 'C' || action == 'S') {
            const char *signal_args = args;
            uint32_t signal;
            if (!hex_to_uint(&signal_args, &signal) || signal > 0xff) {
                return false;
            }
            args = (char *)signal_args;
        }
        bool targeted = *args == ':';
        if (targeted) {
            char *thread = ++args;
            while (*args && *args != ';') {
                args++;
            }
            char delimiter = *args;
            *args = '\0';
            bool valid = valid_thread(thread);
            *args = delimiter;
            if (!valid || selected) {
                return false;
            }
            selected = action;
        } else {
            if ((*args && *args != ';') || fallback) {
                return false;
            }
            fallback = action;
        }
    }
    char action = selected ? selected : fallback;
    *step = action == 's' || action == 'S';
    return action != 0;
}

static void gdbstub_loop(void) {
    gdb_in_debugger = true;
    for (;;) {
        const char *ptr = getpacket();
        if (!ptr) {
            break;
        }
        if (gdb_interrupt) {
            stop_signal = 2;
            stop_reason = NULL;
            if (!send_stop_reply()) {
                break;
            }
            continue;
        }
        remcomOutBuffer[0] = '\0';
        bool step = false;
        char command = *ptr;
        if (*ptr) {
            ptr++;
        }
        switch (command) {
            case '?':
                if (!send_stop_reply()) {
                    goto disconnected;
                }
                continue;
            case 'g': {
                if (*ptr) {
                    goto invalid;
                }
                char *out = remcomOutBuffer;
                for (unsigned i = 0; i < GDB_EZ80_REG_COUNT; i++) {
                    out = append_hex_le(out, gdb_get_reg(i), gdb_ez80_reg_bytes[i]);
                }
                break;
            }
            case 'G': {
                uint32_t values[GDB_EZ80_REG_COUNT];
                if (!valid_hex(ptr, GDB_EZ80_REG_COUNT * 3)) {
                    goto invalid;
                }
                for (unsigned i = 0; i < GDB_EZ80_REG_COUNT; i++) {
                    parse_hex_le(ptr, gdb_ez80_reg_bytes[i], &values[i]);
                    ptr += gdb_ez80_reg_bytes[i] * 2;
                }
                /* Validate the entire packet before changing any register. */
                for (unsigned i = 0; i < GDB_EZ80_REG_COUNT; i++) {
                    gdb_write_reg(i, values[i]);
                }
                strcpy(remcomOutBuffer, "OK");
                break;
            }
            case 'p': {
                uint32_t reg;
                if (!hex_to_uint(&ptr, &reg) || *ptr || reg >= GDB_EZ80_REG_COUNT) {
                    goto invalid;
                }
                append_hex_le(remcomOutBuffer, gdb_get_reg(reg), gdb_ez80_reg_bytes[reg]);
                break;
            }
            case 'P': {
                uint32_t reg, value;
                if (!hex_to_uint(&ptr, &reg) || *ptr != '=' || reg >= GDB_EZ80_REG_COUNT) {
                    goto invalid;
                }
                ptr++;
                if (!valid_hex(ptr, gdb_ez80_reg_bytes[reg]) ||
                    !parse_hex_le(ptr, gdb_ez80_reg_bytes[reg], &value)) {
                    goto invalid;
                }
                gdb_write_reg(reg, value);
                strcpy(remcomOutBuffer, "OK");
                break;
            }
            case 'm': {
                uint32_t addr, length;
                if (!parse_range(&ptr, &addr, &length) || *ptr || length > (GDB_BUF_MAX - 1) / 2) {
                    goto invalid;
                }
                mem2hex_addr(addr, remcomOutBuffer, length);
                break;
            }
            case 'M': {
                uint32_t addr, length;
                if (!parse_range(&ptr, &addr, &length) || *ptr != ':' ||
                    length > (GDB_BUF_MAX - 1) / 2 || !valid_hex(ptr + 1, length)) {
                    goto invalid;
                }
                hex2mem_addr(ptr + 1, addr, length);
                if (cpu.registers.PC >= addr && cpu.registers.PC - addr < length) {
                    cpu.prefetch = mem_peek_byte(cpu.registers.PC);
                }
                strcpy(remcomOutBuffer, "OK");
                break;
            }
            case 's': case 'S': case 'c': case 'C':
                if (!parse_resume(command, ptr)) {
                    goto invalid;
                }
                step = command == 's' || command == 'S';
                goto resume;
            case 'H':
                if ((*ptr != 'c' && *ptr != 'g') || !valid_thread(ptr + 1)) {
                    goto invalid;
                }
                strcpy(remcomOutBuffer, "OK");
                break;
            case 'T':
                if (strcmp(ptr, "1")) {
                    goto invalid;
                }
                strcpy(remcomOutBuffer, "OK");
                break;
            case 'q': {
                static const char xml_prefix[] = "Xfer:features:read:target.xml:";
                if (!strcmp(ptr, "Supported") || !strncmp(ptr, "Supported:", 10)) {
                    snprintf(remcomOutBuffer, sizeof remcomOutBuffer,
                             "PacketSize=%x;qXfer:features:read+;QStartNoAckMode+;vContSupported+",
                             GDB_BUF_MAX - 1);
                } else if (!strncmp(ptr, xml_prefix, sizeof xml_prefix - 1)) {
                    ptr += sizeof xml_prefix - 1;
                    uint32_t offset, length;
                    if (!hex_to_uint(&ptr, &offset) || *ptr != ',') {
                        goto invalid;
                    }
                    ptr++;
                    if (!hex_to_uint(&ptr, &length) || *ptr || !length) {
                        goto invalid;
                    }
                    size_t xml_length = sizeof gdb_target_xml - 1;
                    size_t remaining = offset < xml_length ? xml_length - offset : 0;
                    size_t chunk = remaining < length ? remaining : length;
                    if (chunk > GDB_BUF_MAX - 2) {
                        chunk = GDB_BUF_MAX - 2;
                    }
                    remcomOutBuffer[0] = chunk < remaining ? 'm' : 'l';
                    if (chunk) {
                        memcpy(remcomOutBuffer + 1, gdb_target_xml + offset, chunk);
                    }
                    remcomOutBuffer[1 + chunk] = '\0';
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
                }
                break;
            }
            case 'Q':
                if (!strcmp(ptr, "StartNoAckMode")) {
                    /* GDB sends one final '+' after OK, which getpacket ignores. */
                    gdb_no_ack = true;
                    strcpy(remcomOutBuffer, "OK");
                }
                break;
            case 'v':
                if (!strcmp(ptr, "Cont?")) {
                    strcpy(remcomOutBuffer, "vCont;c;C;s;S");
                } else if (!strncmp(ptr, "Cont;", 5)) {
                    if (!parse_vcont((char *)ptr + 4, &step)) {
                        goto invalid;
                    }
                    goto resume;
                }
                break;
            case 'Z': case 'z': {
                uint32_t type, addr, length;
                if (!hex_to_uint(&ptr, &type) || *ptr != ',') {
                    goto invalid;
                }
                if (type > 4) {
                    break; /* Unsupported breakpoint type: empty RSP reply. */
                }
                ptr++;
                if (!hex_to_uint(&ptr, &addr) || *ptr != ',' || addr >= DBG_ADDR_SIZE) {
                    goto invalid;
                }
                ptr++;
                if (!hex_to_uint(&ptr, &length) || *ptr || !length ||
                    (type > 1 && !valid_range(addr, length))) {
                    goto invalid;
                }
                if (!change_breakpoint(type, addr, length, command == 'Z')) {
                    goto invalid;
                }
                strcpy(remcomOutBuffer, "OK");
                break;
            }
            case 'D':
                if (*ptr) {
                    goto invalid;
                }
                putpacket("OK");
                goto disconnected;
            case 'k':
                goto disconnected;
            default:
                break; /* Unsupported packets must receive an empty reply. */
        }
        goto reply;
    invalid:
        strcpy(remcomOutBuffer, "E01");
    reply:
        if (!putpacket(remcomOutBuffer)) {
            break;
        }
        continue;
    resume:
        gdb_requested_step = step;
        if (step) {
            debug_step(DBG_STEP_IN, 0);
        }
        gdb_in_debugger = false;
        return;
    }
 disconnected:
    gdbstub_disconnect();
}

bool gdbstub_init(unsigned int port) {
    if (!port || port > 65535) {
        return false;
    }
    gdbstub_shutdown();
#ifdef _WIN32
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data)) {
        gui_console_err_printf("[CEmu][GDB] WSAStartup failed.\n");
        return false;
    }
    winsock_started = true;
#endif
    listen_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket_fd == GDB_INVALID_SOCKET) {
        log_socket_error("Failed to create GDB socket");
        goto failed;
    }
    if (!set_nonblocking(listen_socket_fd)) {
        log_socket_error("Failed to configure GDB socket");
        goto failed;
    }
    int on = 1;
#ifdef _WIN32
    int reuse_option = SO_EXCLUSIVEADDRUSE;
#else
    int reuse_option = SO_REUSEADDR;
#endif
    if (setsockopt(listen_socket_fd, SOL_SOCKET, reuse_option, (const char *)&on, sizeof on) < 0) {
        log_socket_error("Failed to configure GDB listener reuse");
        goto failed;
    }
    struct sockaddr_in address;
    memset(&address, 0, sizeof address);
    address.sin_family = AF_INET;
    address.sin_port = htons((uint16_t)port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(listen_socket_fd, (struct sockaddr *)&address, sizeof address) < 0 ||
        listen(listen_socket_fd, 1) < 0) {
        log_socket_error("Failed to listen on GDB socket");
        goto failed;
    }
    gui_console_printf("[CEmu] GDB stub listening on 127.0.0.1:%u\n", port);
    return true;
 failed:
    gdbstub_shutdown();
    return false;
}

bool gdbstub_init_from_env(void) {
    const char *env = getenv("CEMU_GDB_PORT");
    if (!env || !*env) {
        return false;
    }
    char *end;
    errno = 0;
    unsigned long port = strtoul(env, &end, 10);
    if (errno || end == env || *end || !port || port > 65535) {
        gui_console_err_printf("[CEmu][GDB] Invalid CEMU_GDB_PORT: %s\n", env);
        return false;
    }
    return gdbstub_init((unsigned)port);
}

static void gdbstub_disconnect(void) {
    if (socket_fd != GDB_INVALID_SOCKET) {
        gui_console_printf("[CEmu] GDB disconnected.\n");
    }
    close_socket(&socket_fd);
    for (size_t i = 0; i < breakpoint_count; i++) {
        mark_breakpoint(&breakpoints[i], false);
    }
    breakpoint_count = 0;
    debug.gdbWatch = false;
    if (gdb_requested_step) {
        debug_clear_step();
        gdb_requested_step = false;
    }
    gdb_in_debugger = gdb_request_pending = gdb_no_ack = gdb_interrupt = false;
    stop_signal = 5;
    stop_reason = NULL;
}

void gdbstub_shutdown(void) {
    gdbstub_disconnect();
    close_socket(&listen_socket_fd);
#ifdef _WIN32
    if (winsock_started) {
        WSACleanup();
        winsock_started = false;
    }
#endif
}

bool gdbstub_is_connected(void) {
    return socket_fd != GDB_INVALID_SOCKET;
}

void gdbstub_poll(void) {
    if (listen_socket_fd == GDB_INVALID_SOCKET || gdb_in_debugger || debug_is_open()) {
        return;
    }
    if (socket_fd == GDB_INVALID_SOCKET) {
        socket_fd = accept(listen_socket_fd, NULL, NULL);
        if (socket_fd == GDB_INVALID_SOCKET) {
            return;
        }
        if (!set_nonblocking(socket_fd)) {
            log_socket_error("Failed to configure GDB connection");
            gdbstub_disconnect();
            return;
        }
        int on = 1;
        setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof on);
#ifdef SO_NOSIGPIPE
        /* SO_NOSIGPIPE is a socket option, never a send() flag. */
        if (setsockopt(socket_fd, SOL_SOCKET, SO_NOSIGPIPE, (const char *)&on, sizeof on) < 0) {
            log_socket_error("Failed to disable SIGPIPE on GDB connection");
            gdbstub_disconnect();
            return;
        }
#endif
        gui_console_printf("[CEmu] GDB connected.\n");
    }
    int ready = socket_ready(socket_fd, false, 0);
    if (ready < 0 && !socket_interrupted()) {
        gdbstub_disconnect();
        return;
    }
    if (ready <= 0) {
        return;
    }
    char peek;
    gdb_io_count_t count = recv(socket_fd, &peek, 1, MSG_PEEK);
    if (count < 0 && (socket_interrupted() || socket_would_block())) {
        return;
    }
    if (count <= 0) {
        gdbstub_disconnect();
        return;
    }
    if (peek != '$' && peek != 3) {
        /* Discard stray ACKs without opening the GUI or stopping the CPU. */
        recv(socket_fd, &peek, 1, 0);
        return;
    }
    gdb_request_pending = peek == '$';
    gdb_interrupt = peek == 3;
    if (gdb_interrupt) {
        recv(socket_fd, &peek, 1, 0);
    }
    debug_open(DBG_USER, cpu.registers.PC);
}

bool gdbstub_on_debug(int reason, uint32_t addr) {
    if (!gdbstub_is_connected()) {
        return false;
    }
    gdb_requested_step = false;
    stop_signal = gdb_interrupt ? 2 : 5;
    stop_reason = NULL;
    stop_address = addr;
    if (reason == DBG_WATCHPOINT_READ || reason == DBG_WATCHPOINT_WRITE) {
        int mask = reason == DBG_WATCHPOINT_WRITE ? DBG_MASK_WRITE : DBG_MASK_READ;
        stop_reason = reason == DBG_WATCHPOINT_WRITE ? "watch" : "rwatch";
        for (size_t i = 0; i < breakpoint_count; i++) {
            const gdb_breakpoint_t *bp = &breakpoints[i];
            if ((bp->mask & mask) && breakpoint_contains(bp, addr)) {
                stop_address = bp->address;
                if (bp->type == 4) {
                    stop_reason = "awatch";
                }
                break;
            }
        }
    }
    /* Attach/query packets get their own replies, not an unsolicited stop packet. */
    bool reply = !gdb_request_pending;
    gdb_request_pending = false;
    if (reply && !send_stop_reply()) {
        gdbstub_disconnect();
        return true;
    }
    gdbstub_loop();
    return true;
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

int gdbstub_watch_mask(uint32_t addr) {
    (void)addr;
    return 0;
}

void gdbstub_watch_access(uint32_t addr, bool write) {
    (void)addr;
    (void)write;
}

void gdbstub_handle_watch(void) {}

#endif
