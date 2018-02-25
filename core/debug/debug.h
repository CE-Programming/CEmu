#ifdef DEBUG_SUPPORT

#ifndef DEBUG_H
#define DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../defines.h"
#include "../port.h"

extern volatile bool inDebugger;

eZ80portrange_t init_debugger_ports(void);

enum {
    DBG_USER,
    DBG_READY,
    DBG_EXEC_BREAKPOINT,
    DBG_READ_WATCHPOINT,
    DBG_WRITE_WATCHPOINT,
    DBG_PORT_READ,
    DBG_PORT_WRITE,
    DBG_NMI_TRIGGERED,
    DBG_WATCHDOG_TIMEOUT,
    DBG_MISC_RESET,
    DBG_STEP,
    DBG_STEP_IN,
    DBG_STEP_OUT,
    DBG_STEP_OVER,
    DBG_STEP_NEXT,
    DBG_RUN_UNTIL,
    DBG_NUM_COMMANDS,
};

/* For Port Monitoring */
#define DBG_MASK_NONE            0
#define DBG_MASK_PORT_READ       1
#define DBG_MASK_PORT_WRITE      2
#define DBG_MASK_PORT_FREEZE     4

/* For Memory Brakpoints */
#define DBG_MASK_READ            1
#define DBG_MASK_WRITE           2
#define DBG_MASK_EXEC            4
#define DBG_MASK_TEMP_EXEC       8
#define DBG_MASK_RW              ((DBG_MASK_READ) | (DBG_MASK_WRITE))

/* For other things */
#define DBG_INST_START_MARKER    16
#define DBG_INST_MARKER          32

#define DBG_PORT_RANGE            0xFFFF00
#define DBGOUT_PORT_RANGE         0xFB0000
#define DBGERR_PORT_RANGE         0xFC0000
#define SIZEOF_DBG_BUFFER         0x1000

typedef struct {
    uint8_t *block;
    uint8_t *ports;
} debug_data_t;

typedef struct {
    uint32_t cpuCycles, cpuNext;
    uint64_t cpuBaseCycles, cpuHaltCycles, dmaCycles;
    char *buffer;
    char *bufferErr;
    bool resetOpensDebugger;
    uint32_t stepOverInstrSize;
    uint8_t stepOverMode;
    uint32_t stepOutSPL;
    uint16_t stepOutSPS;
    uint32_t stepOverInstrEnd;
    uint32_t runUntilAddress;
    int8_t stepOutWait;
    bool stepOverFirstStep;
    bool ignore;
    debug_data_t data;
    volatile uint32_t bufferPos;
    volatile uint32_t bufferErrPos;
    int64_t totalCycles;
    bool commands;
} debug_state_t;

/* Debugging */
extern debug_state_t debugger;

void debugger_init(void);
void debugger_free(void);

/* Main interface functions */
void open_debugger(int reason, uint32_t address);
void close_debugger(void);

uint8_t debug_peek_byte(uint32_t address);
void debug_switch_step_mode(void);

void debug_breakwatch(uint32_t address, unsigned int type, bool set);
void debug_breakpoint_remove(uint32_t address);

void debug_pmonitor_set(uint16_t address, unsigned int type, bool set);
void debug_pmonitor_remove(uint16_t address);

void debug_set_pc_address(uint32_t address);

void debug_clear_temp_break(void);
void debug_set_step_mode(int mode);

#ifdef __cplusplus
}
#endif

#endif

#endif
