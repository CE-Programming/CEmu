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

void gui_do_stuff(bool wait)
{
    emu_thread->doStuff(wait);
}

void gui_console_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    gui_console_vprintf(fmt, ap);

    va_end(ap);
}

void gui_console_vprintf(const char *fmt, va_list ap)
{
    QString str;
    str.vsprintf(fmt, ap);
    emu_thread->consoleStr(str);
}

void gui_perror(const char *msg)
{
    gui_console_printf("%s: %s\n", msg, strerror(errno));
}

void throttle_timer_off()
{
    emu_thread->setTurboMode(true);
}

void throttle_timer_on()
{
    emu_thread->setTurboMode(false);
}

void throttle_timer_wait()
{
    emu_thread->throttleTimerWait();
}

void gui_debugger_entered()
{
    emu_thread->debuggerEntered();
}

EmuThread::EmuThread(QObject *p) : QThread(p)
{
    assert(emu_thread == nullptr);
    emu_thread = this;
}

void EmuThread::enterDebugger()
{
    enter_debugger = true;
}

//Called occasionally, only way to do something in the same thread the emulator runs in.
void EmuThread::doStuff(bool waitfor)
{
    do
    {
        if(enter_debugger)
        {
            enter_debugger = false;
            debugger(DBG_USER, 0);
        }

        if(/*is_paused && */0)
            msleep(100);

    } while(/*is_paused && */0);
}

void EmuThread::throttleTimerWait()
{
    unsigned int now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    unsigned int throttle = throttle_delay * 1000;
    unsigned int left = throttle - (now % throttle);
    if( left > 0 )
        QThread::usleep(left);
}

void EmuThread::setTurboMode(bool enabled)
{
    turbo_mode = enabled;
    emit turboModeChanged(enabled);
}

void EmuThread::toggleTurbo()
{
    setTurboMode(!turbo_mode);
}


void EmuThread::run()
{
    rom_image = rom.c_str();

    bool reset_true = true;
    bool success = emu_start();

    asic_ptr = &asic;

    if(success) { emu_loop(reset_true); }

    emit exited(0);
}

void EmuThread::setPaused(bool pause)
{
    this->paused = pause;
}

bool EmuThread::stop()
{
    exiting = true;
    paused = false;
    if(!this->wait(200))
    {
        terminate();
        if(!this->wait(200))
            return false;
    }

    emu_cleanup();
    return true;
}

void EmuThread::test()
{
    gui_console_printf("Calculator Reset Triggered...\n");
}
