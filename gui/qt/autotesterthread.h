#ifndef AUTOTESTERTHREAD_H
#define AUTOTESTERTHREAD_H

#include <QtCore/QThread>

class AutotesterThread : public QThread
{
    Q_OBJECT
public:
    explicit AutotesterThread(QObject *parent = 0);
    void launchActualTest();

signals:
    void testError(int errCode);

};

// For friends
extern AutotesterThread *autotester_thread;

#endif
