#ifndef EMUTHREAD_H
#define EMUTHREAD_H

#include <QThread>

class EmuThread : public QThread
{
    Q_OBJECT
public:
    explicit EmuThread(QObject *p = 0);

    void doStuff(bool);
    void throttleTimerWait();

    volatile bool paused = false;
    std::string rom = "";

signals:
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

    // Emulation settings
    void setTurboMode(bool state);
    void toggleTurbo();

private:
};

// For friends
extern EmuThread *emu_thread;

#endif // EMUTHREAD_H
