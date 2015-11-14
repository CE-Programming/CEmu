#ifndef EMUTHREAD_H
#define EMUTHREAD_H

#include <QThread>

class EmuThread : public QThread
{
    Q_OBJECT
public:
    explicit EmuThread(QObject *p = 0);

    void doStuff();

    volatile bool paused = false;
    std::string rom = "";

signals:
    void consoleStr(QString str);
    void exited(int retcode);

public slots:
    virtual void run() override;
    void setPaused(bool paused);
    bool stop();
    void test();

private:
};

// For friends
extern EmuThread *emu_thread;

#endif // EMUTHREAD_H
