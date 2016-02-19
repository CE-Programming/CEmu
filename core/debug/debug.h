#ifdef DEBUG_SUPPORT

#ifndef DEBUG_H
#define DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../defines.h"
#include "../apb.h"

extern volatile bool inDebugger;

eZ80portrange_t init_debugger_ports(void);

/* For use in the debugger */
enum {
    DBG_USER,
    DBG_STEP,
    HIT_EXEC_BREAKPOINT,
    HIT_READ_BREAKPOINT,
    HIT_WRITE_BREAKPOINT,
    HIT_RUN_BREAKPOINT,
    HIT_PORT_WRITE_BREAKPOINT,
    HIT_PORT_READ_BREAKPOINT,
    NUM_DBG_COMMANDS,
};

/* For Port Monitoring */
#define DBG_NO_HANDLE             0
#define DBG_PORT_READ             1
#define DBG_PORT_WRITE            2
#define DBG_PORT_FREEZE           4

/* For Memory Brakpoints */
#define DBG_READ_BREAKPOINT       (1 << 0)
#define DBG_WRITE_BREAKPOINT      (1 << 1)
#define DBG_EXEC_BREAKPOINT       (1 << 2)
#define DBG_STEP_OVER_BREAKPOINT  (1 << 3)
#define DBG_RUN_UNTIL_BREAKPOINT  (1 << 4)
#define DBG_INST_START_MARKER     (1 << 5)
#define DBG_INST_MARKER           (1 << 6)

#define DBG_PORT_RANGE            0xFFFF00
#define CONSOLE_PORT_RANGE        0xFB0000

typedef struct {
    uint8_t *block;
    uint8_t *ports;
} debug_data_t;

typedef struct {        /* For debugging */
    int cpu_cycles;
    int cpu_next;
    uint32_t stepOverInstrEnd;
    uint32_t stepOverInstrSize;
    uint32_t stepOverExtendSize;
    uint8_t stepOverMode;
    uint32_t stepOutSPL;
    uint16_t stepOutSPS;
    int8_t stepOutWait;
    uint32_t runUntilAddress;
    bool runUntilSet;
    debug_data_t data;
} debug_state_t;

/* Debugging */
extern debug_state_t debugger;

void debugger_init(void);
void debugger_free(void);

uint8_t debug_read_byte(uint32_t address);
uint16_t debug_read_short(uint32_t address);
uint32_t debug_read_long(uint32_t address);
uint32_t debug_read_word(uint32_t address, bool mode);
void debug_write_byte(uint32_t address, uint8_t value);
uint8_t debug_port_read_byte(uint32_t address);
void debug_port_write_byte(uint32_t address, uint8_t value);
void open_debugger(int reason, uint32_t address);

void debug_toggle_run_until(uint32_t address);

void debug_breakpoint_set(uint32_t address, unsigned int type, bool set);
void debug_breakpoint_remove(uint32_t address);

void debug_pmonitor_set(uint16_t address, unsigned int type, bool set);
void debug_pmonitor_remove(uint16_t address);

void debug_set_pc_address(uint32_t address);

void debug_clear_run_until(void);
void debug_clear_step_over(void);

#ifdef __cplusplus
}
#endif

#endif

#endif
