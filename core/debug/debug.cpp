#ifdef DEBUG_SUPPORT

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "disasm.h"
#include "debug.h"
#include "../mem.h"
#include "../emu.h"
#include "../cpu.h"

#define SIZEOF_PORT_ADDRESSABLE (0x10000U)
#define SIZEOF_ADDRESSABLE (0x1000000U)

volatile bool inDebugger = false;
debug_state_t debugger;

#ifndef __EMSCRIPTEN__
#include <condition_variable>
static std::mutex debugM;
static std::condition_variable debugCV;
#endif

void debugger_init(void) {
    debugger.stepOverInstrEnd = -1;
    debugger.data.ports = (uint8_t*)calloc(SIZEOF_PORT_ADDRESSABLE, 1);
    debugger.data.block = (uint8_t*)calloc(SIZEOF_ADDRESSABLE, 1);
    debugger.buffer = (char*)malloc(SIZEOF_DBG_BUFFER);      /* Used for printing to the console */
    debugger.bufferErr = (char*)malloc(SIZEOF_DBG_BUFFER);   /* Used for printing to the console */
    debugger.bufferPos = 0;
    debugger.bufferErrPos = 0;
    debugger.granularity = 0;
    debugger.profile.cycles = NULL;

    gui_console_printf("[CEmu] Initialized Debugger...\n");
}

void debugger_free(void) {
    free(debugger.profile.cycles);
    free(debugger.data.block);
    free(debugger.data.ports);
    free(debugger.buffer);
    free(debugger.bufferErr);
    gui_console_printf("[CEmu] Freed Debugger.\n");
}

void debug_profile_enable(void) {
#ifdef PROFILE_SUPPORT
    if (debugger.profile.cycles == NULL) {
        debugger.profile.cycles = (uint64_t*)calloc(SIZEOF_ADDRESSABLE >> debugger.granularity, sizeof(uint64_t));
    }
#endif
}

void debug_profile_disable(void) {
#ifdef PROFILE_SUPPORT
    free(debugger.profile.cycles);
#endif
    debugger.profile.cycles = NULL;
}

static int debug_compare_cycles(const void *a, const void *b) {
    if (((profiler_item_t*)a)->cycles < ((profiler_item_t*)b)->cycles) {
        return -1;
    }
    if (((profiler_item_t*)a)->cycles == ((profiler_item_t*)b)->cycles) {
        return 0;
    }
    return 1;
}

static int debug_compare_percentage(const void *a, const void *b) {
    if (((profiler_item_t*)a)->percentage > ((profiler_item_t*)b)->percentage) {
        return -1;
    }
    if (((profiler_item_t*)a)->percentage == ((profiler_item_t*)b)->percentage) {
        return 0;
    }
    return 1;
}

void debug_profile_export(const char *path) {
    uint32_t i, j;
    static const char *str[] = {
        "address",
        "percentage",
        "cycles"
    };

    uint64_t max = 0, min = UINT64_MAX;
    uint64_t avg = 0, tot = 0;
    uint32_t len = 1, end = 0;
    for (i = 0; i < (SIZEOF_ADDRESSABLE >> debugger.granularity); i++) {
        if (debugger.profile.cycles[i]) {
            tot += debugger.profile.cycles[i];
            len++; end = i;
        }
        if (debugger.profile.cycles[i] < min) {
            min = debugger.profile.cycles[i];
        }
        if (debugger.profile.cycles[i] > max) {
            max = debugger.profile.cycles[i];
        }
    }

    avg = tot / len;
    len--;

    if (!len) {
        return;
    }

    profiler_item_t *tmp = (profiler_item_t*)malloc(len * sizeof(profiler_item_t));
    for (j = i = 0; i < (SIZEOF_ADDRESSABLE >> debugger.granularity) && j != end; i++) {
        if (debugger.profile.cycles[i]) {
            tmp[j].cycles = debugger.profile.cycles[i];
            tmp[j].percentage = static_cast<double>(debugger.profile.cycles[i]) / static_cast<double>(avg);
            tmp[j].address = i;
            j++;
        }
    }

    FILE *file = fopen(path, "w");
    fprintf(file, "CEmu profiler information\n");
    fprintf(file, "sort %s\n", str[debugger.sort]);
    fprintf(file, "granularity %u\n\n", debugger.granularity);
    fprintf(file, "addr   cycles               %% of average\n"
                  "------ -------------------- ------------\n");

    switch (debugger.sort) {
        case PROFILE_SORT_ADDRESS:
            break;
        case PROFILE_SORT_CYCLES:
            qsort(tmp, len, sizeof(profiler_item_t), debug_compare_cycles);
            break;
        case PROFILE_SORT_PERCENTAGE:
            qsort(tmp, len, sizeof(profiler_item_t), debug_compare_percentage);
            break;
        default:
            break;
    }

    for (i = 0; i < len - 1; i++) {
        char cycles[30], percent[30];
        sprintf(cycles, "%" PRIu64, tmp[i].cycles);
        sprintf(percent, "%.2f", tmp[i].percentage);
        fprintf(file, "%06X %20s %12s\n", tmp[i].address, cycles, percent);
    }

    free(tmp);
    fprintf(file, "end\n");
    fclose(file);

    debugger.totalProfile = tot;
}

