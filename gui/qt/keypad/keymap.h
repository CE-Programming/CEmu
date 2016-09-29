#ifndef KEYMAP_H
#define KEYMAP_H

#include <QtGui/QKeyEvent>

struct HostKey {
    Qt::Key code;
    quint32 nativeCode, nativeMask;
    Qt::KeyboardModifier modifier, mask;
    QString name;
};

extern const HostKey *cemu_keymap_83pce[8][8];
extern const HostKey *tilem_keymap_83pce[8][8];
extern const HostKey *wabbitemu_keymap_83pce[8][8];
extern const HostKey *jstified_keymap_83pce[8][8];

extern const HostKey *cemu_keymap_84pce[8][8];
extern const HostKey *tilem_keymap_84pce[8][8];
extern const HostKey *wabbitemu_keymap_84pce[8][8];
extern const HostKey *jstified_keymap_84pce[8][8];

#endif
