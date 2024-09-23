/*
 * Copyright (c) 2015-2024 CE Programming.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CORETHREAD_H
#define CORETHREAD_H

#include "../../core/debug/debug.h"
#include "../../core/emu.h"
#include "../../core/link.h"
#include "../../core/usb/usb.h"

#include <QtCore/QMutex>
#include <QtCore/QQueue>
#include <QtCore/QSemaphore>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <atomic>
#include <chrono>
#include <condition_variable>

namespace cemucore
{
    Q_NAMESPACE
#include <../../core/cemucore.h>
}

#define CONSOLE_BUFFER_SIZE 512

class CoreThread : public QThread {
    Q_OBJECT

public:
    explicit CoreThread(QObject *parent = Q_NULLPTR);
    void stop();
    void reset();
    void resume();
    void receive();
    void unblock();
    void debug(bool state, int mode);
    void doStuff();
    void throttleWait();
    void setSpeed(int value);
    void setThrottle(bool state);
    void setAsicRev(int rev);
    void setAllowAnyRev(bool allow);
    void setForcePython(Qt::CheckState state);
    asic_rev_t handleReset(const boot_ver_t* bootVer, asic_rev_t loadedRev, asic_rev_t defaultRev, bool* python);
    void writeConsole(int console, const char *format, va_list args);
    void debugOpen(int reason, uint32_t addr);
    void save(emu_data_t fileType, const QString &filePath);
    void setRam(const QString &path);
    void load(emu_data_t fileType, const QString &filePath);
    void test(const QString &config, bool run);

    enum {
        ConsoleNorm,
        ConsoleErr,
        ConsoleMax
    };
    enum {
        RequestNone,
        RequestPause,
        RequestReset,
        RequestLoad,
        RequestSave,
        RequestSend,
        RequestReceive,
        RequestUsbPlugDevice,
        RequestAutoTester,
        RequestDebugger,
        RequestBasicDebugger
    };

    int type = ConsoleNorm;
    int writePos = 0;
    int readPos = 0;
    char buffer[CONSOLE_BUFFER_SIZE];
    QSemaphore write;
    QSemaphore read;

signals:
    // console
    void consoleStr();
    void consoleClear();

    // debug
    void debugDisable();
    void debugCommand(int reason, uint32_t addr);

    // speed
    void sendSpeed(double value);

    // state
    void sendAsicRevInfo(const QList<int>& supportedRevs, int loadedRev, int defaultRev, bool python);
    void tested(int status);
    void saved(bool success);
    void loaded(emu_state_t state, emu_data_t type);
    void blocked(int req);
    void linkProgress(int value, int total);

public slots:
    void send(const QStringList &names, int location);
    void cancelTransfer();
    void usbPlugDevice(const QStringList &args = {},
                       usb_progress_handler_t usbProgressHandler = nullptr,
                       void *usbProgressContext = nullptr);
    void enqueueKeys(quint16 key1, quint16 key2 = 0, bool repeat = false);

protected:
    void run() override;

private:

    void doLoad();
    void doSave();
    void doSend();
    void doUsbPlugDevice();
    void doAutotest();

    void block(int status);

    static constexpr size_t PerfArraySize = 20;

    int m_speed, m_actualSpeed;
    bool m_throttle, m_backupThrottleForTransfers;
    std::chrono::steady_clock::time_point m_lastTime;
    std::mutex m_mutexSpeed;
    std::condition_variable m_cvSpeed;

    bool m_debug; // protected by m_mutexDebug

    std::mutex m_requestMutex;
    QQueue<int> m_requestQueue;

    emu_data_t m_saveType;
    QString m_savePath;

    emu_data_t m_loadType;
    QString m_loadPath;

    QString m_autotesterPath;
    bool m_autotesterRun;

    QStringList m_vars;
    int m_sendLoc;

    QStringList m_usbArgs;
    usb_progress_handler_t *m_usbProgressHandler;
    void *m_usbProgressContext;
    // end requestMutex protection

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::mutex m_mutexDebug;
    std::condition_variable m_cvDebug; // protected by m_mutexDebug

    std::atomic_int m_asicRev;
    std::atomic_bool m_allowAnyRev;
    std::atomic<Qt::CheckState> m_forcePython;

    QQueue<quint16> m_keyQueue;
    QMutex m_keyQueueMutex;

    std::chrono::steady_clock::time_point m_perfArray[PerfArraySize];
    size_t m_perfIndex = 0;
};

#endif
