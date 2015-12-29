#include "emuthread.h"

#include <cassert>
#include <iostream>
#include <cstdarg>
#include <chrono>

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>

#include "mainwindow.h"

#include "core/emu.h"
#include "core/debug.h"

EmuThread *emu_thread = nullptr;

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
}

void EmuThread::setDebugMode(bool state) {
    enter_debugger = state;
    if(in_debugger && !state) {
        in_debugger = false;
    }
}

//Called occasionally, only way to do something in the same thread the emulator runs in.
void EmuThread::doStuff(bool wait_for) {
    (void)wait_for;

    if (enter_debugger) {
        enter_debugger = false;
        debugger(DBG_USER, 0);
    }
}

void EmuThread::throttleTimerWait() {
    unsigned int now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    unsigned int throttle = throttle_delay * 1000;
    unsigned int left = throttle - (now % throttle);
    if (left > 0)
        QThread::usleep(left);
}

void EmuThread::run() {
    rom_image = rom.c_str();

    bool reset_true = true;
    bool success = emu_start();

    if(success) { emu_loop(reset_true); }

    emit exited(0);
}

bool EmuThread::stop() {

    if(!isRunning())
        return true;

    exiting = true;
    in_debugger = false;

    /* Cause the cpu core to leave the loop and check for events */
    cycle_count_delta = 0;

    if(!this->wait(200))
    {
        terminate();
        if(!this->wait(200))
        {
            return false;
        }
    }

    emu_cleanup();
    return true;
}
