#ifndef EMUTHREAD_H
#define EMUTHREAD_H

#include "../../core/emu.h"
#include "../../core/debug/debug.h"
#include "../../core/link.h"

#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QSemaphore>

#include <chrono>
#include <condition_variable>

#define CONSOLE_BUFFER_SIZE 512

class EmuThread : public QThread {
    Q_OBJECT

public:
    explicit EmuThread(QObject *parent = Q_NULLPTR);
    void stop();
    void reset();
    void resume();
    void receive();
    void unblock();
    void debug(bool);
    void doStuff();
    void throttleWait();
    void setSpeed(int value);
    void setThrottle(bool state);
    void writeConsole(int console, const char *format, va_list args);
    void debugOpen(int reason, uint32_t addr);
    void save(emu_data_t type, const QString &path);
    void setRam(const QString &path);
    void load(emu_data_t type, const QString &path);
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
        RequestSave,
        RequestLoad,
        RequestSend,
        RequestReceive,
        RequestAutoTester,
        RequestDebugger
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
    void sendSpeed(int value);

    // state
    void tested(int status);
    void saved(bool success);
    void loaded(emu_state_t state, emu_data_t type);
    void blocked(int req);
    void sentFile(const QString &file, int ok);

public slots:
    void send(const QStringList &fileNames, unsigned int location);

protected:
    virtual void run() Q_DECL_OVERRIDE;

private:

    void sendFiles() {
        for (const QString &f : m_vars) {
            emit sentFile(f, sendVariableLink(f.toUtf8(), m_sendLoc));
        }
        emit sentFile(QString(), LINK_GOOD);
    }

    void req(int req) {
        m_request = req;
    }

    void block() {
        std::unique_lock<std::mutex> lock(m_mutex);
        emit blocked(m_request);
        m_cv.wait(lock);
    }

    emu_data_t m_saveType;
    emu_data_t m_loadType;

    int m_speed, m_actualSpeed;
    bool m_throttle;
    std::chrono::steady_clock::time_point m_lastTime;
    std::mutex m_mutexSpeed;
    std::condition_variable m_cvSpeed;

    std::atomic<int> m_request;
    bool m_debug; // protected by m_mutexDebug

    QString m_autotesterPath;
    bool m_autotesterRun;

    QString m_savePath;
    QString m_loadPath;
    QStringList m_vars;
    unsigned int m_sendLoc;

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::mutex m_mutexDebug;
    std::condition_variable m_cvDebug; // protected by m_mutexDebug
};

#endif
