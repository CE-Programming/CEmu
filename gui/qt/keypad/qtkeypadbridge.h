#ifndef QTKEYPADBRIDGE_H
#define QTKEYPADBRIDGE_H

#include <QtGui/QKeyEvent>

#include "keycode.h"
#include "keymap.h"

/* This class is used by every Widget which wants to interact with the
 * virtual keypad. Simply call QtKeypadBridge::keyEvent
 * to relay the key events into the virtual calc. */

class QtKeypadBridge : public QObject {
    Q_OBJECT

public:
    explicit QtKeypadBridge(QObject *parent = Q_NULLPTR) : QObject(parent) {}

    bool setKeymap(const QString & keymapstr);
    void keyEvent(QKeyEvent *event, bool press);
    void releaseAll();
    bool eventFilter(QObject *obj, QEvent *e);

signals:
    void keyStateChanged(KeyCode, bool, bool = false);

private:
    QHash<quint32, KeyCode> pressed;
    const HostKey *(*keymap)[8][8] = nullptr;
};

inline uint qHash(KeyCode key, uint seed) {
    return key.code() ^ seed;
}

#endif
