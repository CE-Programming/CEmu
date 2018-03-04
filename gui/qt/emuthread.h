#ifndef EMUTHREAD_H
#define EMUTHREAD_H

#include "../../core/asic.h"
#include "../../core/debug/debug.h"

#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QSemaphore>

#include <chrono>
#include <condition_variable>

#define CONSOLE_BUFFER_SIZE 512

class EmuThread : public QThread {
    Q_OBJECT

public:
    explicit EmuThread(QObject *p = Q_NULLPTR);

    void doStuff();
    void throttleTimerWait();
    void writeConsoleBuffer(int type, const char *format, va_list args);

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

    int writePos = 0;
    int readPos = 0;
    char buffer[CONSOLE_BUFFER_SIZE];
    QSemaphore write;
    QSemaphore read;

signals:
    // Console Strings
    void consoleStr(int dest);

    // Debugger
    void raiseDebugger();
    void disableDebugger();
    void sendDebugCommand(int reason, uint32_t addr);

    // Status
    void actualSpeedChanged(int value);

    // State
    void saved(bool success);

    // Sending/Receiving
    void sentFile(const QString &file, int ok);
    void locked(int req);

public slots:
    void req(int req);
    int load(bool restore, const QString &rom, const QString &image);
    void reset();
    void stop();
    void debug(bool);

    // Linking
    void send(const QStringList &fileNames, unsigned int location);
    void receive();
    void unlock();

    // Speed
    void setEmuSpeed(int m_speed);
    void setThrottleMode(bool throttled);

    // Save / Restore
    void save(bool image, const QString &path);

protected:
    virtual void run() Q_DECL_OVERRIDE;

private:
    void block();
    void setActualSpeed(int value);
    void sendFiles();

    int m_actualSpeed;
    bool m_saveImage;

    std::atomic<int> m_request;
    std::atomic<int> m_speed;
    std::atomic<bool> m_throttle;

    QString m_savePath;
    QStringList m_vars;
    unsigned int m_sendLoc;

    std::chrono::steady_clock::time_point m_lastTime;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

// For friends
extern EmuThread *emu_thread;

#endif
