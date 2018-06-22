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
                                        m_request{RequestNone}, m_debug{true} {
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
            const std::string tmpSavePath = m_savePath.toStdString();
            emit saved(emu_save(m_saveImage, tmpSavePath.c_str()));
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
            setActualSpeed(0);
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
        setActualSpeed(speed);
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

void EmuThread::save(bool image, const QString &path) {
    m_savePath = path;
    m_saveImage = image;
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
    emit debugCommand(reason, data);
    m_debug = true;
    while (m_debug) {
        m_cvDebug.wait(lock);
    }
}

void EmuThread::resume() {
    m_mutexDebug.lock();
    m_debug = false;
    m_cvDebug.notify_all();
    m_mutexDebug.unlock();
}

void EmuThread::debug(bool state) {
    if (m_debug && !state) {
        resume();
    }
    if (state) {
        req(RequestDebugger);
    }
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
