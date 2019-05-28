#ifndef KEYMAP_H
#define KEYMAP_H

#include <QtGui/QKeyEvent>

struct HostKey {
    Qt::Key code;
    quint32 nativeCode, nativeMask;
    Qt::KeyboardModifiers modifier, mask;
    QString name;
};

extern const HostKey *const cemu_keymap_83pce[8*8];
extern const HostKey *const tilem_keymap_83pce[8*8];
extern const HostKey *const wabbitemu_keymap_83pce[8*8];
extern const HostKey *const jstified_keymap_83pce[8*8];

extern const HostKey *const cemu_keymap_84pce[8*8];
extern const HostKey *const tilem_keymap_84pce[8*8];
extern const HostKey *const wabbitemu_keymap_84pce[8*8];
extern const HostKey *const jstified_keymap_84pce[8*8];

extern HostKey *const custom_keymap[8*8];

#endif
