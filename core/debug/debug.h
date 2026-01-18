#ifndef DEBUG_H
#define DEBUG_H

#include "../defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define VERSION_DBG 0x0004

/* main user interface */
/* please read the comments */

/* reason for debug trigger */
enum {
    DBG_USER,                /* request to open the debugger externally */
    DBG_READY,               /* if reset, debugger ready for new commands */
    DBG_FROZEN,              /* di \ halt */
    DBG_BREAKPOINT,          /* hit a breakpoint */
    DBG_WATCHPOINT_READ,     /* hit a read watchpoint */
    DBG_WATCHPOINT_WRITE,    /* hit a write watchpoint */
    DBG_PORT_READ,           /* read a monitored port */
    DBG_PORT_WRITE,          /* wrote a monitored port */
    DBG_REG_READ,            /* read of a watched CPU register */
    DBG_REG_WRITE,           /* write of a watched CPU register */
    DBG_NMI_TRIGGERED,       /* triggered a non maskable interrupt */
    DBG_WATCHDOG_TIMEOUT,    /* watchdog timer reset */
    DBG_MISC_RESET,          /* miscellaneous reset */
    DBG_STEP,                /* step command executed */
    DBG_BASIC_RECONFIG,      /* basic mode reconfiguration needed */
    DBG_BASIC_STEP,          /* basic step command executed */
    DBG_BASIC_LIVE_START,
    DBG_BASIC_BEGPC_READ,    /* begpc read */
    DBG_BASIC_CURPC_READ,    /* curpc read */
    DBG_BASIC_ENDPC_READ,    /* endpc read */
    DBG_BASIC_BEGPC_WRITE,   /* begpc write */
    DBG_BASIC_CURPC_WRITE,   /* curpc write */
    DBG_BASIC_ENDPC_WRITE,   /* endpc write */
    DBG_BASIC_BASIC_PROG_WRITE, /* basic_prog write */
    DBG_BASIC_LIVE_END,
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
int debug_get_flags(void);
void debug_enable_basic_mode(bool fetches, bool live);
void debug_disable_basic_mode(void);
bool debug_get_executing_basic_prgm(char *name);

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
#define DBGEXT_PORT           0xFD0000
#define DBG_STACK_SIZE        0x100
#define DBG_STACK_MASK        (DBG_STACK_SIZE-1)
#define DBG_ADDR_SIZE         0x1000000
#define DBG_PORT_SIZE         0x10000
#define SIZEOF_DBG_BUFFER     0x1000

/* tios specific debugging locations */
#define DBG_BASIC_BEGPC             0xD02317
#define DBG_BASIC_CURPC             0xD0231A
#define DBG_BASIC_ENDPC             0xD0231D
#define DBG_BASIC_BASIC_PROG        0xD0230E
#define DBG_BASIC_NEWDISPF          0xD00088
#define DBG_BASIC_CMDFLAGS          0xD0008C
#define DBG_BASIC_SYSHOOKFLAG2      0xD000B6
#define DBG_BASIC_PARSER_ACTIVE_BIT (1 << 1)
#define DBG_BASIC_CMDEXEC_BIT       (1 << 6)
#define DBG_BASIC_PROGEXECUTING_BIT (1 << 1)

#ifdef DEBUG_SUPPORT
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
    uint64_t cpuBaseCycles;
    uint64_t cpuHaltCycles;
    int64_t totalCycles, dmaCycles;
    uint32_t flashCacheMisses, flashTotalAccesses, flashWaitStates;
    int64_t flashDelayCycles;
    bool step, stepOver;
    uint32_t tempExec, stepOut;
    bool untilRet;
    uint32_t untilRetBase; /* normalized 24bit stack pointer baseline */
    uint32_t untilRetIndex; /* call-stack index when DBG_UNTIL_RET started */

    uint32_t stackIndex, stackSize;
    debug_stack_entry_t *stack;

    char buffer[SIZEOF_DBG_BUFFER];
    char bufferErr[SIZEOF_DBG_BUFFER];
    uint32_t bufErrPos;
    uint32_t bufPos;

    uint8_t *addr;
    uint8_t *port;

    uint64_t reg_watch_r; /* bitmask of DBG_MASK_READ for dbg_reg_t ids */
    uint64_t reg_watch_w; /* bitmask of DBG_MASK_WRITE for dbg_reg_t ids */
    bool basicMode;
    bool basicModeLive;
    bool basicDeferPC;
    bool stepBasic;
    bool stepBasicNext;
    uint32_t basicLastHookPC;
    uint32_t stepBasicFromPC;
    uint16_t stepBasicBegin;
    uint16_t stepBasicEnd;
    char stepBasicPrgm[10];
} debug_state_t;

extern debug_state_t debug;

enum {
    DBG_STEP_IN=DBG_STEP+1,
    DBG_STEP_OUT,
    DBG_STEP_OVER,
    DBG_STEP_NEXT,
    DBG_RUN_UNTIL,
    DBG_UNTIL_RET,
    DBG_BASIC_STEP_IN,
    DBG_BASIC_STEP_NEXT,
};

/* internal core functions */
void debug_step_switch(void);
void debug_clear_step(void);
void debug_clear_basic_step(void);
void debug_until_ret_handle_indirect_jump(uint32_t target, uint32_t currentSp);
#endif

