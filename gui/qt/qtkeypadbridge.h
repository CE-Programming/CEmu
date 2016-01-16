#ifndef QTKEYPADBRIDGE_H
#define QTKEYPADBRIDGE_H

#include <QtGui/QKeyEvent>

#include "keymap.h"

/* This class is used by every Widget which wants to interact with the
 * virtual keypad. Simply call QtKeypadBridge::keyPressEvent or keyReleaseEvent
 * to relay the key events into the virtual calc. */

class QtKeypadBridge : public QObject
{
public:
    static bool setKeymap(const QString & keymapstr);
    static void keyEvent(QKeyEvent *event, bool press);
    bool eventFilter(QObject *obj, QEvent *e);
private:
    const HostKey *(*keymap)[8][8] = nullptr;
};

extern QtKeypadBridge qt_keypad_bridge;

#endif
