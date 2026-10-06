/* Internal state tests complement the real CPU/GDB integration tests. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../core/debug/gdbstub.c"

eZ80cpu_t cpu;
debug_state_t debug;
static uint32_t new_pc;

void gui_console_printf(const char *format, ...) { (void)format; }
void gui_console_err_printf(const char *format, ...) { (void)format; }
uint8_t cpu_check_signals(void) { return 0; }
uint32_t cpu_address_mode(uint32_t addr, bool mode) {
    return mode ? addr & 0xffffff : (cpu.registers.MBASE << 16) | (addr & 0xffff);
}
bool debug_is_open(void) { return false; }
void debug_open(int reason, uint32_t data) { (void)reason; (void)data; }
void debug_set_pc(uint32_t addr) { new_pc = addr; }
void debug_step(int mode, uint32_t addr) { (void)mode; (void)addr; }
void debug_clear_step(void) { debug.step = false; }
uint8_t mem_peek_byte(uint32_t addr) { (void)addr; return 0; }
void mem_poke_byte(uint32_t addr, uint8_t value) { (void)addr; (void)value; }
void debug_watch(uint32_t addr, int mask, bool set) {
    if (set) {
        debug.addr[addr] |= mask;
    } else {
        debug.addr[addr] &= ~mask;
    }
}

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); abort(); \
} } while (0)

int main(void) {
    debug.addr = calloc(DBG_ADDR_SIZE, 1);
    CHECK(debug.addr);
    cpu.registers.MBASE = 0xd0;
    cpu.registers.AF = 0x1234;
    CHECK(gdb_get_reg(GDB_REG_AF) == 0x1234);
    gdb_write_reg(GDB_REG_AF, 0xabcd);
    CHECK(cpu.registers.MBASE == 0xd0);
    CHECK(cpu.registers.AF == 0xabcd);
    cpu.L = false;
    cpu.registers.SPS = 0x5678;
    cpu.registers.SPL = 0xd12345;
    CHECK(gdb_get_reg(GDB_REG_SP) == 0xd12345);
    gdb_write_reg(GDB_REG_SP, 0xd23456);
    CHECK(cpu.registers.SPL == 0xd23456);
    CHECK(cpu.registers.SPS == 0x5678);
    gdb_write_reg(GDB_REG_SPS, 0xabcd);
    CHECK(cpu.registers.SPL == 0xd23456);
    CHECK(cpu.registers.SPS == 0xabcd);

    /* GDB ownership must preserve GUI flags, counters, and instruction markers. */
    const uint32_t addr = 0xd12345;
    const int gui_flags = DBG_MASK_EXEC | DBG_MASK_READ | DBG_MASK_COUNT | DBG_INST_MARKER;
    debug.addr[addr] = gui_flags;
    CHECK(change_breakpoint(0, addr, 3, true));
    CHECK(change_breakpoint(0, addr, 3, true));
    CHECK(breakpoint_count == 1);
    CHECK(!(debug.addr[addr + 1] & DBG_MASK_GDB));
    CHECK(change_breakpoint(2, addr, 3, true));
    CHECK(change_breakpoint(4, addr + 1, 3, true));
    CHECK(gdbstub_watch_mask(addr) == (DBG_MASK_EXEC | DBG_MASK_WRITE));
    CHECK(change_breakpoint(2, addr, 3, false));
    CHECK(gdbstub_watch_mask(addr) == DBG_MASK_EXEC);
    CHECK(gdbstub_watch_mask(addr + 1) == DBG_MASK_RW);
    CHECK(!(debug.addr[addr + 4] & DBG_MASK_GDB));
    gdbstub_disconnect();
    CHECK(debug.addr[addr] == gui_flags);
    CHECK(!(debug.addr[addr + 1] & DBG_MASK_GDB));
    CHECK(!breakpoint_count);

    const char *ptr = "ffffffff";
    uint32_t value;
    CHECK(hex_to_uint(&ptr, &value) && value == UINT32_MAX && !*ptr);
    ptr = "100000000";
    CHECK(!hex_to_uint(&ptr, &value));
    CHECK(!valid_hex("1234", 3));
    CHECK(!valid_hex("1234zz", 3));
    CHECK(valid_hex("123456", 3));
    CHECK(!valid_range(0x1000000, 1));
    CHECK(!valid_range(0xffffff, 2));
    CHECK(valid_range(0xffffff, 1));
    new_pc = 0xd01234;
    CHECK(parse_resume('C', "05"));
    CHECK(new_pc == 0xd01234);
    CHECK(parse_resume('S', "05;d12345"));
    CHECK(new_pc == 0xd12345);
    CHECK(!parse_resume('c', "d12345junk"));
    bool step;
    char vcont[] = ";s:1;c";
    CHECK(parse_vcont(vcont, &step) && step);
    char bad_vcont[] = ";s:2";
    CHECK(!parse_vcont(bad_vcont, &step));
    CHECK(!gdbstub_init(0));
    CHECK(!gdbstub_init(65536));
    free(debug.addr);
    puts("GDB state tests passed");
    return 0;
}
