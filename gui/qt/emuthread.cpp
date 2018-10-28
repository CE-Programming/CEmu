#include "emuthread.h"
#include "capture/animated-png.h"
#include "../../core/emu.h"
#include "../../core/cpu.h"
#include "../../core/control.h"
#include "../../core/link.h"

#include <cassert>
#include <cstdarg>
#include <thread>

static EmuThread *emu;

// reimplemented callbacks

void gui_do_stuff(void) {
    emu->doStuff();
}

void gui_console_clear(void) {
    emu->consoleClear();
}

void gui_console_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    emu->writeConsole(EmuThread::ConsoleNorm, format, args);
    va_end(args);
}

void gui_console_err_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    emu->writeConsole(EmuThread::ConsoleErr, format, args);
    va_end(args);
}

void gui_debug_open(int reason, uint32_t data) {
    emu->debugOpen(reason, data);
}

void gui_debug_close(void) {
    emu->debugDisable();
}

void gui_throttle(void) {
    emu->throttleWait();
}

EmuThread::EmuThread(QObject *parent) : QThread{parent}, write{CONSOLE_BUFFER_SIZE},
                                        m_speed{100}, m_throttle{true},
                                        m_lastTime{std::chrono::steady_clock::now()},
                                        m_request{RequestNone}, m_debug{false} {
    assert(emu == Q_NULLPTR);
    emu = this;
}

void EmuThread::run() {
    emu_loop();
}

void EmuThread::writeConsole(int console, const char *format, va_list args) {
    if (type != console) {
        write.acquire(CONSOLE_BUFFER_SIZE);
        type = console;
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
        emit consoleStr();
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
                emit consoleStr();
            }
            if (size) {
                write.acquire(size);
                memmove(buffer + writePos, tmp + tmpPos, size);
                writePos += size;
                read.release(size);
                emit consoleStr();
            }
        }
        if (tmp != buffer) {
            delete [] tmp;
        }
    }
}

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
                cpu_crash("user request");
                break;
            case RequestSend:
                sendFiles();
                break;
            case RequestReceive:
                block();
                break;
            case RequestDebugger:
                debug_open(DBG_USER, 0);
                break;
        }

        if (m_request == RequestSave) {
            emit saved(emu_save(m_saveType, m_savePath.toStdString().c_str()));
        }

        if (m_request == RequestLoad) {
            emit loaded(emu_load(m_loadType, m_loadPath.toStdString().c_str()), m_loadType);
        }

        m_request = RequestNone;
    }

    m_lastTime += std::chrono::steady_clock::now() - cur_time;
}

void EmuThread::throttleWait() {
    int speed;
    bool throttle;
    {
        std::unique_lock<std::mutex> lockSpeed(m_mutexSpeed);
        speed = m_speed;
        if (!speed) {
            sendSpeed(0);
            m_cvSpeed.wait(lockSpeed, [this] { return m_speed != 0; });
            speed = m_speed;
            m_lastTime = std::chrono::steady_clock::now();
        }
        throttle = m_throttle;
    }
    std::chrono::duration<int, std::ratio<100, 60>> unit(1);
    std::chrono::steady_clock::duration interval(std::chrono::duration_cast<std::chrono::steady_clock::duration>
                                                (std::chrono::duration<int, std::ratio<1, 60 * 1000000>>(1000000 * 100 / speed)));
    std::chrono::steady_clock::time_point cur_time = std::chrono::steady_clock::now(), next_time = m_lastTime + interval;
    if (throttle && cur_time < next_time) {
        sendSpeed(speed);
        m_lastTime = next_time;
        std::this_thread::sleep_until(next_time);
    } else {
        if (m_lastTime != cur_time) {
            sendSpeed(unit / (cur_time - m_lastTime));
            m_lastTime = cur_time;
        }
        std::this_thread::yield();
    }
}

void EmuThread::unblock() {
    m_mutex.lock();
    m_cv.notify_all();
    m_mutex.unlock();
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

void EmuThread::save(emu_data_t type, const QString &path) {
    m_savePath = path;
    m_saveType = type;
    req(RequestSave);
}

void EmuThread::setSpeed(int value) {
    {
        std::unique_lock<std::mutex> lockSpeed(m_mutexSpeed);
        m_speed = value;
    }
    if (value) {
        m_cvSpeed.notify_one();
    }
}

void EmuThread::setThrottle(bool state) {
    std::unique_lock<std::mutex> lockSpeed(m_mutexSpeed);
    m_throttle = state;
}

void EmuThread::debugOpen(int reason, uint32_t data) {
    std::unique_lock<std::mutex> lock(m_mutexDebug);
    m_debug = true;
    emit debugCommand(reason, data);
    m_cvDebug.wait(lock, [this](){ return !m_debug; });
}

void EmuThread::resume() {
    {
        std::lock_guard<std::mutex> lock(m_mutexDebug);
        m_debug = false;
    }
    m_cvDebug.notify_all();
}

void EmuThread::debug(bool state) {
    bool oldState;
    {
        std::lock_guard<std::mutex> lock(m_mutexDebug);
        oldState = m_debug;
    }
    if (oldState && !state) {
        resume();
    }
    if (state) {
        req(RequestDebugger);
    }
}

void EmuThread::load(emu_data_t type, const QString &path) {

    /* if loading an image or rom, we need to restart emulation */
    if (type == EMU_DATA_IMAGE || type == EMU_DATA_ROM) {
        setTerminationEnabled();
        stop();

        emit loaded(emu_load(type, path.toStdString().c_str()), type);
    } else if (type == EMU_DATA_RAM) {
        m_loadPath = path;
        m_loadType = type;
        req(RequestLoad);
    }
}

void EmuThread::stop() {
    if (!isRunning()) {
        return;
    }
    emu_exit();
    if (!wait(200)) {
        terminate();
        wait(300);
    }
}
