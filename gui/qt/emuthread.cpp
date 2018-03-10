#include "emuthread.h"
#include "capture/animated-png.h"
#include "../../core/emu.h"
#include "../../core/cpu.h"
#include "../../core/control.h"
#include "../../core/link.h"

#include <cassert>
#include <cstdarg>
#include <thread>

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
    emu_thread->writeConsoleBuffer(EmuThread::ConsoleNorm, format, args);
    va_end(args);
}

void gui_console_err_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    emu_thread->writeConsoleBuffer(EmuThread::ConsoleErr, format, args);
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

EmuThread::EmuThread(QObject *parent) : QThread{parent}, write(CONSOLE_BUFFER_SIZE) {
    assert(emu_thread == Q_NULLPTR);
    emu_thread = this;
    m_speed = 100;
    m_throttle = true;
    m_request = RequestNone;
    m_lastTime = std::chrono::steady_clock::now();
}

void EmuThread::writeConsoleBuffer(int type, const char *format, va_list args) {
    static int prevType = ConsoleNorm;
    if (prevType != type) {
        write.acquire(CONSOLE_BUFFER_SIZE);
        prevType = type;
        write.release(CONSOLE_BUFFER_SIZE);
    }
    int available = write.available();
    int remaining = CONSOLE_BUFFER_SIZE - writePos;
    int space = available < remaining ? available : remaining;
    int size;
    va_list argsCopy;
    va_copy(argsCopy, args);
    size = vsnprintf(buffer + writePos, space, format, argsCopy);
    va_end(argsCopy);
    if (size >= 0 && size < space) {
        if (size == 0) {
            return;
        }
        write.acquire(size);
        writePos += size;
        read.release(size);
        emit consoleStr(type);
    } else {
        if (size < 0) {
            va_copy(argsCopy, args);
            size = vsnprintf(nullptr, 0, format, argsCopy);
            va_end(argsCopy);
            if (size <= 0) {
                return;
            }
        }
        char *tmp = size < available - remaining ? buffer : new char[size + 1];
        if (tmp && vsnprintf(tmp, size + 1, format, args) >= 0) {
            int tmpPos = 0;
            while (size >= remaining) {
                write.acquire(remaining);
                memcpy(buffer + writePos, tmp + tmpPos, remaining);
                tmpPos += remaining;
                size -= remaining;
                writePos = 0;
                read.release(remaining);
                remaining = CONSOLE_BUFFER_SIZE;
                emit consoleStr(type);
            }
            if (size) {
                write.acquire(size);
                memmove(buffer + writePos, tmp + tmpPos, size);
                writePos += size;
                read.release(size);
                emit consoleStr(type);
            }
        }
        if (tmp != buffer) {
            delete [] tmp;
        }
    }
}

void EmuThread::unlock() {
    m_mutex.lock();
    m_cv.notify_all();
    m_mutex.unlock();
}

void EmuThread::block() {
    std::unique_lock<std::mutex> lock(m_mutex);
    emit locked(m_request);
    m_cv.wait(lock);
}

// Called occasionally, only way to do something in the same thread the emulator runs in.
void EmuThread::doStuff() {
    const std::chrono::steady_clock::time_point cur_time = std::chrono::steady_clock::now();

    if (m_request != RequestNone) {
        switch (+m_request) {
            default:
                break;
            case RequestPause:
                block();
                break;
            case RequestReset:
                cpu.events |= EVENT_RESET;
                break;
            case RequestSend:
                sendFiles();
                break;
            case RequestReceive:
                block();
                break;
            case RequestDebugger:
                open_debugger(DBG_USER, 0);
                break;
        }

        if (m_request == RequestSave) {
            emit saved(emu_save(m_saveImage, m_savePath.toStdString().c_str()));
        }

        m_request = RequestNone;
    }

    m_lastTime += std::chrono::steady_clock::now() - cur_time;
}

void EmuThread::sendFiles() {
    const int fileNum = m_vars.size();

    for (int i = 0; i < fileNum; i++) {
        const QString &f = m_vars.at(i);
        emit sentFile(f, sendVariableLink(f.toUtf8(), m_sendLoc));
    }

    emit sentFile(QString(), LINK_GOOD);
}

void EmuThread::setActualSpeed(int value) {
    if (!control.off) {
        if (m_actualSpeed != value) {
            m_actualSpeed = value;
            emit actualSpeedChanged(value);
        }
    }
}

void EmuThread::throttleTimerWait() {
    if (!m_speed) {
        setActualSpeed(0);
        while(!m_speed) {
            QThread::msleep(10);
        }
        return;
    }
    std::chrono::duration<int, std::ratio<100, 60>> unit(1);
    std::chrono::steady_clock::duration interval(std::chrono::duration_cast<std::chrono::steady_clock::duration>
                                                (std::chrono::duration<int, std::ratio<1, 60 * 1000000>>(1000000 * 100 / m_speed)));
    std::chrono::steady_clock::time_point cur_time = std::chrono::steady_clock::now(), next_time = m_lastTime + interval;
    if (m_throttle && cur_time < next_time) {
        setActualSpeed(m_speed);
        m_lastTime = next_time;
        std::this_thread::sleep_until(next_time);
    } else {
        if (m_lastTime != cur_time) {
            setActualSpeed(unit / (cur_time - m_lastTime));
            m_lastTime = cur_time;
        }
        std::this_thread::yield();
    }
}

void EmuThread::req(int req) {
    m_request = req;
}

void EmuThread::reset() {
    req(RequestReset);
}

void EmuThread::receive() {
    req(RequestReceive);
}

void EmuThread::send(const QStringList &list, unsigned int location) {
    m_vars = list;
    m_sendLoc = location;
    req(RequestSend);
}

void EmuThread::save(bool image, const QString &path) {
    m_savePath = path;
    m_saveImage = image;
    req(RequestSave);
}

void EmuThread::setEmuSpeed(int value) {
    m_speed = value;
}

void EmuThread::setThrottleMode(bool value) {
    m_throttle = value;
}

void EmuThread::debug(bool state) {
    if (inDebugger && !state) {
        debug_clear_temp_break();
        close_debugger();
    }
    if (state) {
        req(RequestDebugger);
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
