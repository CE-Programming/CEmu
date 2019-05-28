#include <QtCore/QSettings>
#include <QtCore/QMetaEnum>

#include "utils.h"
#include "qtkeypadbridge.h"
#include "../../../core/keypad.h"
#include "../../../core/asic.h"

QtKeypadBridge *keypadBridge = Q_NULLPTR;

static const QString custom_keys[8][8] = {
    { "", "", "", "", "", "", "", "" },
    { "graph", "trace", "zoom", "wind", "yequ", "2nd", "mode", "del" },
    { "on", "sto", "ln", "log", "sq", "inv", "math", "alpha" },
    { "0", "1", "4", "7", "comma", "sin", "apps", "xton" },
    { "dot", "2", "5", "8", "lpar", "cos", "prgm", "stat" },
    { "neg", "3", "6", "9", "rpar", "tan", "vars", "" },
    { "enter", "add", "sub", "mul", "div", "pow", "clr", "" },
    { "down", "left", "right", "up", "", "", "", "" }
};

bool QtKeypadBridge::setKeymap(KeymapMode map) {
    bool ret = true;
    m_mode = map;
    switch (map) {
        case KEYMAP_CEMU:
            keymap = get_device_type() == TI84PCE ? &cemu_keymap_84pce : &cemu_keymap_83pce;
            break;
        case KEYMAP_TILEM:
            keymap = get_device_type() == TI84PCE ? &tilem_keymap_84pce : &tilem_keymap_83pce;
            break;
        case KEYMAP_WABBITEMU:
            keymap = get_device_type() == TI84PCE ? &wabbitemu_keymap_84pce : &wabbitemu_keymap_83pce;
            break;
        case KEYMAP_JSTIFIED:
            keymap = get_device_type() == TI84PCE ? &jstified_keymap_84pce : &jstified_keymap_83pce;
            break;
        case KEYMAP_CUSTOM:
            keymap = &custom_keymap;
            break;
    }
    return ret;
}

void QtKeypadBridge::keyEvent(QKeyEvent *event, bool press) {
    if (event->isAutoRepeat())
        return;

    Qt::Key code = static_cast<Qt::Key>(event->key());
    quint32 nativeCode = event->nativeScanCode();
    Qt::KeyboardModifiers modifiers = event->modifiers();

    if (modifiers == (Qt::ShiftModifier | Qt::ControlModifier)) {
        return;
    }

    KeyCode lastKey = pressed.take(nativeCode);
    if (lastKey.valid()) {
        emit keyStateChanged(lastKey, false);
        if (!press) {
            return;
        }
    }

    for (unsigned row = 0; row < sizeof(*keymap)/sizeof(**keymap); ++row) {
        for (unsigned col = 0; col < sizeof(**keymap)/sizeof(***keymap); ++col) {
            for (const HostKey *key = (*keymap)[row][col]; key->code; ++key) {
                if (key->code == code && key->modifier == (key->mask & modifiers)
                    && key->nativeCode == (key->nativeMask & nativeCode)) {
                    emit keyStateChanged({row, col}, press);
                    if (nativeCode > 1 && press) {
                        pressed[nativeCode] = {row, col};
                    }
                    return;
                }
            }
        }
    }

    keypad.gpioEnable |= 0x800;
    keypad_intrpt_check();
}

void QtKeypadBridge::releaseAll() {
    for (KeyCode code : pressed) {
        if (code.valid()) {
            emit keyStateChanged(code, false);
        }
    }
    pressed.clear();
}

QString QtKeypadBridge::toModifierString(Qt::KeyboardModifiers m) {
    QString seq;
    if (m & Qt::ControlModifier) { seq += "Ctrl+"; }
    if (m & Qt::AltModifier) { seq += "Alt+"; }
    if (m & Qt::ShiftModifier) { seq += "Shift+"; }
    if (m & Qt::MetaModifier) { seq += "Meta+"; }
    if (!seq.isEmpty()) {
        seq.truncate(seq.length() - 1);
    } else {
        seq = QStringLiteral("None");
    }
    return seq;
}

Qt::KeyboardModifiers QtKeypadBridge::toModifierValue(QString m) {
    Qt::KeyboardModifiers seq = Qt::NoModifier;
    if (m.contains("Ctrl")) { seq |= Qt::ControlModifier; }
    if (m.contains("Alt")) { seq |= Qt::AltModifier; }
    if (m.contains("Shift")) { seq |= Qt::ShiftModifier; }
    if (m.contains("Meta")) { seq |= Qt::MetaModifier; }
    return seq;
}

