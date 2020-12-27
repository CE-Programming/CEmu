#ifndef EMUTHREAD_H
#define EMUTHREAD_H

#include <QtCore/QMutex>
#include <QtCore/QQueue>
#include <QtCore/QSemaphore>
#include <QtCore/QThread>
#include <QtCore/QTimer>

class EmuThread : public QThread {
    Q_OBJECT

public:
    explicit EmuThread(QObject *parent = Q_NULLPTR);

signals:

public slots:

protected:

private:

};

#endif
