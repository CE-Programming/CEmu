#ifndef KEYMAP_H
#define KEYMAP_H

#include <QtGui/QKeyEvent>

struct HostKey {
    Qt::Key key[3];
    QString name;
    bool alt;
};

extern const HostKey keymap_83pce_cemu[8][8];
extern const HostKey keymap_83pce_tilem[8][8];
extern const HostKey keymap_83pce_wabbitemu[8][8];

extern const HostKey keymap_84pce_cemu[8][8];
extern const HostKey keymap_84pce_tilem[8][8];
extern const HostKey keymap_84pce_wabbitemu[8][8];

#endif
