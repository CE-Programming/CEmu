#ifndef EMUTHREAD_H
#define EMUTHREAD_H

#include "../../core/asic.h"
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
    void save(bool image, const QString &path);
    int load(bool restore, const QString &rom, const QString &image);

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
        RequestSend,
        RequestReceive,
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
    void saved(bool success);
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

    bool m_saveImage;

    int m_speed, m_actualSpeed;
    bool m_throttle;
    std::chrono::steady_clock::time_point m_lastTime;
    std::mutex m_mutexSpeed;
    std::condition_variable m_cvSpeed;

    std::atomic<int> m_request;
    std::atomic<bool> m_debug;

    QString m_savePath;
    QStringList m_vars;
    unsigned int m_sendLoc;

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::mutex m_mutexDebug;
    std::condition_variable m_cvDebug;
};

#endif
