#ifndef EMUTHREAD_H
#define EMUTHREAD_H

#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <chrono>
#include <condition_variable>

#include "../../core/asic.h"
#include "../../core/debug/debug.h"

class EmuThread : public QThread {
    Q_OBJECT

public:
    explicit EmuThread(QObject *p = Q_NULLPTR);

    void doStuff();
    void throttleTimerWait();
    QString rom, image;

signals:
    // Debugger
    void raiseDebugger();
    void disableDebugger();
    void sendDebugCommand(int reason, uint32_t addr);
    void debugInputRequested(bool);

    // I/O
    void consoleStr(const QString& str);
    void consoleErrStr(const QString& str);
    void exited(int);

    // Status
    void sendGuiUpdates(int actualSpeed, int fps);

    // Save/Restore state
    void saved(bool success);
    void restored(bool success);

    // Stopped/Started
    void started(bool success);
    void stopped();

    // Sending/Receiving
    void sentFile(const QString &file, int ok);
    void receiveReady();

public slots:
    bool stop();
    void reset();
    void load();

    // Debugging
    void setDebugMode(bool);
    void setDebugStepInMode();
    void setDebugStepOverMode();
    void setDebugStepNextMode();
    void setDebugStepOutMode();
    void setRunUntilMode();

    // Linking
    void send(const QStringList& fileNames, unsigned int location);
    void receive();
    void receiveDone();

    // Speed
    void setEmuSpeed(int speed);
    void setThrottleMode(bool throttled);

    // Save/Restore
    bool restore(const QString &path);
    void saveImage(const QString &path);
    void saveRom(const QString &path);

    // Speed and fps
    void sendUpdates();

protected:
    virtual void run() Q_DECL_OVERRIDE;

private:
    void setActualSpeed(int actualSpeed);
    void sendFiles();

    int speed, actualSpeed, fps;

    bool doReset = false;

    bool enterDebugger = false;
    bool enterSendState = false;
    bool enterReceiveState = false;
    bool enterRestore = false;

    bool throttleOn = true;
    bool enterSaveImage = false;
    bool enterSaveRom = false;

    QString romExportPath;

    QStringList vars;
    unsigned int sendLoc;

    std::chrono::steady_clock::time_point lastTime;
    std::mutex mutex;
    std::condition_variable cv;

    QTimer guiTimer;
};

// For friends
extern EmuThread *emu_thread;

#endif
