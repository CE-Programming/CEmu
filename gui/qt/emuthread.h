#ifndef EMUTHREAD_H
#define EMUTHREAD_H

#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QSemaphore>

#include <chrono>
#include <condition_variable>

#include "../../core/asic.h"
#include "../../core/debug/debug.h"

#define CONSOLE_BUFFER_SIZE 512

enum {
    CONSOLE_NORM,
    CONSOLE_ERR,
    CONSOLE_MAX
};

enum {
    REQUEST_NONE,
    REQUEST_PAUSE,
    REQUEST_RESET,
    REQUEST_SAVE,
    REQUEST_SEND,
    REQUEST_RECEIVE,
    REQUEST_DEBUGGER
};

class EmuThread : public QThread {
    Q_OBJECT

public:
    explicit EmuThread(QObject *p = Q_NULLPTR);

    void req(int req);
    void doStuff();
    void throttleTimerWait();
    void writeConsoleBuffer(int type, const char *format, va_list args);
    int consoleWritePosition[CONSOLE_MAX] = {0};
    int consoleReadPosition[CONSOLE_MAX] = {0};
    char consoleBuffer[CONSOLE_MAX][CONSOLE_BUFFER_SIZE];
    QSemaphore consoleWriteSemaphore[CONSOLE_MAX];
    QSemaphore consoleReadSemaphore[CONSOLE_MAX];

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
    int load(bool restore, const QString &rom, const QString &image);
    void stop();

    // Debugging
    void debug(bool);

    // Linking
    void send(const QStringList &fileNames, unsigned int location);
    void unlock();

    // Speed
    void setEmuSpeed(int speed);
    void setThrottleMode(bool throttled);

    // Save / Restore
    void save(bool image, const QString &path);

protected:
    virtual void run() Q_DECL_OVERRIDE;

private:
    void block();
    void setActualSpeed(int value);
    void sendFiles();

    int actualSpeed;
    bool saveImage;

    std::atomic<int> request;
    std::atomic<int> speed;
    std::atomic<bool> throttle;

    QString savePath;
    QStringList vars;
    unsigned int sendLoc;

    std::chrono::steady_clock::time_point lastTime;
    std::mutex mutex;
    std::condition_variable cv;
};

// For friends
extern EmuThread *emu_thread;

#endif
