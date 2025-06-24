#include "emuthread.h"

#include "../../core/bootver.h"
#include "../../core/control.h"
#include "../../core/cpu.h"
#include "../../core/emu.h"
#include "../../core/extras.h"
#include "../../core/link.h"
#include "../../tests/autotester/autotester.h"
#include "../../tests/autotester/crc32.hpp"
#include "capture/animated-png.h"

#include <QtCore/QVector>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>

#include <cassert>
#include <cstdarg>
#include <thread>

static EmuThread *emu;

// reimplemented callbacks

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

asic_rev_t gui_handle_reset(const boot_ver_t* boot_ver, asic_rev_t loaded_rev, asic_rev_t default_rev, emu_device_t device, bool* python) {
    return emu->handleReset(boot_ver, loaded_rev, default_rev, device, python);
}

namespace {

class ArgList {
public:
    ArgList(const QStringList &args, bool nullTerminate = true);

    const char *const *args();
    std::size_t size();

private:
    QList<QByteArray> m_storage;
    QVector<const char *> m_args;
};

ArgList::ArgList(const QStringList &args, bool nullTerminate) {
    m_storage.reserve(args.size());
    m_args.reserve(args.size() + nullTerminate);
    for (const QString &arg : args) {
        m_storage << arg.toUtf8();
        m_args << m_storage.back().data();
    }
    if (nullTerminate) {
        m_args << nullptr;
    }
}

const char *const *ArgList::args() {
    return m_args.data();
}

std::size_t ArgList::size() {
    return m_storage.size();
}

}

EmuThread::EmuThread(QObject *parent) : QThread{parent}, write{CONSOLE_BUFFER_SIZE},
                                        m_speed{100}, m_throttle{true},
                                        m_lastTime{std::chrono::steady_clock::now()},
                                        m_debug{false} {
    assert(emu == nullptr);
    emu = this;
    std::fill(m_perfArray, m_perfArray + PerfArraySize, m_lastTime);
}

void EmuThread::run() {
    while (!(cpu_check_signals() & CPU_SIGNAL_EXIT)) {
        emu_run(1u);
        doStuff();
        throttleWait();
    }
    asic_free();
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
    {
        std::unique_lock<std::mutex> requestLock{m_requestMutex};
        while (!m_requestQueue.isEmpty()) {
            int req = m_requestQueue.dequeue();
            requestLock.unlock();
            switch (+req) {
                default:
                    break;
                case RequestPause:
                    block(req);
                    break;
                case RequestReset:
                    cpu_crash("user request");
                    break;
                case RequestSend:
                    doSend();
                    break;
                case RequestReceive:
                    block(req);
                    break;
                case RequestUsbPlugDevice:
                    doUsbPlugDevice();
                    break;
                case RequestDebugger:
                    debug_open(DBG_USER, 0);
                    break;
                case RequestBasicDebugger:
                debug_open(DBG_BASIC_RECONFIG, 0);
                    break;
                case RequestSave:
                    doSave();
                    break;
                case RequestLoad:
                    doLoad();
                    break;
                case RequestAutoTester:
                    uint32_t run_rate_prev = emu_get_run_rate();
                    emu_set_run_rate(1000);
                    if (!autotester::doTestSequence()) {
                        emit tested(1);
                    } else {
                        emit tested(0);
                    }
                    emu_set_run_rate(run_rate_prev);
                    break;
            }
            requestLock.lock();
        }
    }

    {
        QMutexLocker locker(&m_keyQueueMutex);
        while (!m_keyQueue.isEmpty() && sendKey(m_keyQueue.head())) {
            m_keyQueue.dequeue();
        }
    }
}