uint8_t debug_peek_byte(uint32_t addr) {
    uint8_t value = mem_peek_byte(addr), data;

    if ((data = debugger.data.block[addr])) {
        disasmHighlight.rWatch |= data & DBG_READ_WATCHPOINT ? true : false;
        disasmHighlight.wWatch |= data & DBG_WRITE_WATCHPOINT ? true : false;
        disasmHighlight.xBreak |= data & DBG_EXEC_BREAKPOINT ? true : false;
        if (data & DBG_INST_START_MARKER && disasmHighlight.addr < 0) {
            disasmHighlight.addr = addr;
        }
    }

    if (cpu.registers.PC == addr) {
        disasmHighlight.pc = true;
    }

    return value;
}

void close_debugger(void) {
#ifndef __EMSCRIPTEN__
    std::unique_lock<std::mutex> lock(debugM);
    debugCV.notify_all();
#endif
    inDebugger = false;
}

void open_debugger(int reason, uint32_t data) {
    if (inDebugger) {
        return;
    }

    if (debugger.ignore && (reason == HIT_PORT_READ_WATCHPOINT || reason == HIT_PORT_WRITE_WATCHPOINT ||
                            reason == HIT_READ_WATCHPOINT || reason == HIT_WRITE_WATCHPOINT ||
                            reason == HIT_EXEC_BREAKPOINT)) {
        return;
    }

    if ((reason == DBG_STEP) && debugger.stepOverFirstStep) {
        if (((cpu.events & EVENT_DEBUG_STEP_NEXT)
                && !(debugger.data.block[cpu.registers.PC] & DBG_TEMP_EXEC_BREAKPOINT)) || (cpu.events & EVENT_DEBUG_STEP_OUT)) {
            debugger.stepOverFirstStep = false;
            gui_debugger_raise_or_disable(false);
            return;
        }
        debug_clear_temp_break();
    }

    bool ignoreDmaCycles = debugger.ignoreDmaCycles;
    debugger.cpuCycles = cpu.cycles;
    debugger.cpuNext = cpu.next;
    debugger.cpuBaseCycles = cpu.baseCycles;
    debugger.cpuHaltCycles = cpu.haltCycles;
    debugger.cpuDmaCycles = cpu.dmaCycles;
    debugger.cycleCount += cpu_total_cycles();
    if (ignoreDmaCycles) {
        debugger.cycleCount -= cpu.dmaCycles;
    }

    if (debugger.bufferPos) {
        debugger.buffer[debugger.bufferPos] = '\0';
        gui_console_printf("%s", debugger.buffer);
        debugger.bufferPos = 0;
    }

    if (debugger.bufferErrPos) {
        debugger.bufferErr[debugger.bufferErrPos] = '\0';
        gui_console_err_printf("%s", debugger.bufferErr);
        debugger.bufferErrPos = 0;
    }

    inDebugger = true;

#ifndef __EMSCRIPTEN__
    std::unique_lock<std::mutex> lock(debugM);
#endif
    gui_debugger_send_command(reason, data);
#ifndef __EMSCRIPTEN__
    debugCV.wait(lock);
#endif

    cpu.next = debugger.cpuNext;
    cpu.cycles = debugger.cpuCycles;
    cpu.baseCycles = debugger.cpuBaseCycles;
    cpu.haltCycles = debugger.cpuHaltCycles;
    cpu.dmaCycles = debugger.cpuDmaCycles;
    debugger.cycleCount -= cpu_total_cycles();
    if (ignoreDmaCycles) {
        debugger.cycleCount += cpu.dmaCycles;
    }

    if (cpu.events & EVENT_DEBUG_STEP && !cpu.halted) {
        cpu.next = cpu.cycles + 1;
    }
}

void debug_switch_step_mode(void) {
    if (cpu.events & EVENT_DEBUG_STEP_OVER) {
        debugger.stepOverFirstStep = true;
        debugger.stepOutSPL = cpu.registers.SPL + 1;
        debugger.stepOutSPS = cpu.registers.SPS + 1;
        debugger.stepOutWait = 0;
        cpu.events &= ~EVENT_DEBUG_STEP_OVER;
        cpu.events |= EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OUT;
    }
}

void debug_breakwatch(uint32_t address, unsigned int type, bool set) {
    if (set) {
        debugger.data.block[address] |= type;
    } else {
        debugger.data.block[address] &= ~type;
    }
}

void debug_init_run_until(uint32_t address) {
    debugger.runUntilAddress = address;
}

void debug_clear_temp_break(void) {
    cpu.events &= ~(EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OUT | EVENT_DEBUG_STEP_OVER);
    if (debugger.stepOverInstrEnd != 0xFFFFFFFFU) {
        do {
            debugger.data.block[debugger.stepOverInstrEnd] &= ~DBG_TEMP_EXEC_BREAKPOINT;
            debugger.stepOverInstrEnd = cpu_mask_mode(debugger.stepOverInstrEnd - 1, debugger.stepOverMode);
        } while (debugger.data.block[debugger.stepOverInstrEnd] & DBG_TEMP_EXEC_BREAKPOINT);
    }
    debugger.stepOverInstrEnd = 0xFFFFFFFFU;
}

void debug_set_pc_address(uint32_t address) {
    cpu_flush(address, cpu.ADL);
}

void debug_breakpoint_remove(uint32_t address) {
    debug_breakwatch(address, ~DBG_NO_HANDLE, false);
}

void debug_pmonitor_set(uint16_t address, unsigned int type, bool set) {
    if (set) {
        debugger.data.ports[address] |= type;
    } else {
        debugger.data.ports[address] &= ~type;
    }
}

void debug_pmonitor_remove(uint16_t address) {
    debug_pmonitor_set(address, ~DBG_NO_HANDLE, false);
}

#endif
