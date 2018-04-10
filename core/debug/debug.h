#ifdef DEBUG_SUPPORT

#ifndef DEBUG_H
#define DEBUG_H

#include "../atomics.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "../defines.h"
#include <stdint.h>
#include <stdbool.h>

#define VERSION_DBG 0x0003

/* main user interface */
/* please read the comments */

/* reason for debug trigger */
enum {
    DBG_USER,              /* request to open the debugger externally */
    DBG_READY,             /* if reset, debugger ready for new commands */
    DBG_FROZEN,            /* di \ halt */
    DBG_BREAKPOINT,        /* hit a breakpoint */
    DBG_WATCHPOINT_READ,   /* hit a read watchpoint */
    DBG_WATCHPOINT_WRITE,  /* hit a write watchpoint */
    DBG_PORT_READ,         /* read a monitored port */
    DBG_PORT_WRITE,        /* wrote a monitored port */
    DBG_NMI_TRIGGERED,     /* triggered a non maskable interrupt */
    DBG_WATCHDOG_TIMEOUT,  /* watchdog timer reset */
    DBG_MISC_RESET,        /* miscellaneous reset */
    DBG_STEP,              /* step command executed */
    DBG_NUMBER
};

/* available software commands */
enum {
    CMD_NONE,
    CMD_ABORT,             /* abort() routine hit */
    CMD_DEBUG,             /* debugger() routine hit */
    CMD_SET_BREAKPOINT,    /* enable breakpoint: DE=address */
    CMD_REM_BREAKPOINT,    /* remove breakpoint: DE=address */
    CMD_SET_R_WATCHPOINT,  /* set read watchpoint: DE=address | C=length */
    CMD_SET_W_WATCHPOINT,  /* set write watchpoint: DE=address | C=length */
    CMD_SET_RW_WATCHPOINT, /* set read/write watchpoint: DE=address | C=length */
    CMD_REM_WATCHPOINT,    /* remove watchpoint: DE=address */
    CMD_REM_ALL_BREAK,     /* remove all breakpoints */
    CMD_REM_ALL_WATCH,     /* remove all watchpoints */
    CMD_SET_E_WATCHPOINT,  /* set empty watchpoint: DE=address | C=length */
    CMD_NUMBER
};

/* interface functions */
void debug_init(void);                               /* call before starting emulation */
void debug_free(void);                               /* call after emulation end */
void debug_set_pc(uint32_t addr);                    /* when in gui debug set program counter */
void debug_inst_start(void);
void debug_inst_fetch(void);
void debug_inst_repeat(void);
void debug_record_call(uint32_t retAddr, bool stack);
void debug_record_ret(uint32_t retAddr, bool stack);
void debug_watch(uint32_t addr, int mask, bool set); /* set a breakpoint or a watchpoint */
void debug_ports(uint16_t addr, int mask, bool set); /* set port monitor flags */
void debug_flag(int mask, bool set);                 /* configure setup of debug core */
void debug_step(int mode, uint32_t addr);            /* set a step mode, addr points to the instruction after pc */
void debug_open(int reason, uint32_t data);          /* open the debugger (Should only be called from gui_do_stuff) */
bool debug_is_open(void);                            /* returns the status of the core debugger */

/* masks */
#define DBG_MASK_NONE         (0 << 0)
#define DBG_IGNORE            (1 << 0)   /* ignore any breakpoints, watchpoints, or similar */
#define DBG_SOFT_COMMANDS     (1 << 1)   /* allow software commands from executing code */
#define DBG_OPEN_ON_RESET     (1 << 2)   /* open the debugger when a reset is triggered */

/* port monitoring */
#define DBG_MASK_PORT_READ    (1 << 0)   /* port monitor reads */
#define DBG_MASK_PORT_WRITE   (1 << 1)   /* port monitor writes */
#define DBG_MASK_PORT_FREEZE  (1 << 2)   /* port freeze value */

/* breakpoints / watchpoints */
#define DBG_MASK_READ         (1 << 0)   /* read watchpoint */
#define DBG_MASK_WRITE        (1 << 1)   /* write watchpoint */
#define DBG_MASK_EXEC         (1 << 2)   /* breakpoint */
#define DBG_MASK_RW           ((DBG_MASK_READ) | (DBG_MASK_WRITE))


/* internal items below this line */
#define DBG_INST_START_MARKER (1 << 3)
#define DBG_INST_MARKER       (1 << 4)

#define DBG_PORT_RANGE        0xFFFF00
#define DBGOUT_PORT_RANGE     0xFB0000
#define DBGERR_PORT_RANGE     0xFC0000
#define DBG_STACK_SIZE        0x1000
#define DBG_STACK_MASK        (DBG_STACK_SIZE-1)
#define DBG_ADDR_SIZE         0x1000000
#define DBG_PORT_SIZE         0x10000
#define SIZEOF_DBG_BUFFER     0x1000

typedef struct {
    bool mode : 1;
    bool popped : 1;
    uint32_t stack : 24;
    uint32_t retAddr : 24;
    uint8_t range : 8;
} debug_stack_entry_t;

typedef struct {
    uint32_t cpuNext;
    uint32_t cpuCycles;
    uint64_t dmaCycles;
    uint64_t cpuBaseCycles;
    uint64_t cpuHaltCycles;
    int64_t totalCycles;
    bool step, stepOver;
    uint32_t tempExec, stepOut;

    uint32_t stackIndex, stackSize;
    debug_stack_entry_t *stack;

    char buffer[SIZEOF_DBG_BUFFER];
    char bufferErr[SIZEOF_DBG_BUFFER];
    uint32_t bufErrPos;
    uint32_t bufPos;

    uint8_t *addr;
    uint8_t *port;
    _Atomic(int) flags;
    _Atomic(bool) open;
    _Atomic(bool) ignore;
    _Atomic(bool) commands;
    _Atomic(bool) openOnReset;
} debug_state_t;

extern debug_state_t debug;

enum {
    DBG_STEP_IN=DBG_STEP+1,
    DBG_STEP_OUT,
    DBG_STEP_OVER,
    DBG_STEP_NEXT,
    DBG_RUN_UNTIL
};

/* internal core functions */
void debug_step_switch(void);
void debug_clear_step(void);

#ifdef __cplusplus
}
#endif

#endif

#endif