void EmuThread::throttleWait() {
    int speed;
    bool throttle;
    {
        std::unique_lock<std::mutex> lockSpeed(m_mutexSpeed);
        speed = m_speed;
        throttle = m_throttle;
        if (!speed && throttle) {
            emit sendSpeed(0);
            m_cvSpeed.wait(lockSpeed, [this] { return m_speed != 0 || !m_throttle; });
            speed = m_speed;
            throttle = m_throttle;
            m_lastTime = std::chrono::steady_clock::now();
        }
    }
    double run_rate = sched_get_clock_rate_precise(CLOCK_RUN);
    std::chrono::steady_clock::time_point cur_time = std::chrono::steady_clock::now();
    if (!throttle) {
        m_lastTime = cur_time;
        std::this_thread::yield();
    } else {
        std::chrono::steady_clock::duration interval(std::chrono::duration_cast<std::chrono::steady_clock::duration>
                                                     (std::chrono::duration<double>(100 / (speed * run_rate))));
        std::chrono::steady_clock::time_point next_time = m_lastTime + interval;
        std::chrono::steady_clock::time_point tolerance_time = m_lastTime + std::chrono::milliseconds(40);
        if (cur_time < std::max(next_time, tolerance_time)) {
            m_lastTime = next_time;
            if (cur_time < next_time) {
                std::this_thread::sleep_until(next_time);
            }
        } else {
            m_lastTime = cur_time;
            std::this_thread::yield();
        }
    }
    std::chrono::steady_clock::time_point timeNUnitsAgo = m_perfArray[m_perfIndex];
    m_perfArray[m_perfIndex] = m_lastTime;
    if (++m_perfIndex == PerfArraySize) {
        m_perfIndex = 0;
    }
    std::chrono::duration<double> diff = m_lastTime - timeNUnitsAgo;
    emit sendSpeed(diff.count() * run_rate * (1.0 / PerfArraySize));
}

void EmuThread::block(int status) {
    std::unique_lock<std::mutex> lock(m_mutex);
    emit blocked(status);
    m_cv.wait(lock);
}

void EmuThread::unblock() {
    m_mutex.lock();
    m_cv.notify_all();
    m_mutex.unlock();
}

asic_rev_t EmuThread::handleReset(const boot_ver_t* bootVer, asic_rev_t loadedRev, asic_rev_t defaultRev, emu_device_t device, bool* python) {
    // Build a list of supported revisions
    QList<int> supportedRevs;
    supportedRevs.reserve(ASIC_REV_M - ASIC_REV_PRE_A + 1);
    for (int rev = ASIC_REV_PRE_A; rev <= ASIC_REV_M; rev++) {
        if (bootver_check_rev(bootVer, (asic_rev_t)rev, device)) {
            supportedRevs.push_back(rev);
        }
    }

    // If CPU reset, override the ASIC revision
    if (loadedRev == ASIC_REV_AUTO) {
        loadedRev = (asic_rev_t)m_asicRev.load();
        if (!m_allowAnyRev.load() && !supportedRevs.contains((int)loadedRev)) {
            loadedRev = ASIC_REV_AUTO;
        }
        Qt::CheckState forcePython = m_forcePython.load();
        if (forcePython != Qt::PartiallyChecked) {
            *python = (forcePython == Qt::Checked);
        }
    }
    emit sendAsicRevInfo(supportedRevs, (int)loadedRev, (int)defaultRev, *python);

    return (loadedRev != ASIC_REV_AUTO) ? loadedRev : defaultRev;
}

void EmuThread::reset() {
    std::lock_guard<std::mutex> requestLock{m_requestMutex};
    m_requestQueue.enqueue(RequestReset);
}

void EmuThread::cancelTransfer() {
    usbPlugDevice();
}

void EmuThread::receive() {
    std::lock_guard<std::mutex> requestLock{m_requestMutex};
    m_requestQueue.enqueue(RequestReceive);
}

void EmuThread::send(const QStringList &list, int location) {
    std::lock_guard<std::mutex> requestLock{m_requestMutex};
    m_vars = list;
    m_sendLoc = location;
    m_requestQueue.enqueue(RequestSend);
}

void EmuThread::usbPlugDevice(const QStringList &args,
                              usb_progress_handler_t usbProgressHandler,
                              void *usbProgressContext) {
    std::lock_guard<std::mutex> requestLock{m_requestMutex};
    m_usbArgs = args;
    m_usbProgressHandler = usbProgressHandler;
    m_usbProgressContext = usbProgressContext;
    m_requestQueue.enqueue(RequestUsbPlugDevice);
}

void EmuThread::doLoad() {
    std::unique_lock<std::mutex> requestLock{m_requestMutex};
    emu_data_t loadType = m_loadType;
    QByteArray loadPath = m_loadPath.toUtf8();
    requestLock.unlock();
    emit loaded(emu_load(loadType, loadPath.data()), m_loadType);
}


void EmuThread::doSave() {
    std::unique_lock<std::mutex> requestLock{m_requestMutex};
    emu_data_t saveType = m_saveType;
    QByteArray savePath = m_savePath.toUtf8();
    requestLock.unlock();
    emit saved(emu_save(saveType, savePath.data()));
}

