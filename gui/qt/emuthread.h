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
#define CONSOLE_NORM 0
#define CONSOLE_ERR 1

class EmuThread : public QThread {
    Q_OBJECT

public:
    explicit EmuThread(QObject *p = Q_NULLPTR);

    void doStuff();
    void throttleTimerWait();
    void writeConsoleBuffer(int dest, const char *format, va_list args);
    int consoleWritePosition = 0;
    int consoleReadPosition = 0;
    char consoleBuffer[CONSOLE_BUFFER_SIZE];
    QSemaphore consoleWriteSemaphore;
    QSemaphore consoleReadSemaphore;

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
    void stopped();

    // Sending/Receiving
    void sentFile(const QString &file, int ok);
    void receiveReady();

public slots:
    int load(bool restore, const QString &rom, const QString &image);
    bool stop();
    void reset();

    // Debugging
    void debug(bool);

    // Linking
    void send(const QStringList &fileNames, unsigned int location);
    void receive();
    void unlock();

    // Speed
    void setEmuSpeed(int speed);
    void setThrottleMode(bool throttled);

    // Save / Restore
    void save(bool image, const QString &path);

protected:
    virtual void run() Q_DECL_OVERRIDE;

private:
    void setActualSpeed(int actualSpeed);
    void sendFiles();

    int speed, actualSpeed;

    bool doReset = false;

    bool enterDebugger = false;
    bool enterSendState = false;
    bool enterReceiveState = false;
    bool enterRestore = false;
    bool enterSave = false;
    bool saveImage = false;

    bool throttleOn = true;

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
