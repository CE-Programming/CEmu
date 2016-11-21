#ifndef EMUTHREAD_H
#define EMUTHREAD_H

#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <chrono>

#include "../../core/asic.h"
#include "../../core/debug/debug.h"

extern QTimer speedUpdateTimer;

class EmuThread : public QThread {
    Q_OBJECT
public:
    explicit EmuThread(QObject *p = Q_NULLPTR);

    void doStuff();
    void throttleTimerWait();
    volatile bool waitForLink = false;
    QString rom, image;

signals:
    // Debugger
    void raiseDebugger();
    void disableDebugger();
    void sendDebugCommand(int, uint32_t);
    void debugInputRequested(bool);

    // I/O
    void consoleStr(QString);
    void consoleErrStr(QString);
    void exited(int);

    // Status
    void actualSpeedChanged(int);
    void isBusy(bool busy);

    // Save/Restore state
    void saved(bool);
    void restored(bool);

    // Stopped/Started
    void started(bool);
    void stopped();

public slots:
    virtual void run() override;
    bool stop();
    void resetTriggered();

    // Debugging
    void setDebugMode(bool);
    void setDebugStepInMode();
    void setDebugStepOverMode();
    void setDebugStepNextMode();
    void setDebugStepOutMode();
    void setRunUntilMode();

    // Linking
    void setSendState(bool);
    void setReceiveState(bool);

    // Speed
    void setEmuSpeed(int);
    void changeThrottleMode(bool);

    // Save/Restore
    bool restore(QString);
    void save(QString);
    void saveRomImage(QString);

    // Speed
    void sendActualSpeed();

private:
    void setActualSpeed(int);

    int speed, actualSpeed;
    bool enterDebugger = false;
    bool enterSendState = false;
    bool enterReceiveState = false;
    bool throttleOn = true;
    std::chrono::steady_clock::time_point lastTime;
    QString romExportPath;
    volatile bool saveImage = false;
    volatile bool saveRom = false;
    volatile bool doRestore = false;
};

// For friends
extern EmuThread *emu_thread;

#endif
