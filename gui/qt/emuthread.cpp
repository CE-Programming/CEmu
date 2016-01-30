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

#include "capture/gif.h"
#include "../../core/emu.h"
#include "../../core/lcd.h"
#include "../../core/link.h"
#include "../../core/debug/debug.h"
#include "../../core/debug/disasm.h"

EmuThread *emu_thread = nullptr;

void gui_emu_sleep(void) {
    QThread::usleep(50);
}

void gui_do_stuff(void) {
    emu_thread->doStuff();
}

static void gui_console_vprintf(const char *fmt, va_list ap) {
    QString str;
    str.vsprintf(fmt, ap);
    emu_thread->consoleStr(str);
}

void gui_console_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    gui_console_vprintf(fmt, ap);

    va_end(ap);
}

void gui_debugger_send_command(int reason, uint32_t addr) {
    emu_thread->sendDebugCommand(reason, addr);
}

void gui_debugger_entered_or_left(bool entered) {
    if (entered == true) {
        emu_thread->debuggerEntered();
    }
}

void throttle_timer_wait(void) {
    emu_thread->throttleTimerWait();
}

EmuThread::EmuThread(QObject *p) : QThread(p) {
    assert(emu_thread == nullptr);
    emu_thread = this;
    lcd_event_gui_callback = gif_new_frame;
    speed = actualSpeed = 100;
    lastTime= std::chrono::steady_clock::now();
}

void EmuThread::changeEmuSpeed(int value) {
    speed = value;
}

void EmuThread::changeThrottleMode(bool mode) {
    throttleOn = mode;
}

void EmuThread::setDebugMode(bool state) {
    enterDebugger = state;
    if(inDebugger && !state) {
        inDebugger = false;
    }
}

void EmuThread::setSendState(bool state) {
    enterSendState = state;
    emu_is_sending = state;
}

void EmuThread::setReceiveState(bool state) {
    enterReceiveState = state;
    emu_is_recieving = state;
}

void EmuThread::setDebugStepMode() {
    cpu_events |= EVENT_DEBUG_STEP;
    enterDebugger = false;
    inDebugger = false;
}

void EmuThread::setDebugStepOverMode() {
    disasm.base_address = cpu.registers.PC;
    disasm.adl = cpu.ADL;
    disassembleInstruction();
    debugger.stepOverAddress = disasm.new_address;
    debugger.data.block[debugger.stepOverAddress] |= DBG_STEP_OVER_BREAKPOINT;
    cpu_events |= EVENT_DEBUG_STEP_OVER;
    enterDebugger = false;
    inDebugger = false;
}

void EmuThread::setDebugStepOutMode() {
    debugger.stepOutSPL = cpu.registers.SPL;
    debugger.stepOutSPS = cpu.registers.SPS;
    cpu_events |= EVENT_DEBUG_STEP_OUT;
    enterDebugger = false;
    inDebugger = false;
}

//Called occasionally, only way to do something in the same thread the emulator runs in.
void EmuThread::doStuff() {
    std::chrono::steady_clock::time_point cur_time = std::chrono::steady_clock::now();

    if (enterSendState || enterReceiveState) {
        enterReceiveState = enterSendState = false;
        enterVariableLink();
    }

    if (enterDebugger) {
        enterDebugger = false;
        open_debugger(DBG_USER, 0);
    }

    lastTime += std::chrono::steady_clock::now() - cur_time;
}

void EmuThread::setActualSpeed(int value) {
    if(!calc_is_off()) {
        if (actualSpeed != value) {
            emit actualSpeedChanged(actualSpeed = value);
        }
    }
}

void EmuThread::throttleTimerWait() {
    std::chrono::duration<int, std::ratio<100, 60>> unit(1);
    std::chrono::steady_clock::duration interval(std::chrono::duration_cast<std::chrono::steady_clock::duration>
                                                 (std::chrono::duration<int, std::ratio<1, 60 * 1000000>>(1000000 * 100 / speed)));
    std::chrono::steady_clock::time_point cur_time = std::chrono::steady_clock::now(), next_time = lastTime + interval;
    if (throttleOn && cur_time < next_time) {
        setActualSpeed(speed);
        lastTime = next_time;
        std::this_thread::sleep_until(next_time);
    } else {
        setActualSpeed(unit / (cur_time - lastTime));
        lastTime = cur_time;
        std::this_thread::yield();
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

    inDebugger = false;
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
