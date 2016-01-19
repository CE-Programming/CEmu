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

    void doStuff(bool);
    void throttleTimerWait();

    std::string rom = "";

signals:
    // Debugger
    void debuggerEntered();
    void sendDebugCommand(int, uint32_t);

    void consoleStr(QString);
    void exited(int);

    void actualSpeedChanged(int actualSpeed);

public slots:
    virtual void run() override;
    bool stop();

    // GIF Rendering
    void renderGIF();

    // Debugging
    void setDebugMode(bool);
    void setDebugStepMode();
    void setDebugStepOverMode();
    void setDebugStepOutMode();

    // Linking
    void setSendState(bool);
    void setReceiveState(bool);

    // Speed
    void changeEmuSpeed(int);
    void changeThrottleMode(bool);

private:
    void setActualSpeed(int value);

    bool enter_debugger = false;
    bool enter_send_state = false;
    bool enter_receive_state = false;
    bool throttle_on = true;
    int speed, actualSpeed;
    std::chrono::steady_clock::time_point last_time;
};

// For friends
extern EmuThread *emu_thread;

#endif
