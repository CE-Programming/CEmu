#ifndef EMUTHREAD_H
#define EMUTHREAD_H

#include <QtCore/QThread>

#include "core/asic.h"
#include "core/debug/debug.h"

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
    void sendDebugCommand(int reason, uint32_t addr);

    void consoleStr(QString str);
    void exited(int retcode);

public slots:
    virtual void run() override;
    bool stop();

    // Debugging
    void setDebugMode(bool state);

    // Linking
    void setSendState(bool state);

private:
    bool enter_debugger = false;
    bool enter_send_state = false;
};

// For friends
extern EmuThread *emu_thread;

#endif
