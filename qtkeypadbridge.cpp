#include "qtkeypadbridge.h"

#include "keymap.h"
#include "core/keypad.h"
#include "qmlbridge.h"

#include <iostream>

QtKeypadBridge qt_keypad_bridge;

void QtKeypadBridge::keyPressEvent(QKeyEvent *event)
{
    Qt::Key key = static_cast<Qt::Key>(event->key());

    switch(key)
    {
    case Qt::Key_Return:
        key = Qt::Key_Enter;

    default:
        auto& keymap = keymap_tp;
        for(unsigned int row = 0; row < sizeof(keymap)/sizeof(*keymap); ++row)
        {
            for(unsigned int col = 0; col < sizeof(*keymap)/sizeof(**keymap); ++col)
            {
                if(key == keymap[row][col].key && keymap[row][col].alt == (bool(event->modifiers() & Qt::AltModifier) || bool(event->modifiers() & Qt::MetaModifier)))
                {
                    if(row == 0 && col == 9)
                        keypad_on_pressed();

                    keypad.key_map[row] |= 1 << col;
                    notifyKeypadStateChanged(row, col, true);
                    keypad_intrpt_check();
                    return;
                }
            }
        }
        return;
    }

    keypad.gpio_interrupt_mask |= 0x800;

    keypad_intrpt_check();
}

void QtKeypadBridge::keyReleaseEvent(QKeyEvent *event)
{
    Qt::Key key = static_cast<Qt::Key>(event->key());

    switch(key)
    {
    case Qt::Key_Return:
        key = Qt::Key_Enter;
    default:
        auto& keymap = keymap_tp;
        for(unsigned int row = 0; row < sizeof(keymap)/sizeof(*keymap); ++row)
        {
            for(unsigned int col = 0; col < sizeof(*keymap)/sizeof(**keymap); ++col)
            {
                if(key == keymap[row][col].key && keymap[row][col].alt == (bool(event->modifiers() & Qt::AltModifier) || bool(event->modifiers() & Qt::MetaModifier)))
                {
                    keypad.key_map[row] &= ~(1 << col);
                    notifyKeypadStateChanged(row, col, false);
                    keypad_intrpt_check();
                    return;
                }
            }
        }
        return;
    }

    keypad.gpio_interrupt_mask |= 0x800;
    keypad_intrpt_check();
}

bool QtKeypadBridge::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj);

    // OKAY, WHY IS THIS NOT WORKING... THE EVENT FILTER IS CALLED, YET NEVER ON A KEYPRESS...

    if(e->type() == QEvent::KeyPress) {
        keyPressEvent(static_cast<QKeyEvent*>(e));
    } else if(e->type() == QEvent::KeyRelease) {
        keyReleaseEvent(static_cast<QKeyEvent*>(e));
    } else {
        return false;
    }

    return true;
}
