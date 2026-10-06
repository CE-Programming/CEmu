/* Exercise GDB/GUI watchpoint overlap through the real core memory path. */
#include <stdio.h>
#include <stdlib.h>

#include "../../core/debug/gdbstub.c"
#include "../../core/schedule.h"

static unsigned gui_stops;
static uint32_t stopped_value;
static const uint32_t watched_address = 0xd11000;

void gui_console_clear(void) {}
void gui_console_printf(const char *format, ...) { (void)format; }
void gui_console_err_printf(const char *format, ...) { (void)format; }
void gui_debug_close(void) {}
void gui_debug_open(int reason, uint32_t data) {
    (void)reason;
    (void)data;
    gui_stops++;
    stopped_value = mem_peek_long(watched_address);
}
asic_rev_t gui_handle_reset(const boot_ver_t *boot, asic_rev_t loaded,
                           asic_rev_t fallback, emu_device_t device, bool *python) {
    (void)boot; (void)fallback; (void)device; (void)python;
    return loaded;
}

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); abort(); \
} } while (0)

int main(void) {
    /* No ROM or network is needed to test watch scheduling and memory accesses. */
    mem_init();
    cpu_init();
    sched_init();
    sched_reset();
    debug_init();
    cpu.L = cpu.ADL = true;
    cpu.registers.PC = 0xd10000;
    for (unsigned i = 0; i < 3; ++i) {
        debug_watch(watched_address + i, DBG_MASK_RW, true);
    }
    CHECK(change_breakpoint(4, watched_address, 3, true));
    mem_write_cpu(watched_address, 0x56);
    mem_write_cpu(watched_address + 1, 0x34);
    mem_write_cpu(watched_address + 2, 0x12);
    CHECK(gui_stops == 0 && debug.gdbWatch);
    debug_inst_start();
    CHECK(gui_stops == 1 && !debug.gdbWatch);
    CHECK(stopped_value == 0x123456);
    CHECK(mem_read_cpu(watched_address, false) == 0x56);
    CHECK(mem_read_cpu(watched_address + 1, false) == 0x34);
    CHECK(mem_read_cpu(watched_address + 2, false) == 0x12);
    CHECK(gui_stops == 1 && debug.gdbWatch);
    debug_inst_start();
    CHECK(gui_stops == 2 && !debug.gdbWatch);

    /* A wider GUI range can trigger before the first GDB-owned byte. The
       dummy connected handle is used only for memory accesses, never I/O. */
    CHECK(change_breakpoint(4, watched_address, 3, false));
    CHECK(change_breakpoint(4, watched_address + 1, 2, true));
    socket_fd = (gdb_socket_t)0;
    mem_write_cpu(watched_address, 0x9a);
    CHECK(debug.gdbWatch && !pending_watch_gdb);
    mem_write_cpu(watched_address + 1, 0x78);
    mem_write_cpu(watched_address + 2, 0x56);
    CHECK(gui_stops == 2 && pending_watch_gdb);
    CHECK(pending_watch_address == watched_address + 1);
    socket_fd = GDB_INVALID_SOCKET;
    debug_inst_start();
    CHECK(gui_stops == 3 && stopped_value == 0x56789a);
    socket_fd = (gdb_socket_t)0;
    CHECK(mem_read_cpu(watched_address, false) == 0x9a);
    CHECK(mem_read_cpu(watched_address + 1, false) == 0x78);
    CHECK(mem_read_cpu(watched_address + 2, false) == 0x56);
    CHECK(gui_stops == 3 && pending_watch_gdb);
    CHECK(pending_watch_address == watched_address + 1);
    socket_fd = GDB_INVALID_SOCKET;
    debug_inst_start();
    CHECK(gui_stops == 4 && !debug.gdbWatch);

    /* Removing GDB ownership leaves the GUI watches working immediately. */
    CHECK(change_breakpoint(4, watched_address + 1, 2, false));
    mem_write_cpu(watched_address, 0xab);
    CHECK(gui_stops == 5 && stopped_value == 0x56789a);
    CHECK(mem_read_cpu(watched_address, false) == 0xab);
    CHECK(gui_stops == 6 && stopped_value == 0x5678ab);
    CHECK(!debug.gdbWatch);
    debug_free();
    mem_free();
    puts("GDB/GUI memory watchpoint tests passed");
    return 0;
}
