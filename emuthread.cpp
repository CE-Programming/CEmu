#include "emuthread.h"

#include <cassert>
#include <iostream>

#include <QEventLoop>
#include <QTimer>

#include "mainwindow.h"

#include "core/emu.h"

EmuThread *emu_thread = nullptr;

void gui_do_stuff()
{
    emu_thread->doStuff();
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

EmuThread::EmuThread(QObject *p) : QThread(p)
{
    assert(emu_thread == nullptr);
    emu_thread = this;
}

//Called occasionally, only way to do something in the same thread the emulator runs in.
void EmuThread::doStuff()
{
    while(paused)
        msleep(100);
}

void EmuThread::run()
{
    rom_image = rom.c_str();

    int ret = emulate();

    emit exited(ret);
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
