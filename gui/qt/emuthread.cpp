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
QTimer speedUpdateTimer;

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

static void gui_console_err_vprintf(const char *fmt, va_list ap) {
    QString str;
    str.vsprintf(fmt, ap);
    emu_thread->errConsoleStr(str);
}

void gui_console_err_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    gui_console_err_vprintf(fmt, ap);

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

void gui_entered_send_state(bool entered) {
    if(entered) {
        emu_thread->waitForLink = false;
    }
}

EmuThread::EmuThread(QObject *p) : QThread(p) {
    assert(emu_thread == nullptr);
    emu_thread = this;
    lcd_event_gui_callback = gif_new_frame;
    speed = actualSpeed = 100;
    updateTimer.start();
    lastTime= updateTimer.elapsed();
    connect(&speedUpdateTimer, SIGNAL(timeout()), this, SLOT(sendActualSpeed()));
}

void EmuThread::resetTriggered() {
    cpuEvents |= EVENT_RESET;
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

void EmuThread::setDebugStepInMode() {
    cpuEvents |= EVENT_DEBUG_STEP;
    enterDebugger = false;
    inDebugger = false;
}

void EmuThread::setDebugStepOverMode() {
    debugger.stepOutSPL = cpu.registers.SPL;
    debugger.stepOutSPS = cpu.registers.SPS;
    cpuEvents |= EVENT_DEBUG_STEP_OUT;
    enterDebugger = false;
    inDebugger = false;
}

void EmuThread::setDebugStepNextMode() {
    disasm.base_address = cpu.registers.PC;
    disasm.adl = cpu.ADL;
    disassembleInstruction();
    debugger.stepOverAddress = disasm.new_address;
    debugger.data.block[debugger.stepOverAddress] |= DBG_STEP_OVER_BREAKPOINT;
    cpuEvents |= EVENT_DEBUG_STEP_OVER;
    enterDebugger = false;
    inDebugger = false;
}

void EmuThread::setDebugStepOutMode() {
    debugger.stepOutSPL = cpu.registers.SPL + 1;
    debugger.stepOutSPS = cpu.registers.SPS + 1;
    cpuEvents |= EVENT_DEBUG_STEP_OUT;
    enterDebugger = false;
    inDebugger = false;
}

void gui_set_busy(bool busy) {
    emit emu_thread->isBusy(busy);
}

// Called occasionally, only way to do something in the same thread the emulator runs in.
void EmuThread::doStuff() {
    qint64 cur_time = updateTimer.elapsed();

    if (saveImage) {
        bool success = emu_save(imagePath.c_str());
        saveImage = false;
        emit saved(success);
    }

    if (saveRom) {
        bool success = emu_save_rom(exportRomPath.c_str());
        saveRom = false;
        emit saved(success);
    }

    if (enterSendState || enterReceiveState) {
        enterReceiveState = enterSendState = false;
        enterVariableLink();
    }

    if (enterDebugger) {
        enterDebugger = false;
        open_debugger(DBG_USER, 0);
    }

    lastTime += updateTimer.elapsed() - cur_time;
}

void EmuThread::sendActualSpeed() {
    if(!calc_is_off()) {
        emit actualSpeedChanged(actualSpeed);
    }
}

void EmuThread::setActualSpeed(int value) {
    if(!calc_is_off()) {
        if (actualSpeed != value) {
            actualSpeed = value;
        }
    }
}

void EmuThread::throttleTimerWait() {
    qint64 interval = 100000 / (60 * speed);
    qint64 cur_time = updateTimer.elapsed(), next_time = lastTime + interval;
    if (throttleOn && cur_time < next_time) {
        setActualSpeed(speed);
        lastTime = next_time;
        QThread::msleep(interval);
    } else {
        if ((cur_time - lastTime) == 0) cur_time++;
        setActualSpeed(100 * (interval / (cur_time - lastTime)));
        lastTime = cur_time;
        QThread::yieldCurrentThread();
    }
}

void EmuThread::run() {
    setTerminationEnabled();

    bool doReset = !doRestore;
    bool success = emu_start(rom.c_str(), doRestore ? imagePath.c_str() : nullptr);

    if(doRestore) {
        emit restored(success);
    } else {
        emit started(success);
    }

    doRestore = false;

    if(success) {
        emu_loop(doReset);
    }
    emit stopped();
}

bool EmuThread::stop() {

    if(!isRunning()) {
        return true;
    }

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

bool EmuThread::restore(QString path) {
    imagePath = QDir::toNativeSeparators(path).toStdString();
    doRestore = true;
    if(!stop()) {
        return false;
    }

    start();
    return true;
}

void EmuThread::save(QString path) {
    imagePath = QDir::toNativeSeparators(path).toStdString();
    saveImage = true;
}

void EmuThread::saveRomImage(QString path) {
    exportRomPath = QDir::toNativeSeparators(path).toStdString();
    saveRom = true;
}
