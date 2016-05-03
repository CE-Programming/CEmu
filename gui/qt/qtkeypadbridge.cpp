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
#include "../../core/tidevices.h"

QtKeypadBridge qt_keypad_bridge;

bool QtKeypadBridge::setKeymap(const QString & keymapstr)
{
    bool ret = true;
    if (!QStringLiteral("cemu").compare(keymapstr, Qt::CaseInsensitive))
    {
        qt_keypad_bridge.keymap = (get_device_type() == TI84PCE) ? &cemu_keymap_84pce : &cemu_keymap_83pce;
    }
    else if (!QStringLiteral("tilem").compare(keymapstr, Qt::CaseInsensitive))
    {
        qt_keypad_bridge.keymap = (get_device_type() == TI84PCE) ? &tilem_keymap_84pce : &tilem_keymap_83pce;
    }
    else if (!QStringLiteral("wabbitemu").compare(keymapstr, Qt::CaseInsensitive))
    {
        qt_keypad_bridge.keymap = (get_device_type() == TI84PCE) ? &wabbitemu_keymap_84pce : &wabbitemu_keymap_83pce;
    }
    else if (!QStringLiteral("jsTIfied").compare(keymapstr, Qt::CaseInsensitive))
    {
        qt_keypad_bridge.keymap = (get_device_type() == TI84PCE) ? &pindurti_keymap_84pce : &pindurti_keymap_83pce;
    }
    else if (!QStringLiteral("smartview").compare(keymapstr, Qt::CaseInsensitive))
    {
        qt_keypad_bridge.keymap = (get_device_type() == TI84PCE) ? &smartview_keymap_84pce : &smartview_keymap_83pce;
    }
    else
    {
        ret = false;
    }
    return ret;
}

void QtKeypadBridge::keyEvent(QKeyEvent *event, bool press)
{
    Qt::Key code = static_cast<Qt::Key>(event->key());
    quint32 nativeCode = event->nativeScanCode();
    Qt::KeyboardModifiers modifiers = event->modifiers();

    for(unsigned int row = 0; row < sizeof(*keymap)/sizeof(**keymap); ++row)
    {
        for(unsigned int col = 0; col < sizeof(**keymap)/sizeof(***keymap); ++col)
        {
            for(const HostKey *key = (*qt_keypad_bridge.keymap)[row][col]; key->code; ++key)
            {
                if(key->code == code && key->modifier == (key->mask & modifiers) && key->nativeCode == (key->nativeMask & nativeCode))
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
