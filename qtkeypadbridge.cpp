#include "qtkeypadbridge.h"

#include "keymap.h"
#include "core/keypad.h"
#include "qmlbridge.h"

#include <iostream>

QtKeypadBridge qt_keypad_bridge;

void QtKeypadBridge::keyEvent(QKeyEvent *event, bool press)
{
    Qt::Key key = static_cast<Qt::Key>(event->key());

    auto& keymap = keymap_tp;
    for(unsigned int row = 0; row < sizeof(keymap)/sizeof(*keymap); ++row)
    {
        for(unsigned int col = 0; col < sizeof(*keymap)/sizeof(**keymap); ++col)
        {
            for(unsigned int index = 0; index < sizeof((**keymap).key)/sizeof(*(**keymap).key); ++index)
            {
                if(key == keymap[row][col].key[index] && keymap[row][col].alt == (bool(event->modifiers() & Qt::AltModifier) || bool(event->modifiers() & Qt::MetaModifier)))
                {
                    if(row == 2 && col == 0)
                    {
                        if (press)
                            keypad_on_pressed();
                    }
                    else
                    {
                        keypad.key_map[row] &= ~(1 << col);
                        keypad.key_map[row] |= press << col;
                        notifyKeypadStateChanged(row, col, press);
                        keypad_intrpt_check();
                    }
                    return;
                }
            }
        }
    }

    keypad.gpio_enable |= 0x800;
    keypad_intrpt_check();
}

bool QtKeypadBridge::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj);

    if(e->type() == QEvent::KeyPress) {
        keyEvent(static_cast<QKeyEvent*>(e), true);
    } else if(e->type() == QEvent::KeyRelease) {
        keyEvent(static_cast<QKeyEvent*>(e), false);
    } else {
        return false;
    }

    return true;
}
