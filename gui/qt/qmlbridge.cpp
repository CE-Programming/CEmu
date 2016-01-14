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

#include <QtWidgets/QMessageBox>

#include <iostream>
#include <cassert>

#include "qmlbridge.h"
#include "../../core/keypad.h"

QMLBridge::QMLBridge(QObject *p) : QObject(p) {
}

QMLBridge::~QMLBridge() {
}

static const constexpr unsigned int ROWS = 8U, COLS = 8U;

void QMLBridge::keypadStateChanged(unsigned int keymap_id, bool state) {
    unsigned int col = keymap_id % COLS, row = keymap_id / COLS;
    assert(row < ROWS);

    keypad_key_event(row, col, state);
}

static QObject *buttons[ROWS][COLS];

void QMLBridge::registerNButton(unsigned int keymap_id, QVariant button) {
    unsigned int col = keymap_id % COLS, row = keymap_id / COLS;
    assert(row < ROWS);

    if (buttons[row][col]) {
        qWarning() << "Warning: Button " << keymap_id << " already registered as " << buttons[row][col] << "!";
    }
    else {
        buttons[row][col] = button.value<QObject*>();
    }
}


void notifyKeypadStateChanged(unsigned int row, unsigned int col, bool state) {
    assert(row < ROWS);
    assert(col < COLS);

    if(!buttons[row][col]) {
        qWarning() << "Warning: Button " << row*11+col << " not present in keypad!";
        return;
    }

    QQmlProperty::write(buttons[row][col], "state", state);
}

QObject *qmlBridgeFactory(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return new QMLBridge();
}
