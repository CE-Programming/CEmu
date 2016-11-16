#ifndef DEBUGGER_H
#define DEBUGGER_H

enum debugCMDS {
    CMD_ABORT=1,           // abort() routine hit
    CMD_DEBUG,             // debugger() routine hit
    CMD_SET_BREAKPOINT,    // set a breakpoint with the value in DE; enabled
    CMD_REM_BREAKPOINT,    // remove a breakpoint with the value in DE
    CMD_SET_R_WATCHPOINT,  // set a read watchpoint with the value in DE; length in C
    CMD_SET_W_WATCHPOINT,  // set a write watchpoint with the value in DE; length in C
    CMD_SET_RW_WATCHPOINT, // set a read/write watchpoint with the value in DE; length in C
    CMD_REM_WATCHPOINT,    // we need to remove a watchpoint with the value in DE
    CMD_REM_ALL_BREAK,     // we need to remove all breakpoints
    CMD_REM_ALL_WATCH,     // we need to remove all watchpoints
    CMD_SET_E_WATCHPOINT   // set an empty watchpoint with the value in DE; length in C
};

enum openDebuggerMode {
    DBG_OPEN=0,
    DBG_SAVE
};

#define DBG_VERSION 1

#endif
