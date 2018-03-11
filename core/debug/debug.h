#ifdef DEBUG_SUPPORT

#ifndef DEBUG_H
#define DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../defines.h"
#include <stdint.h>
#include <stdbool.h>

#define VERSION_DBG 0x0003

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

enum {
    CMD_NONE,
    CMD_ABORT,             /* abort() routine hit */
    CMD_DEBUG,             /* debugger() routine hit */
    CMD_SET_BREAKPOINT,    /* set a breakpoint with the value in DE; enabled */
    CMD_REM_BREAKPOINT,    /* remove a breakpoint with the value in DE */
    CMD_SET_R_WATCHPOINT,  /* set a read watchpoint with the value in DE; length in C */
    CMD_SET_W_WATCHPOINT,  /* set a write watchpoint with the value in DE; length in C */
    CMD_SET_RW_WATCHPOINT, /* set a read/write watchpoint with the value in DE; length in C */
    CMD_REM_WATCHPOINT,    /* we need to remove a watchpoint with the value in DE */
    CMD_REM_ALL_BREAK,     /* we need to remove all breakpoints */
    CMD_REM_ALL_WATCH,     /* we need to remove all watchpoints */
    CMD_SET_E_WATCHPOINT   /* set an empty watchpoint with the value in DE; length in C */
};

/* For Port Monitoring */
#define DBG_MASK_NONE            (0 << 0)
#define DBG_MASK_PORT_READ       (1 << 0)
#define DBG_MASK_PORT_WRITE      (1 << 1)
#define DBG_MASK_PORT_FREEZE     (1 << 2)

/* For Memory Brakpoints */
#define DBG_MASK_READ            (1 << 0)
#define DBG_MASK_WRITE           (1 << 1)
#define DBG_MASK_EXEC            (1 << 2)
#define DBG_MASK_TEMP_EXEC       (1 << 3)
#define DBG_MASK_RW              ((DBG_MASK_READ) | (DBG_MASK_WRITE))

/* For other things */
#define DBG_INST_START_MARKER    (1 << 4)
#define DBG_INST_MARKER          (1 << 5)

#define DBG_PORT_RANGE            0xFFFF00
#define DBGOUT_PORT_RANGE         0xFB0000
#define DBGERR_PORT_RANGE         0xFC0000
#define SIZEOF_DBG_BUFFER         0x1000

typedef struct {
    uint32_t cpuCycles, cpuNext;
    uint64_t cpuBaseCycles, cpuHaltCycles, dmaCycles;
    char buffer[SIZEOF_DBG_BUFFER];
    char bufferErr[SIZEOF_DBG_BUFFER];
    bool stepOverFirstStep;
    int8_t stepOutWait;
    int64_t totalCycles;
    uint8_t stepOverMode;
    uint32_t stepOutSPL;
    uint16_t stepOutSPS;
    uint32_t stepOverEnd;
    uint32_t runUntilAddr;
    uint32_t bufPos;
    uint32_t bufErrPos;
    uint8_t *addr;
    uint8_t *port;
    bool open;
    bool ignore; /* not thread safe! */
    bool commands; /* not thread safe! */
    bool openOnReset; /* not thread safe! */
} debug_state_t;

extern debug_state_t debug;

/* main interface functions */
/* if using, please read the comments */
void debug_init(void);                                               /* call before starting emulation */
void debug_free(void);                                               /* call after emulation end */
void debug_set_pc(uint32_t addr);                                    /* when in gui debug set program counter */
void debug_breakwatch(uint32_t addr, unsigned int type, bool set);   /* set a breakpoint or a watchpoint */
void debug_ports(uint16_t addr, unsigned int type, bool set);        /* set port monitor flags */
void debug_step(int mode, uint32_t addr);                            /* set a step mode, addr points to the instruction after pc */

/* internal core functions (no need to use or worry about) */
void debug_open(int reason, uint32_t data);
void debug_step_switch(void);
void debug_step_reset(void);

#ifdef __cplusplus
}
#endif

#endif

#endif