bool QtKeypadBridge::keymapExport(const QString &path) {
    QSettings config(path, QSettings::IniFormat);
    config.clear();

    for (unsigned row = 0; row < sizeof(*keymap)/sizeof(**keymap); ++row) {
        for (unsigned col = 0; col < sizeof(**keymap)/sizeof(***keymap); ++col) {
            QStringList nameBindings;
            QStringList nativeCodeBindings;
            QStringList nativeMaskBindings;
            QStringList modifierBindings;
            QStringList maskBindings;
            for (const HostKey *key = (*keymap)[row][col]; key->code; ++key) {
                nameBindings.append(key->name);
                nativeCodeBindings.append(QString::number(key->nativeCode, 16));
                nativeMaskBindings.append(QString::number(key->nativeMask, 16));
                modifierBindings.append(toModifierString(key->modifier));
                maskBindings.append(toModifierString(key->mask));
            }
            if (!custom_keys[row][col].isEmpty()) {
                config.setValue(custom_keys[row][col] + QStringLiteral("/keys"), nameBindings);
                config.setValue(custom_keys[row][col] + QStringLiteral("/modifiers"), modifierBindings);
                config.setValue(custom_keys[row][col] + QStringLiteral("/masks"), maskBindings);
                config.setValue(custom_keys[row][col] + QStringLiteral("/native_codes"), nativeCodeBindings);
                config.setValue(custom_keys[row][col] + QStringLiteral("/native_masks"), nativeMaskBindings);
            }
        }
    }

    config.sync();
    return true;
}

bool QtKeypadBridge::keymapImport(const QString &path) {
    if (!fileExists(path)) {
        return false;
    }

    QSettings config(path, QSettings::IniFormat);

    for (unsigned row = 0; row < sizeof(custom_keymap)/sizeof(*custom_keymap); ++row) {
        for (unsigned col = 0; col < sizeof(*custom_keymap)/sizeof(**custom_keymap); ++col) {
            if (!custom_keys[row][col].isEmpty()) {
                QStringList nameBindings = config.value(custom_keys[row][col] + QStringLiteral("/keys")).toStringList();
                QStringList nativeCodeBindings = config.value(custom_keys[row][col] + QStringLiteral("/native_codes")).toStringList();
                QStringList nativeMaskBindings = config.value(custom_keys[row][col] + QStringLiteral("/native_masks")).toStringList();
                QStringList modifierBindings = config.value(custom_keys[row][col] + QStringLiteral("/modifiers")).toStringList();
                QStringList maskBindings = config.value(custom_keys[row][col] + QStringLiteral("/masks")).toStringList();
                if (nameBindings.length() != nativeCodeBindings.length() ||
                    nameBindings.length() != nativeMaskBindings.length() ||
                    nameBindings.length() != modifierBindings.length() ||
                    nameBindings.length() != maskBindings.length() ||
                    nameBindings.length() > 4) {
                    return false;
                }
                HostKey *key = custom_keymap[row][col];
                for (int i = 0; i < nameBindings.length(); ++i) {
                    key->name = nameBindings.at(i);
                    key->code = Qt::Key(QMetaEnum::fromType<Qt::Key>().keyToValue(("Key_" + key->name).toUtf8()));
                    key->mask = toModifierValue(maskBindings.at(i));
                    key->modifier = toModifierValue(modifierBindings.at(i));
                    key->nativeCode = static_cast<quint32>(nativeCodeBindings.at(i).toInt(nullptr, 16));
                    key->nativeMask = static_cast<quint32>(nativeMaskBindings.at(i).toInt(nullptr, 16));
                    key++;
                }
            }
        }
    }

    return true;
}

bool QtKeypadBridge::eventFilter(QObject *obj, QEvent *e){
    Q_UNUSED(obj);

    if (e->type() == QEvent::KeyPress) {
        keyEvent(static_cast<QKeyEvent*>(e), true);
    } else if (e->type() == QEvent::KeyRelease) {
        keyEvent(static_cast<QKeyEvent*>(e), false);
    } else if (e->type() == QEvent::WindowDeactivate) {
        releaseAll();
    }

    return false;
}
