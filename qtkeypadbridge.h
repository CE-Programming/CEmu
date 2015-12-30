#ifndef QTKEYPADBRIDGE_H
#define QTKEYPADBRIDGE_H

#include <QtGui/QKeyEvent>

/* This class is used by every Widget which wants to interact with the
 * virtual keypad. Simply call QtKeypadBridge::keyPressEvent or keyReleaseEvent
 * to relay the key events into the virtual calc. */

class QtKeypadBridge : public QObject
{
public:
    static void keyEvent(QKeyEvent *event, bool press);
    virtual bool eventFilter(QObject *obj, QEvent *e);
};

extern QtKeypadBridge qt_keypad_bridge;

#endif