/* register watchpoints */
/* these ids correspond to logical CPU registers shown in the UI */
typedef enum {
    DBG_REG_A = 0,
    DBG_REG_F,
    DBG_REG_B,
    DBG_REG_C,
    DBG_REG_D,
    DBG_REG_E,
    DBG_REG_H,
    DBG_REG_L,
    DBG_REG_IXH,
    DBG_REG_IXL,
    DBG_REG_IYH,
    DBG_REG_IYL,
    DBG_REG_AP,   /* A' */
    DBG_REG_FP,   /* F' */
    DBG_REG_BP,   /* B' */
    DBG_REG_CP,   /* C' */
    DBG_REG_DP,   /* D' */
    DBG_REG_EP,   /* E' */
    DBG_REG_HP,   /* H' */
    DBG_REG_LP,   /* L' */
    DBG_REG_AF,   /* 16-bit */
    DBG_REG_BC,   /* 24-bit */
    DBG_REG_DE,   /* 24-bit */
    DBG_REG_HL,   /* 24-bit */
    DBG_REG_IX,   /* 24-bit */
    DBG_REG_IY,   /* 24-bit */
    DBG_REG_AFP,  /* 16-bit */
    DBG_REG_BCP,  /* 24-bit */
    DBG_REG_DEP,  /* 24-bit */
    DBG_REG_HLP,  /* 24-bit */
    DBG_REG_SPS,  /* 16-bit */
    DBG_REG_SPL,  /* 24-bit */
    DBG_REG_PC,   /* 24-bit */
    DBG_REG_I,    /* 16-bit */
    DBG_REG_R,    /* 8-bit  */
    DBG_REG_MBASE,/* 8-bit  */
    DBG_REG_COUNT
} dbg_reg_t;

#ifdef DEBUG_SUPPORT
/* enable/disable register watch for a given id and mask (DBG_MASK_READ/WRITE) */
void debug_reg_watch(unsigned regID, int mask, bool set);
/* get current mask (DBG_MASK_READ/WRITE) for a register id */
int  debug_reg_get_mask(unsigned regID);
/* helpers to be invoked around register accesses */
void debug_touch_reg_read(unsigned regID);
void debug_touch_reg_write(unsigned regID, uint32_t oldValue, uint32_t new_value);
/* normalize a register value to its natural width (8/16/24) */
uint32_t debug_norm_reg_value(unsigned regID, uint32_t value);
#else
static inline void debug_reg_watch(unsigned regID, int mask, bool set) {
    (void)regID;
    (void)mask;
    (void)set;
}

static inline int debug_reg_get_mask(unsigned regID) {
    (void)regID;
    return 0;
}

static inline void debug_touch_reg_read(unsigned regID) {
    (void)regID;
}

static inline void debug_touch_reg_write(unsigned regID, uint32_t oldValue, uint32_t new_value) {
    (void)regID;
    (void)oldValue;
    (void)new_value;
}

static inline uint32_t debug_norm_reg_value(unsigned regID, uint32_t value) {
    (void)regID;
    return value;
}
#endif

#ifdef DEBUG_SUPPORT
/* direct touch helper for write only sites */
#define DBG_REG_TOUCH_W(ID, OLD, NEW) \
    do { debug_touch_reg_write((unsigned)(DBG_REG_##ID), (uint32_t)(OLD), (uint32_t)(NEW)); } while (0)

/* trigger helpers to wrap reads/writes
 * REG_READ returns the single evaluated value of EXPR
 * REG_WRITE evaluates LVAL once to capture OLD, and again to assign
 */
#define REG_READ_EX(ID, EXPR) \
    (__extension__({ \
        uint32_t __v = (uint32_t)(EXPR); \
        debug_touch_reg_read((unsigned)(DBG_REG_##ID)); \
        __v; \
    }))

#define REG_WRITE_EX(ID, LVAL, VAL) \
    (__extension__({ \
        uint32_t __old = (uint32_t)(LVAL); \
        uint32_t __new = (uint32_t)(VAL); \
        debug_touch_reg_write((unsigned)(DBG_REG_##ID), __old, __new); \
        (LVAL) = (__new); \
    }))
#else
#define DBG_REG_TOUCH_W(ID, OLD, NEW) \
    do { \
        (void)(DBG_REG_##ID); \
        (void)(OLD); \
        (void)(NEW); \
    } while (0)

#define REG_READ_EX(ID, EXPR) \
    (__extension__({ \
        (void)(DBG_REG_##ID); \
        (uint32_t)(EXPR); \
    }))

#define REG_WRITE_EX(ID, LVAL, VAL) \
    (__extension__({ \
        (void)(DBG_REG_##ID); \
        uint32_t __new = (uint32_t)(VAL); \
        (LVAL) = (__new); \
    }))
#endif

/* map CPU context to register IDs */
/* eZ80 PREFIX: 0 = HL, 2 = IX, 3 = IY */
#define DBG_REG_ID_INDEX(PFX)   (( (PFX) == 0 ) ? DBG_REG_HL  : ( (PFX) == 2 ? DBG_REG_IX  : DBG_REG_IY ))
#define DBG_REG_ID_INDEX_H(PFX) (( (PFX) == 0 ) ? DBG_REG_H   : ( (PFX) == 2 ? DBG_REG_IXH : DBG_REG_IYH ))
#define DBG_REG_ID_INDEX_L(PFX) (( (PFX) == 0 ) ? DBG_REG_L   : ( (PFX) == 2 ? DBG_REG_IXL : DBG_REG_IYL ))

#define DBG_REG_ID_SP(MODE_L)   ( (MODE_L) ? DBG_REG_SPL : DBG_REG_SPS )

#ifdef __cplusplus
}
#endif

#endif
