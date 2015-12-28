#ifndef EMUTHREAD_H
#define EMUTHREAD_H

#include <QThread>

#include "core/asic.h"

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

    void consoleStr(QString str);
    void exited(int retcode);

public slots:
    virtual void run() override;
    bool stop();

    // Debugging
    void setDebugMode(bool state);

private:
    bool enter_debugger = false;
};

// For friends
extern EmuThread *emu_thread;

#endif // EMUTHREAD_H
