/* Copyright (C) 2015  Fabian Vogt
 * Modified for the CE calculator by CEmu developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include "qtkeypadbridge.h"
#include "qmlbridge.h"
#include "keymap.h"
#include "../../core/keypad.h"

QtKeypadBridge qt_keypad_bridge;

bool QtKeypadBridge::setKeymap(const QString & keymapstr)
{
    bool ret = true;
    // TODO select 83pce keypads if emulating a 83PCE.
    if (QStringLiteral("cemu").compare(keymapstr, Qt::CaseInsensitive))
    {
        qt_keypad_bridge.keymap = &keymap_84pce_cemu;
    }
    else if (QStringLiteral("tilem").compare(keymapstr, Qt::CaseInsensitive))
    {
        qt_keypad_bridge.keymap = &keymap_84pce_tilem;
    }
    else if (QStringLiteral("wabbitemu").compare(keymapstr, Qt::CaseInsensitive))
    {
        qt_keypad_bridge.keymap = &keymap_84pce_wabbitemu;
    }
    else
    {
        ret = false;
    }
    return ret;
}

void QtKeypadBridge::keyEvent(QKeyEvent *event, bool press)
{
    Qt::Key key = static_cast<Qt::Key>(event->key());

    for(unsigned int row = 0; row < sizeof(*keymap)/sizeof(**keymap); ++row)
    {
        for(unsigned int col = 0; col < sizeof(**keymap)/sizeof(***keymap); ++col)
        {
            for(unsigned int index = 0; index < sizeof((***keymap).key)/sizeof(*(***keymap).key); ++index)
            {
                if(key == (*qt_keypad_bridge.keymap)[row][col].key[index] && (*qt_keypad_bridge.keymap)[row][col].alt == (bool(event->modifiers() & Qt::AltModifier) || bool(event->modifiers() & Qt::MetaModifier)))
                {
                    keypad_key_event(row, col, press);
                    notifyKeypadStateChanged(row, col, press);
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
