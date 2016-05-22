#ifdef DEBUG_SUPPORT

#include "debug.h"
#include "stepping.h"
#include "disasm.h"
#include "../mem.h"
#include "../emu.h"
#include "../asic.h"

void debug_set_step_next(void) {
    debug_clear_step_over();
    disasm.base_address = cpu.registers.PC;
    //fprintf(stderr, "[setDebugStepNextMode] disasm.base_address=0x%08x\n", disasm.base_address);
    disasm.adl = cpu.ADL;
    disassembleInstruction();
    debugger.stepOverFirstStep = true;
    debugger.stepOverCall = false;
    debugger.stepOverInstrEnd = disasm.new_address;
    debugger.data.block[debugger.stepOverInstrEnd] |= DBG_STEP_OVER_BREAKPOINT;
    debugger.stepOverMode = cpu.ADL;
    debugger.stepOutSPL = 0;
    debugger.stepOutSPS = 0;
    debugger.stepOutWait = -1;
    cpuEvents |= EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_NEXT;
}

void debug_set_step_in(void) {
    debug_clear_step_over();
    debugger.stepOverFirstStep = false;
    //fprintf(stderr, "[setDebugStepInMode] stepOverFirstStep=false\n");
    cpuEvents |= EVENT_DEBUG_STEP;
}

void debug_set_step_over(void) {
    debug_clear_step_over();
    disasm.base_address = cpu.registers.PC;
    //fprintf(stderr, "[setDebugStepNextMode] disasm.base_address=0x%08x\n", disasm.base_address);
    disasm.adl = cpu.ADL;
    disassembleInstruction();
    debugger.stepOverInstrEnd = disasm.new_address;
    debugger.data.block[debugger.stepOverInstrEnd] |= DBG_STEP_OVER_BREAKPOINT;
    debugger.stepOverMode = cpu.ADL;
    debugger.stepOverFirstStep = false;
    debugger.stepOverCall = true;
    //fprintf(stderr, "[setDebugStepOverMode] stepOverFirstStep=false\n");
    cpuEvents |= EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OVER;
}

void debug_set_step_out(void) {
    debug_clear_step_over();
    debugger.stepOverFirstStep = true;
    debugger.stepOutSPL = cpu.registers.SPL + 1;
    debugger.stepOutSPS = cpu.registers.SPS + 1;
    debugger.stepOutWait = 0;
    //fprintf(stderr, "[setDebugStepOutMode] stepOverFirstStep=true\n");
    //fprintf(stderr, "[setDebugStepOutMode] stepOverInstrEnd=0x%08x\n", debugger.stepOverInstrEnd);
    //fprintf(stderr, "[setDebugStepOutMode] stepOutSPL=0x%08x\n", debugger.stepOutSPL);
    //fprintf(stderr, "[setDebugStepOutMode] stepOutSPS=0x%08x\n", debugger.stepOutSPS);
    //fprintf(stderr, "[setDebugStepOutMode] stepOutWait=%i\n", debugger.stepOutWait);
    cpuEvents |= EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OUT;
}

#endif
