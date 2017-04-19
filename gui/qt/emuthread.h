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
    void sendDebugCommand(int, uint32_t);
    void debugInputRequested(bool);

    // I/O
    void consoleStr(const QString&);
    void consoleErrStr(const QString&);
    void exited(int);

    // Status
    void actualSpeedChanged(int);

    // Save/Restore state
    void saved(bool);
    void restored(bool);

    // Stopped/Started
    void started(bool);
    void stopped();

    // Sending/Receiving
    void sentFile(const QString&, bool);
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
    void send(const QStringList&, unsigned int);
    void receive();
    void receiveDone();

    // Speed
    void setEmuSpeed(int);
    void setThrottleMode(bool);

    // Save/Restore
    bool restore(const QString&);
    void saveImage(const QString&);
    void saveRom(const QString&);

    // Speed
    void sendActualSpeed();

protected:
    virtual void run() Q_DECL_OVERRIDE;

private:
    void setActualSpeed(int);
    void sendFiles();

    int speed, actualSpeed;

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

    QTimer speedTimer;
};

// For friends
extern EmuThread *emu_thread;

#endif
