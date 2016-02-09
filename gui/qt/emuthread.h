#ifndef EMUTHREAD_H
#define EMUTHREAD_H

#include <QtCore/QThread>

#include <chrono>

#include "../../core/asic.h"
#include "../../core/debug/debug.h"

class EmuThread : public QThread
{
    Q_OBJECT
public:
    explicit EmuThread(QObject *p = 0);

    void doStuff();
    void throttleTimerWait();

    std::string rom;

signals:
    // Debugger
    void debuggerEntered();
    void sendDebugCommand(int, uint32_t);
    void debugInputRequested(bool);

    void consoleChar(char);
    void consoleStr(QString);
    void exited(int);

    void actualSpeedChanged(int actualSpeed);

public slots:
    virtual void run() override;
    bool stop();

    // Debugging
    void setDebugMode(bool);
    void setDebugStepInMode();
    void setDebugStepOverMode();
    void setDebugStepNextMode();
    void setDebugStepOutMode();
    void debuggerInput(QString str);

    // Linking
    void setSendState(bool);
    void setReceiveState(bool);

    // Speed
    void changeEmuSpeed(int);
    void changeThrottleMode(bool);

private:
    void setActualSpeed(int value);

    bool enterDebugger = false;
    bool enterSendState = false;
    bool enterReceiveState = false;
    bool throttleOn = true;
    int speed, actualSpeed;
    std::chrono::steady_clock::time_point lastTime;
    std::string debugInput;
};

// For friends
extern EmuThread *emu_thread;

#endif