void EmuThread::doSend() {
    std::unique_lock<std::mutex> requestLock{m_requestMutex};
    ArgList args{m_vars, false};
    int sendLoc = m_sendLoc;
    m_backupThrottleForTransfers = m_throttle;
    requestLock.unlock();
    setThrottle(false);
    emu_send_variables(args.args(), args.size(), sendLoc,
                       [](void *context, int value, int total) {
                           EmuThread* emuThread = reinterpret_cast<EmuThread *>(context);
                           if (value == 1 && total == 1) {
                               emuThread->setThrottle(emuThread->m_backupThrottleForTransfers);
                           }
                           emit emuThread->linkProgress(value, total);
                           return false;
                       }, this);
}

void EmuThread::doUsbPlugDevice() {
    std::unique_lock<std::mutex> requestLock{m_requestMutex};
    ArgList args{m_usbArgs, true};
    usb_progress_handler_t *usbProgressHandler = m_usbProgressHandler;
    void *usbProgressContext = m_usbProgressContext;
    requestLock.unlock();
    if (usb_plug_device(args.size(), args.args(), usbProgressHandler, usbProgressContext) &&
        usbProgressHandler) {
        usbProgressHandler(usbProgressContext, 0, 0);
    }
}

void EmuThread::doAutotest() {
    uint32_t saveRunRate = emu_get_run_rate();
    emu_set_run_rate(1000);
    emit tested(!autotester::doTestSequence());
    emu_set_run_rate(saveRunRate);
}

void EmuThread::enqueueKeys(quint16 key1, quint16 key2, bool repeat) {
    QMutexLocker locker(&m_keyQueueMutex);
    if (!repeat || m_keyQueue.isEmpty() ||
        (m_keyQueue.front() != key1 && m_keyQueue.front() != key2)) {
        for (auto key : {key1, key2}) {
            if (key) {
                m_keyQueue.enqueue(key);
            }
        }
    }
}

void EmuThread::test(const QString &config, bool run) {
    std::lock_guard<std::mutex> requestLock{m_requestMutex};
    m_autotesterPath = config;
    m_autotesterRun = run;
    m_requestQueue.enqueue(RequestAutoTester);
}

void EmuThread::save(emu_data_t fileType, const QString &filePath) {
    std::lock_guard<std::mutex> requestLock{m_requestMutex};
    m_savePath = filePath;
    m_saveType = fileType;
    m_requestQueue.enqueue(RequestSave);
}

void EmuThread::setSpeed(int value) {
    {
        std::lock_guard<std::mutex> lockSpeed(m_mutexSpeed);
        m_speed = value;
    }
    if (value) {
        m_cvSpeed.notify_one();
    }
}

void EmuThread::setThrottle(bool state) {
    std::lock_guard<std::mutex> lockSpeed(m_mutexSpeed);
    m_throttle = state;
    if (!state) {
        m_cvSpeed.notify_one();
    }
}

void EmuThread::setAsicRev(int rev) {
    m_asicRev.store(rev);
}

void EmuThread::setAllowAnyRev(bool allow) {
    m_allowAnyRev.store(allow);
}

void EmuThread::setForcePython(Qt::CheckState state) {
    m_forcePython.store(state);
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

void EmuThread::debug(bool state, int mode) {
    bool oldState;
    {
        std::lock_guard<std::mutex> lock(m_mutexDebug);
        oldState = m_debug;
    }
    if (oldState && !state) {
        resume();
    }
    if (state) {
        std::lock_guard<std::mutex> requestLock{m_requestMutex};
        m_requestQueue.enqueue(mode);
    }
}

void EmuThread::load(emu_data_t fileType, const QString &filePath) {

    /* if loading an image or rom, we need to restart emulation */
    if (fileType == EMU_DATA_IMAGE || fileType == EMU_DATA_ROM) {
        setTerminationEnabled();
        stop();

        emit loaded(emu_load(fileType, filePath.toStdString().c_str()), fileType);
    } else if (fileType == EMU_DATA_RAM) {
        std::lock_guard<std::mutex> requestLock{m_requestMutex};
        m_loadPath = filePath;
        m_loadType = fileType;
        m_requestQueue.enqueue(RequestLoad);
    }
}

void EmuThread::stop() {
    // Need to run events to allow queued slots to be processed during exit
    QEventLoop eventLoop;
    connect(this, &QThread::finished, &eventLoop, [&]() { eventLoop.exit(0); });
    if (!isRunning()) {
        return;
    }
    emu_exit();
    QTimer::singleShot(500, &eventLoop, [&]() { eventLoop.exit(1); });
    if (eventLoop.exec()) {
        terminate();
        wait(500);
    }
}
