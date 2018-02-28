#include "emuthread.h"

#include <cassert>
#include <cstdarg>
#include <thread>

#include "mainwindow.h"

#include "capture/animated-png.h"
#include "../../core/emu.h"
#include "../../core/cpu.h"
#include "../../core/control.h"
#include "../../core/link.h"

EmuThread *emu_thread = Q_NULLPTR;

void gui_emu_sleep(unsigned long microseconds) {
    QThread::usleep(microseconds);
}

void gui_do_stuff(void) {
    emu_thread->doStuff();
}

void gui_console_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    emu_thread->writeConsoleBuffer(CONSOLE_NORM, format, args);
    va_end(args);
}

void gui_console_err_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    emu_thread->writeConsoleBuffer(CONSOLE_ERR, format, args);
    va_end(args);
}

void gui_debugger_send_command(int reason, uint32_t addr) {
    emu_thread->sendDebugCommand(reason, addr);
}

void gui_debugger_raise_or_disable(bool entered) {
    if (entered) {
        emu_thread->raiseDebugger();
    } else {
        emu_thread->disableDebugger();
    }
}

void throttle_timer_wait(void) {
    emu_thread->throttleTimerWait();
}

EmuThread::EmuThread(QObject *p) : QThread(p) {
    assert(emu_thread == Q_NULLPTR);
    emu_thread = this;
    for (int i = 0; i < CONSOLE_MAX; i++) {
        consoleWriteSemaphore[i].release(CONSOLE_BUFFER_SIZE);
    }
    speed = 100;
    throttle = true;
    request = REQUEST_NONE;
    lastTime = std::chrono::steady_clock::now();
}

void EmuThread::writeConsoleBuffer(int type, const char *format, va_list args) {
    va_list argsCopy;
    va_copy(argsCopy, args);
    int available = consoleWriteSemaphore[type].available();
    int remaining = CONSOLE_BUFFER_SIZE - consoleWritePosition[type];
    int space = available < remaining ? available : remaining;
    int size = vsnprintf(consoleBuffer[type] + consoleWritePosition[type], space, format, argsCopy);
    va_end(argsCopy);
    if (size < space) {
        if (size > 0) {
            consoleWriteSemaphore[type].acquire(size);
            consoleWritePosition[type] += size;
            consoleReadSemaphore[type].release(size);
            emit consoleStr(type);
        }
    } else {
        int bufferPosition = 0;
        char *buffer = size < available - remaining ? consoleBuffer[type] : new char[size + 1];
        if (buffer && vsnprintf(buffer, size + 1, format, args) >= 0) {
            while (size >= remaining) {
                consoleWriteSemaphore[type].acquire(remaining);
                memcpy(consoleBuffer[type] + consoleWritePosition[type], buffer + bufferPosition, remaining);
                bufferPosition += remaining;
                size -= remaining;
                consoleWritePosition[type] = 0;
                consoleReadSemaphore[type].release(remaining);
                remaining = CONSOLE_BUFFER_SIZE;
                emit consoleStr(type);
            }
            if (size) {
                consoleWriteSemaphore[type].acquire(size);
                memmove(consoleBuffer[type] + consoleWritePosition[type], buffer + bufferPosition, size);
                consoleWritePosition[type] += size;
                consoleReadSemaphore[type].release(size);
                emit consoleStr(type);
            }
        }
        if (buffer != consoleBuffer[type]) {
            delete [] buffer;
        }
    }
}

void EmuThread::unlock() {
    mutex.lock();
    cv.notify_all();
    mutex.unlock();
}

void EmuThread::block() {
    std::unique_lock<std::mutex> lock(mutex);
    emit locked(request);
    cv.wait(lock);
}

// Called occasionally, only way to do something in the same thread the emulator runs in.
void EmuThread::doStuff() {
    const std::chrono::steady_clock::time_point cur_time = std::chrono::steady_clock::now();

    if (request != REQUEST_NONE) {
        switch (request) {
            default:
                break;
            case REQUEST_PAUSE:
                block();
                break;
            case REQUEST_RESET:
                cpu.events |= EVENT_RESET;
                break;
            case REQUEST_SEND:
                sendFiles();
                break;
            case REQUEST_RECEIVE:
                block();
                break;
            case REQUEST_DEBUGGER:
                open_debugger(DBG_USER, 0);
                break;
        }

        if (request == REQUEST_SAVE) {
            emit saved(emu_save(saveImage, savePath.toStdString().c_str()));
        }

        request = REQUEST_NONE;
    }

    lastTime += std::chrono::steady_clock::now() - cur_time;
}

void EmuThread::sendFiles() {
    const int fileNum = vars.size();

    for (int i = 0; i < fileNum; i++) {
        const QString &f = vars.at(i);
        emit sentFile(f, sendVariableLink(f.toUtf8(), sendLoc));
    }

    emit sentFile(QString(), LINK_GOOD);
}

void EmuThread::setActualSpeed(int value) {
    if (!control.off) {
        if (actualSpeed != value) {
            actualSpeed = value;
            emit actualSpeedChanged(value);
        }
    }
}

void EmuThread::throttleTimerWait() {
    if (!speed) {
        setActualSpeed(0);
        while(!speed) {
            QThread::msleep(10);
        }
        return;
    }
    std::chrono::duration<int, std::ratio<100, 60>> unit(1);
    std::chrono::steady_clock::duration interval(std::chrono::duration_cast<std::chrono::steady_clock::duration>
                                                (std::chrono::duration<int, std::ratio<1, 60 * 1000000>>(1000000 * 100 / speed)));
    std::chrono::steady_clock::time_point cur_time = std::chrono::steady_clock::now(), next_time = lastTime + interval;
    if (throttle && cur_time < next_time) {
        setActualSpeed(speed);
        lastTime = next_time;
        std::this_thread::sleep_until(next_time);
    } else {
        if (lastTime != cur_time) {
            setActualSpeed(unit / (cur_time - lastTime));
            lastTime = cur_time;
        }
        std::this_thread::yield();
    }
}

void EmuThread::req(int req) {
    request = req;
}

void EmuThread::send(const QStringList &list, unsigned int location) {
    vars = list;
    sendLoc = location;
    req(REQUEST_SEND);
}

void EmuThread::save(bool image, const QString &path) {
    savePath = path;
    saveImage = image;
    req(REQUEST_SAVE);
}

void EmuThread::setEmuSpeed(int value) {
    speed = value;
}

void EmuThread::setThrottleMode(bool value) {
    throttle = value;
}

void EmuThread::debug(bool state) {
    if (inDebugger && !state) {
        debug_clear_temp_break();
        close_debugger();
    }
    if (state) {
        req(REQUEST_DEBUGGER);
    }
}

void EmuThread::run() {
    emu_loop();
}

int EmuThread::load(bool restore, const QString &rom, const QString &image) {
    int ret = EMU_LOAD_FAIL;

    setTerminationEnabled();
    stop();

    if (restore) {
        ret = emu_load(true, image.toStdString().c_str());
    } else {
        ret = emu_load(false, rom.toStdString().c_str());
    }
    return ret;
}

// call from gui thread
void EmuThread::stop() {
    if (!isRunning()) {
        return;
    }
    exiting = true;
    if (!wait(200)) {
        terminate();
        wait(300);
    }
}
