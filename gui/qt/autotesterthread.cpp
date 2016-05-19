#include "autotesterthread.h"

#include "../../tests/autotester/autotester.h"

AutotesterThread *autotester_thread;

AutotesterThread::AutotesterThread(QObject *parent) : QThread(parent)
{
    autotester_thread = this;
}

void AutotesterThread::launchActualTest()
{
    if (!autotester::configLoaded) {
        emit testError(-1);
        return;
    }

    // Files are sent from the Qt part before that

    // Follow the sequence
    if (!autotester::doTestSequence()) {
        emit testError(1);
        return;
    }
}
