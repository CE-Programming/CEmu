#ifdef DEBUG_SUPPORT

#include "debug.h"
#include "stepping.h"
#include "disasm.h"
#include "../mem.h"
#include "../emu.h"
#include "../asic.h"

void debug_set_step_next(void) {
    debug_clear_temp_break();
    disasm.base_address = cpu.registers.PC;
    disasm.adl = cpu.ADL;
    disassembleInstruction();
    debugger.stepOverFirstStep = true;
    debugger.stepOverInstrEnd = disasm.new_address;
    debugger.data.block[debugger.stepOverInstrEnd] |= DBG_TEMP_EXEC_BREAKPOINT;
    debugger.stepOverMode = cpu.ADL;
    debugger.stepOutSPL = 0;
    debugger.stepOutSPS = 0;
    debugger.stepOutWait = -1;
    cpuEvents |= EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_NEXT;
}

void debug_set_step_in(void) {
    debug_clear_temp_break();
    disasm.base_address = cpu.registers.PC;
    disasm.adl = cpu.ADL;
    disassembleInstruction();
    debugger.stepOverInstrEnd = disasm.new_address;
    debugger.data.block[debugger.stepOverInstrEnd] |= DBG_TEMP_EXEC_BREAKPOINT;
    debugger.stepOverMode = cpu.ADL;
    debugger.stepOverFirstStep = false;
    cpuEvents |= EVENT_DEBUG_STEP;
}

void debug_set_step_over(void) {
    debug_clear_temp_break();
    disasm.base_address = cpu.registers.PC;
    disasm.adl = cpu.ADL;
    disassembleInstruction();
    debugger.stepOverInstrEnd = disasm.new_address;
    debugger.data.block[debugger.stepOverInstrEnd] |= DBG_TEMP_EXEC_BREAKPOINT;
    debugger.stepOverMode = cpu.ADL;
    debugger.stepOverFirstStep = false;
    cpuEvents |= EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OVER;
}

void debug_set_run_until(void) {
    debug_clear_temp_break();
    debugger.stepOverInstrEnd = debugger.runUntilAddress;
    debugger.data.block[debugger.stepOverInstrEnd] |= DBG_TEMP_EXEC_BREAKPOINT;
    debugger.stepOverMode = cpu.ADL;
    debugger.stepOverFirstStep = false;
    cpuEvents &= ~(EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OVER | EVENT_DEBUG_STEP_OUT | EVENT_DEBUG_STEP_NEXT);
}

void debug_set_step_out(void) {
    debug_clear_temp_break();
    debugger.stepOverFirstStep = true;
    debugger.stepOutSPL = cpu.registers.SPL + 1;
    debugger.stepOutSPS = cpu.registers.SPS + 1;
    debugger.stepOutWait = 0;
    cpuEvents |= EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OUT;
}

#endif
