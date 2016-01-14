/* Copyright (C) 2015  Fabian Vogt
 * Modified for the CE calculator by CEmu developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include "emuthread.h"

#include <cassert>
#include <iostream>
#include <cstdarg>
#include <thread>

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>

#include "mainwindow.h"

#include "../../core/emu.h"
#include "../../core/link.h"
#include "../../core/debug/debug.h"
#include "../../core/debug/disasm.h"

EmuThread *emu_thread = nullptr;

void gui_emu_yield(void) {
    QThread::yieldCurrentThread();
}

void gui_emu_sleep(void) {
    QThread::usleep(50);
}

void gui_do_stuff(bool wait) {
    emu_thread->doStuff(wait);
}

void gui_console_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    gui_console_vprintf(fmt, ap);

    va_end(ap);
}

void gui_console_vprintf(const char *fmt, va_list ap) {
    QString str;
    str.vsprintf(fmt, ap);
    emu_thread->consoleStr(str);
}

void gui_perror(const char *msg) {
    gui_console_printf("%s: %s\n", msg, strerror(errno));
}

void gui_debugger_send_command(int reason, uint32_t addr) {
    emu_thread->sendDebugCommand(reason, addr);
}

void throttle_timer_wait() {
    emu_thread->throttleTimerWait();
}

void gui_debugger_entered_or_left(bool entered) {
    if (entered == true) {
        emu_thread->debuggerEntered();
    }
}

EmuThread::EmuThread(QObject *p) : QThread(p) {
    assert(emu_thread == nullptr);
    emu_thread = this;
    speed = 100;
    last_time = std::chrono::steady_clock::now();
}

void EmuThread::changeEmuSpeed(int value) {
    speed = value;
}

void EmuThread::setDebugMode(bool state) {
    enter_debugger = state;
    if(in_debugger && !state) {
        in_debugger = false;
    }
}

void EmuThread::setSendState(bool state) {
    enter_send_state = state;
    emu_is_sending = state;
}

void EmuThread::setReceiveState(bool state) {
    enter_receive_state = state;
    emu_is_recieving = state;
}

void EmuThread::setDebugStepMode() {
    cpu_events |= EVENT_DEBUG_STEP;
    enter_debugger = false;
    in_debugger = false;
}

void EmuThread::setDebugStepOverMode() {
    disasm.base_address = cpu.registers.PC;
    disasm.adl = cpu.ADL;
    disassembleInstruction();
    mem.debug.stepOverAddress = disasm.new_address;
    mem.debug.block[mem.debug.stepOverAddress] |= DBG_STEP_OVER_BREAKPOINT;
    cpu_events |= EVENT_DEBUG_STEP_OVER;
    enter_debugger = false;
    in_debugger = false;
}

void EmuThread::setDebugStepOutMode() {
    mem.debug.stepOutSPL = cpu.registers.SPL;
    mem.debug.stepOutSPS = cpu.registers.SPS;
    cpu_events |= EVENT_DEBUG_STEP_OUT;
    enter_debugger = false;
    in_debugger = false;
}

//Called occasionally, only way to do something in the same thread the emulator runs in.
void EmuThread::doStuff(bool wait_for) {
    std::chrono::steady_clock::time_point cur_time = std::chrono::steady_clock::now();

    (void)wait_for;

    if (enter_send_state) {
        enter_send_state = false;
        enterVariableLink();
    }

    if (enter_receive_state) {
        enter_receive_state = false;
        enterVariableLink();
    }

    if (enter_debugger) {
        enter_debugger = false;
        debugger(DBG_USER, 0);
    }

    last_time += std::chrono::steady_clock::now() - cur_time;
}

void EmuThread::throttleTimerWait() {
    std::chrono::steady_clock::duration interval(std::chrono::duration_cast<std::chrono::steady_clock::duration>
                                                 (std::chrono::duration<int, std::ratio<1, 60 * 1000000>>(1000000 * 100 / speed)));
    std::chrono::steady_clock::time_point cur_time = std::chrono::steady_clock::now(), next_time = last_time + interval;
    if (cur_time < next_time) {
        std::this_thread::sleep_until(last_time = next_time);
    } else {
        last_time = cur_time;
    }
}

void EmuThread::run() {
    rom_image = rom.c_str();

    bool reset_true = true;
    bool success = emu_start();

    if (success) {
        emu_loop(reset_true);
        emit exited(0);
    } else {
        emit exited(-1);
    }
}

bool EmuThread::stop() {

    if(!isRunning())
        return true;

    in_debugger = false;
    emu_is_sending = false;

    /* Cause the CPU core to leave the loop and check for events */
    exiting = true; // exit outer loop
    cpu.next = 0; // exit inner loop

    if(!this->wait(200))
    {
        terminate();
        if(!this->wait(200))
        {
            return false;
        }
    }

    return true;
}
