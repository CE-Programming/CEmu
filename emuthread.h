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

    asic_state_t *asic_ptr = NULL;
    volatile bool paused = false;
    std::string rom = "";

signals:
    // Debugger
    void debuggerEntered(bool state);

    void consoleStr(QString str);
    void exited(int retcode);

    // Status
    void statusMsg(QString str);
    void speedChanged(double value);
    void turboModeChanged(bool state);

public slots:
    virtual void run() override;
    void setPaused(bool paused);
    bool stop();
    void test();

    // Debugging
    void enterDebugger();

    // Emulation settings
    void setTurboMode(bool state);
    void toggleTurbo();

private:
    bool enter_debugger = false;
};

// For friends
extern EmuThread *emu_thread;
enum current_registers {
  AF, HL, DE, BC, IX, IY, AF_, HL_, DE_, BC_,
  MB, PC, SPS, SPL
};

#endif // EMUTHREAD_H
