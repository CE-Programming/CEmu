#include <QtCore/QSettings>

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

    if (m_mode == KEYMAP_CUSTOM) {
        for (unsigned row = 0; row < sizeof(custom_keymap)/sizeof(*custom_keymap); ++row) {
            for (unsigned col = 0; col < sizeof(*custom_keymap)/sizeof(**custom_keymap); ++col) {
                for (const HostKey *key = custom_keymap[row][col]; key->code; ++key) {
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
    } else {
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
                    key->code = static_cast<Qt::Key>(str2key[key->name]);
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


// this was done because qt is freaking stupid and doesn't actually do it
const QHash<QString, unsigned int> QtKeypadBridge::str2key = QHash<QString, unsigned int>{
    { "Escape", Qt::Key_Escape },
    { "Tab", Qt::Key_Tab },
    { "Backtab", Qt::Key_Backtab },
    { "Backspace", Qt::Key_Backspace },
    { "Return", Qt::Key_Return },
    { "Enter", Qt::Key_Enter },
    { "Insert", Qt::Key_Insert },
    { "Delete", Qt::Key_Delete },
    { "Pause", Qt::Key_Pause },
    { "Print", Qt::Key_Print },
    { "SysReq", Qt::Key_SysReq },
    { "Clear", Qt::Key_Clear },
    { "Home", Qt::Key_Home },
    { "End", Qt::Key_End },
    { "Left", Qt::Key_Left },
    { "Up", Qt::Key_Up },
    { "Right", Qt::Key_Right },
    { "Down", Qt::Key_Down },
    { "PageUp", Qt::Key_PageUp },
    { "PageDown", Qt::Key_PageDown },
    { "Shift", Qt::Key_Shift },
    { "Control", Qt::Key_Control },
    { "Meta", Qt::Key_Meta },
    { "Alt", Qt::Key_Alt },
    { "CapsLock", Qt::Key_CapsLock },
    { "NumLock", Qt::Key_NumLock },
    { "ScrollLock", Qt::Key_ScrollLock },
    { "F1", Qt::Key_F1 },
    { "F2", Qt::Key_F2 },
    { "F3", Qt::Key_F3 },
    { "F4", Qt::Key_F4 },
    { "F5", Qt::Key_F5 },
    { "F6", Qt::Key_F6 },
    { "F7", Qt::Key_F7 },
    { "F8", Qt::Key_F8 },
    { "F9", Qt::Key_F9 },
    { "F10", Qt::Key_F10 },
    { "F11", Qt::Key_F11 },
    { "F12", Qt::Key_F12 },
    { "F13", Qt::Key_F13 },
    { "F14", Qt::Key_F14 },
    { "F15", Qt::Key_F15 },
    { "F16", Qt::Key_F16 },
    { "F17", Qt::Key_F17 },
    { "F18", Qt::Key_F18 },
    { "F19", Qt::Key_F19 },
    { "F20", Qt::Key_F20 },
    { "F21", Qt::Key_F21 },
    { "F22", Qt::Key_F22 },
    { "F23", Qt::Key_F23 },
    { "F24", Qt::Key_F24 },
    { "F25", Qt::Key_F25 },
    { "F26", Qt::Key_F26 },
    { "F27", Qt::Key_F27 },
    { "F28", Qt::Key_F28 },
    { "F29", Qt::Key_F29 },
    { "F30", Qt::Key_F30 },
    { "F31", Qt::Key_F31 },
    { "F32", Qt::Key_F32 },
    { "F33", Qt::Key_F33 },
    { "F34", Qt::Key_F34 },
    { "F35", Qt::Key_F35 },
    { "Super_L", Qt::Key_Super_L },
    { "Super_R", Qt::Key_Super_R },
    { "Menu", Qt::Key_Menu },
    { "Hyper_L", Qt::Key_Hyper_L },
    { "Hyper_R", Qt::Key_Hyper_R },
    { "Help", Qt::Key_Help },
    { "Direction_L", Qt::Key_Direction_L },
    { "Direction_R", Qt::Key_Direction_R },
    { "Space", Qt::Key_Space },
    { "Any", Qt::Key_Any },
    { "Exclam", Qt::Key_Exclam },
    { "QuoteDbl", Qt::Key_QuoteDbl },
    { "NumberSign", Qt::Key_NumberSign },
    { "Dollar", Qt::Key_Dollar },
    { "Percent", Qt::Key_Percent },
    { "Ampersand", Qt::Key_Ampersand },
    { "Apostrophe", Qt::Key_Apostrophe },
    { "ParenLeft", Qt::Key_ParenLeft },
    { "ParenRight", Qt::Key_ParenRight },
    { "Asterisk", Qt::Key_Asterisk },
    { "Plus", Qt::Key_Plus },
    { "Comma", Qt::Key_Comma },
    { "Minus", Qt::Key_Minus },
    { "Period", Qt::Key_Period },
    { "Slash", Qt::Key_Slash },
    { "0", Qt::Key_0 },
    { "1", Qt::Key_1 },
    { "2", Qt::Key_2 },
    { "3", Qt::Key_3 },
    { "4", Qt::Key_4 },
    { "5", Qt::Key_5 },
    { "6", Qt::Key_6 },
    { "7", Qt::Key_7 },
    { "8", Qt::Key_8 },
    { "9", Qt::Key_9 },
    { "Colon", Qt::Key_Colon },
    { "Semicolon", Qt::Key_Semicolon },
    { "Less", Qt::Key_Less },
    { "Equal", Qt::Key_Equal },
    { "Greater", Qt::Key_Greater },
    { "Question", Qt::Key_Question },
    { "At", Qt::Key_At },
    { "A", Qt::Key_A },
    { "B", Qt::Key_B },
    { "C", Qt::Key_C },
    { "D", Qt::Key_D },
    { "E", Qt::Key_E },
    { "F", Qt::Key_F },
    { "G", Qt::Key_G },
    { "H", Qt::Key_H },
    { "I", Qt::Key_I },
    { "J", Qt::Key_J },
    { "K", Qt::Key_K },
    { "L", Qt::Key_L },
    { "M", Qt::Key_M },
    { "N", Qt::Key_N },
    { "O", Qt::Key_O },
    { "P", Qt::Key_P },
    { "Q", Qt::Key_Q },
    { "R", Qt::Key_R },
    { "S", Qt::Key_S },
    { "T", Qt::Key_T },
    { "U", Qt::Key_U },
    { "V", Qt::Key_V },
    { "W", Qt::Key_W },
    { "X", Qt::Key_X },
    { "Y", Qt::Key_Y },
    { "Z", Qt::Key_Z },
    { "BracketLeft", Qt::Key_BracketLeft },
    { "Backslash", Qt::Key_Backslash },
    { "BracketRight", Qt::Key_BracketRight },
    { "AsciiCircum", Qt::Key_AsciiCircum },
    { "Underscore", Qt::Key_Underscore },
    { "QuoteLeft", Qt::Key_QuoteLeft },
    { "BraceLeft", Qt::Key_BraceLeft },
    { "Bar", Qt::Key_Bar },
    { "BraceRight", Qt::Key_BraceRight },
    { "AsciiTilde", Qt::Key_AsciiTilde },
    { "unknown", Qt::Key_unknown },
};
