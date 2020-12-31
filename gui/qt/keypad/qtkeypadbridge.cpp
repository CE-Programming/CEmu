#include "qtkeypadbridge.h"

#include <cemucore.h>

#include <QtCore/QMetaEnum>
#include <QtCore/QSettings>

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

bool QtKeypadBridge::setKeymap(Keymap map)
{
    bool ret = true;
    mKeymap = map;
    switch (map) {
        case Keymap::Natural:
            keymap = nullptr;
            break;
        case Keymap::CEmu:
            keymap = get_device_type() == TI84PCE ? cemu_keymap_84pce : cemu_keymap_83pce;
            break;
        case Keymap::TilEm:
            keymap = get_device_type() == TI84PCE ? tilem_keymap_84pce : tilem_keymap_83pce;
            break;
        case Keymap::WabbitEmu:
            keymap = get_device_type() == TI84PCE ? wabbitemu_keymap_84pce : wabbitemu_keymap_83pce;
            break;
        case Keymap::JsTIfied:
            keymap = get_device_type() == TI84PCE ? jstified_keymap_84pce : jstified_keymap_83pce;
            break;
        case Keymap::Custom:
            keymap = custom_keymap;
            break;
    }
    return ret;
}

void QtKeypadBridge::skEvent(QKeyEvent *event, bool press) {
    if (event->isAutoRepeat())
        return;

    Qt::Key code = Qt::Key(event->key());
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

    for (unsigned row = 0; row < 8; ++row) {
        for (unsigned col = 0; col < 8; ++col) {
            for (const HostKey *key = keymap[row*8+col]; key->code; ++key) {
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

void QtKeypadBridge::kEvent(QString text, int key, bool repeat) {
    quint32 keys = kKeyMap[key & ~Qt::KeypadModifier];
    if (!keys && text.length() == 1) {
        keys = kTextMap[text[0]];
    }
    emit sendKeys(keys >> 16, keys, repeat);
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

    for (unsigned row = 0; row < 8; ++row) {
        for (unsigned col = 0; col < 8; ++col) {
            QStringList nameBindings;
            QStringList nativeCodeBindings;
            QStringList nativeMaskBindings;
            QStringList modifierBindings;
            QStringList maskBindings;
            for (const HostKey *key = keymap[row*8+col]; key->code; ++key) {
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
    QSettings config(path, QSettings::IniFormat);

    for (unsigned row = 0; row < 8; ++row) {
        for (unsigned col = 0; col < 8; ++col) {
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
                HostKey *key = custom_keymap[row*8+col];
                for (int i = 0; i < nameBindings.length(); ++i) {
                    key->name = nameBindings.at(i);
                    key->code = Qt::Key(QMetaEnum::fromType<Qt::Key>().keyToValue(("Key_" + key->name).toUtf8()));
                    key->mask = toModifierValue(maskBindings.at(i));
                    key->modifier = toModifierValue(modifierBindings.at(i));
                    key->nativeCode = quint32(nativeCodeBindings.at(i).toInt(nullptr, 16));
                    key->nativeMask = quint32(nativeMaskBindings.at(i).toInt(nullptr, 16));
                    key++;
                }
            }
        }
    }

    return true;
}

bool QtKeypadBridge::eventFilter(QObject *obj, QEvent *e) {
    Q_UNUSED(obj);

    if (keymap) {
        switch (e->type()) {
            case QEvent::KeyPress:
            case QEvent::KeyRelease:
                skEvent(static_cast<QKeyEvent *>(e), e->type() == QEvent::KeyPress);
                break;
            case QEvent::WindowDeactivate:
                releaseAll();
                break;
            default:
                return false;
        }
    } else {
        switch (e->type()) {
            case QEvent::KeyPress: {
                QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
                kEvent(keyEvent->text(), keyEvent->modifiers() | keyEvent->key(),
                       keyEvent->isAutoRepeat());
                break;
            }
            case QEvent::InputMethod:
                kEvent(static_cast<QInputMethodEvent *>(e)->commitString());
                break;
            default:
                return false;
        }
    }

    return true;
}

const QHash<QChar, quint32> QtKeypadBridge::kTextMap = {
    { u' ',      0x00000099 }, // kSpace
    { u'!',      0x0000FE24 }, // kExtendEcho | kXFactorial
    { u'"',      0x000000CB }, // kQuote
    { u'#',      0x0000FB03 }, // kExtendEcho3 | kHash
    { u'$',      0x0000FB04 }, // kExtendEcho3 | kDollar
    { u'%',      0x0000FB0C }, // kExtendEcho3 | kPercent
    { u'&',      0x0000FB05 }, // kExtendEcho3 | kAmp
    { u'\'',     0x0000FE27 }, // kExtendEcho | kAPost
    { u'(',      0x00000085 }, // kLParen
    { u')',      0x00000086 }, // kRParen
    { u'*',      0x00000082 }, // kMul
    { u'+',      0x00000080 }, // kAdd
    { u',',      0x0000008B }, // kComma
    { u'-',      0x00000081 }, // kSub
    { u'.',      0x0000008D }, // kDecPnt
    { u'/',      0x00000083 }, // kDiv
    { u'0',      0x0000008E }, // k0
    { u'1',      0x0000008F }, // k1
    { u'2',      0x00000090 }, // k2
    { u'3',      0x00000091 }, // k3
    { u'4',      0x00000092 }, // k4
    { u'5',      0x00000093 }, // k5
    { u'6',      0x00000094 }, // k6
    { u'7',      0x00000095 }, // k7
    { u'8',      0x00000096 }, // k8
    { u'9',      0x00000097 }, // k9
    { u':',      0x000000C6 }, // kColon
    { u';',      0x0000FB07 }, // kExtendEcho3 | kSemicolon
    { u'<',      0x0000FE0E }, // kExtendEcho | kTLT
    { u'=',      0x0000FE0A }, // kExtendEcho | kTequ
    { u'>',      0x0000FE0C }, // kExtendEcho | kTGT
    { u'?',      0x000000CA }, // kQuest
    { u'@',      0x0000FB02 }, // kExtendEcho3 | kAt
    { u'A',      0x0000009A }, // kCapA
    { u'B',      0x0000009B }, // kCapB
    { u'C',      0x0000009C }, // kCapC
    { u'D',      0x0000009D }, // kCapD
    { u'E',      0x0000009E }, // kCapE
    { u'F',      0x0000009F }, // kCapF
    { u'G',      0x000000A0 }, // kCapG
    { u'H',      0x000000A1 }, // kCapH
    { u'I',      0x000000A2 }, // kCapI
    { u'J',      0x000000A3 }, // kCapJ
    { u'K',      0x000000A4 }, // kCapK
    { u'L',      0x000000A5 }, // kCapL
    { u'M',      0x000000A6 }, // kCapM
    { u'N',      0x000000A7 }, // kCapN
    { u'O',      0x000000A8 }, // kCapO
    { u'P',      0x000000A9 }, // kCapP
    { u'Q',      0x000000AA }, // kCapQ
    { u'R',      0x000000AB }, // kCapR
    { u'S',      0x000000AC }, // kCapS
    { u'T',      0x000000AD }, // kCapT
    { u'U',      0x000000AE }, // kCapU
    { u'V',      0x000000AF }, // kCapV
    { u'W',      0x000000B0 }, // kCapW
    { u'X',      0x000000B1 }, // kCapX
    { u'Y',      0x000000B2 }, // kCapY
    { u'Z',      0x000000B3 }, // kCapZ
    { u'[',      0x00000087 }, // kLBrack
    { u'\\',     0x0000FB08 }, // kExtendEcho3 | kBackslash
    { u']',      0x00000088 }, // kRBrack
    { u'^',      0x00000084 }, // kExpon
    { u'_',      0x0000FB0A }, // kExtendEcho3 | kUnderscore
    { u'`',      0x0000FB06 }, // kExtendEcho3 | kBackquote
    { u'a',      0x0000FCE2 }, // kExtendEcho2 | kLa
    { u'b',      0x0000FCE3 }, // kExtendEcho2 | kLb
    { u'c',      0x0000FCE4 }, // kExtendEcho2 | kLc
    { u'd',      0x0000FCE5 }, // kExtendEcho2 | kLd
    { u'e',      0x0000FCE6 }, // kExtendEcho2 | kLe
    { u'f',      0x0000FCE7 }, // kExtendEcho2 | kLf
    { u'g',      0x0000FCE8 }, // kExtendEcho2 | kLg
    { u'h',      0x0000FCE9 }, // kExtendEcho2 | kLh
    { u'i',      0x0000FCEA }, // kExtendEcho2 | kLi
    { u'j',      0x0000FCEB }, // kExtendEcho2 | kLj
    { u'k',      0x0000FCEC }, // kExtendEcho2 | kLk
    { u'l',      0x0000FCED }, // kExtendEcho2 | kLl
    { u'm',      0x0000FCEE }, // kExtendEcho2 | kLm
    { u'n',      0x0000FCEF }, // kExtendEcho2 | kLsmalln
    { u'o',      0x0000FCF0 }, // kExtendEcho2 | kLo
    { u'p',      0x0000FCF1 }, // kExtendEcho2 | kLp
    { u'q',      0x0000FCF2 }, // kExtendEcho2 | kLq
    { u'r',      0x0000FCF3 }, // kExtendEcho2 | kLsmallr
    { u's',      0x0000FCF4 }, // kExtendEcho2 | kLs
    { u't',      0x0000FCF5 }, // kExtendEcho2 | kLt
    { u'u',      0x0000FCF6 }, // kExtendEcho2 | kLu
    { u'v',      0x0000FCF7 }, // kExtendEcho2 | kLv
    { u'w',      0x0000FCF8 }, // kExtendEcho2 | kLw
    { u'x',      0x0000FCF9 }, // kExtendEcho2 | kLx
    { u'y',      0x0000FCFA }, // kExtendEcho2 | kLy
    { u'z',      0x0000FCFB }, // kExtendEcho2 | kLz
    { u'{',      0x000000EC }, // kLBrace
    { u'|',      0x0000FB09 }, // kExtendEcho3 | kBar
    { u'}',      0x000000ED }, // kRBrace
    { u'~',      0x0000FB0B }, // kExtendEcho3 | kTilde
    { u'\u00A1', 0x0000FCCF }, // kExtendEcho2 | kexclamDown
    { u'\u00A8', 0x0000FCCD }, // kExtendEcho2 | kdieresis
    { u'\u00B0', 0x0000FE26 }, // kExtendEcho | kDegr
    { u'\u00B2', 0x000000BD }, // kSquare
    { u'\u00B3', 0x0000FE16 }, // kExtendEcho | kCube
    { u'\u00B4', 0x0000FCCB }, // kExtendEcho2 | kaccent
    { u'\u00BF', 0x0000FCCE }, // kExtendEcho2 | kquesDown
    { u'\u00C0', 0x0000FCA0 }, // kExtendEcho2 | kcapAGrave
    { u'\u00C1', 0x0000FC9F }, // kExtendEcho2 | kcapAAcute
    { u'\u00C2', 0x0000FCA1 }, // kExtendEcho2 | kcapACaret
    { u'\u00C4', 0x0000FCA2 }, // kExtendEcho2 | kcapADier
    { u'\u00C7', 0x0000FCC7 }, // kExtendEcho2 | kcapCCed
    { u'\u00C8', 0x0000FCA8 }, // kExtendEcho2 | kcapEGrave
    { u'\u00C9', 0x0000FCA7 }, // kExtendEcho2 | kcapEAcute
    { u'\u00CA', 0x0000FCA9 }, // kExtendEcho2 | kcapECaret
    { u'\u00CB', 0x0000FCAA }, // kExtendEcho2 | kcapEDier
    { u'\u00CC', 0x0000FCB0 }, // kExtendEcho2 | kcapIGrave
    { u'\u00CD', 0x0000FCAF }, // kExtendEcho2 | kcapIAcute
    { u'\u00CE', 0x0000FCB1 }, // kExtendEcho2 | kcapICaret
    { u'\u00CF', 0x0000FCB2 }, // kExtendEcho2 | kcapIDier
    { u'\u00D1', 0x0000FCC9 }, // kExtendEcho2 | kcapNTilde
    { u'\u00D2', 0x0000FCB8 }, // kExtendEcho2 | kcapOGrave
    { u'\u00D3', 0x0000FCB7 }, // kExtendEcho2 | kcapOAcute
    { u'\u00D4', 0x0000FCB9 }, // kExtendEcho2 | kcapOCaret
    { u'\u00D6', 0x0000FCBA }, // kExtendEcho2 | kcapODier
    { u'\u00D9', 0x0000FCC0 }, // kExtendEcho2 | kcapUGrave
    { u'\u00DA', 0x0000FCBF }, // kExtendEcho2 | kcapUAcute
    { u'\u00DB', 0x0000FCC1 }, // kExtendEcho2 | kcapUCaret
    { u'\u00DC', 0x0000FCC2 }, // kExtendEcho2 | kcapUDier
    { u'\u00DF', 0x0000FB8E }, // kExtendEcho3 | kSS
    { u'\u00E0', 0x0000FCA4 }, // kExtendEcho2 | kAGrave
    { u'\u00E1', 0x0000FCA3 }, // kExtendEcho2 | kAAcute
    { u'\u00E2', 0x0000FCA5 }, // kExtendEcho2 | kACaret
    { u'\u00E4', 0x0000FCA6 }, // kExtendEcho2 | kADier
    { u'\u00E7', 0x0000FCC8 }, // kExtendEcho2 | kCCed
    { u'\u00E8', 0x0000FCAC }, // kExtendEcho2 | kEGrave
    { u'\u00E9', 0x0000FCAB }, // kExtendEcho2 | kEAcute
    { u'\u00EA', 0x0000FCAD }, // kExtendEcho2 | kECaret
    { u'\u00EB', 0x0000FCAE }, // kExtendEcho2 | kEDier
    { u'\u00EC', 0x0000FCB4 }, // kExtendEcho2 | kIGrave
    { u'\u00ED', 0x0000FCB3 }, // kExtendEcho2 | kIAcute
    { u'\u00EE', 0x0000FCB5 }, // kExtendEcho2 | kICaret
    { u'\u00EF', 0x0000FCB6 }, // kExtendEcho2 | kIDier
    { u'\u00F1', 0x0000FCCA }, // kExtendEcho2 | kNTilde
    { u'\u00F2', 0x0000FCBC }, // kExtendEcho2 | kOGrave
    { u'\u00F3', 0x0000FCBB }, // kExtendEcho2 | kOAcute
    { u'\u00F4', 0x0000FCBD }, // kExtendEcho2 | kOCaret
    { u'\u00F6', 0x0000FCBE }, // kExtendEcho2 | kODier
    { u'\u00F9', 0x0000FCC4 }, // kExtendEcho2 | kUGrave
    { u'\u00FA', 0x0000FCC3 }, // kExtendEcho2 | kUAcute
    { u'\u00FB', 0x0000FCC5 }, // kExtendEcho2 | kUCaret
    { u'\u00FC', 0x0000FCC6 }, // kExtendEcho2 | kUDier
    { u'\u02E3', 0x0000FB8F }, // kExtendEcho3 | kSupX
    { u'\u0394', 0x0000FCD3 }, // kExtendEcho2 | kcapDelta
    { u'\u03A3', 0x0000FCDA }, // kExtendEcho2 | kcapSigma
    { u'\u03A9', 0x0000FCDE }, // kExtendEcho2 | kcapOmega
    { u'\u03B1', 0x0000FCD0 }, // kExtendEcho2 | kalpha
    { u'\u03B2', 0x0000FCD1 }, // kExtendEcho2 | kbeta
    { u'\u03B3', 0x0000FCD2 }, // kExtendEcho2 | kgamma
    { u'\u03B4', 0x0000FCD4 }, // kExtendEcho2 | kdelta
    { u'\u03B5', 0x0000FCD5 }, // kExtendEcho2 | kepsilon
    { u'\u03B8', 0x000000CC }, // kTheta
    { u'\u03BB', 0x0000FCD6 }, // kExtendEcho2 | klambda
    { u'\u03BC', 0x0000FCD7 }, // kExtendEcho2 | kmu
    { u'\u03C0', 0x000000B5 }, // kPi
    { u'\u03C1', 0x0000FCD9 }, // kExtendEcho2 | krho
    { u'\u03C3', 0x0000FCDB }, // kExtendEcho2 | ksigma
    { u'\u03C4', 0x0000FCDC }, // kExtendEcho2 | ktau
    { u'\u03C6', 0x0000FCDD }, // kExtendEcho2 | kphi
    { u'\u03C7', 0x0000FCE0 }, // kExtendEcho2 | kchi2
    { u'\u2080', 0x0000FB91 }, // kExtendEcho3 | kSub0
    { u'\u2081', 0x0000FB92 }, // kExtendEcho3 | kSub1
    { u'\u2082', 0x0000FB93 }, // kExtendEcho3 | kSub2
    { u'\u2083', 0x0000FB94 }, // kExtendEcho3 | kSub3
    { u'\u2084', 0x0000FB95 }, // kExtendEcho3 | kSub4
    { u'\u2085', 0x0000FB96 }, // kExtendEcho3 | kSub5
    { u'\u2086', 0x0000FB97 }, // kExtendEcho3 | kSub6
    { u'\u2087', 0x0000FB98 }, // kExtendEcho3 | kSub7
    { u'\u2088', 0x0000FB99 }, // kExtendEcho3 | kSub8
    { u'\u2089', 0x0000FB9A }, // kExtendEcho3 | kSub9
    { u'\u221A', 0x0000FBA4 }, // kExtendEcho3 | kSqrt2
    { u'\u2220', 0x0000FB8D }, // kExtendEcho3 | kAng
    { u'\u2260', 0x0000FE0B }, // kExtendEcho | kTNoteQ
    { u'\u2264', 0x0000FE0F }, // kExtendEcho | kTLTE
    { u'\u2265', 0x0000FE0D }, // kExtendEcho | kTGTE
};

const QHash<int, quint32> QtKeypadBridge::kKeyMap = {
    {                       Qt::Key_Escape,    0x00000040 }, // kQuit
    {                       Qt::Key_Backspace, 0x0002000A }, // kLeft, kDel
    { Qt::ControlModifier | Qt::Key_Backspace, 0x00000009 }, // kClear
    {                       Qt::Key_Return,    0x00000005 }, // kEnter
    {                       Qt::Key_Enter,     0x00000005 }, // kEnter
    {                       Qt::Key_Insert,    0x0000000B }, // kIns
    {                       Qt::Key_Delete,    0x0000000A }, // kDel
    {                       Qt::Key_Clear,     0x00000009 }, // kClear
    {                       Qt::Key_Home,      0x0000000E }, // kBOL
    {                       Qt::Key_End,       0x0000000F }, // kEOL
    {                       Qt::Key_Left,      0x00000002 }, // kLeft
    { Qt::ControlModifier | Qt::Key_Left,      0x0000000E }, // kBOL
    {                       Qt::Key_Up,        0x00000003 }, // kUp
    { Qt::ControlModifier | Qt::Key_Up,        0x00000007 }, // kAlphaUp
    {                       Qt::Key_Right,     0x00000001 }, // kRight
    { Qt::ControlModifier | Qt::Key_Right,     0x0000000F }, // kEOL
    {                       Qt::Key_Down,      0x00000004 }, // kDown
    { Qt::ControlModifier | Qt::Key_Down,      0x00000008 }, // kAlphaDown
    {                       Qt::Key_PageUp,    0x00000007 }, // kAlphaUp
    {                       Qt::Key_PageDown,  0x00000008 }, // kAlphaDown
    {                       Qt::Key_F1,        0x00000049 }, // kYequ
    { Qt::ControlModifier | Qt::Key_F1,        0x00000055 }, // kStatP
    {                       Qt::Key_F2,        0x00000048 }, // kWindow
    { Qt::ControlModifier | Qt::Key_F2,        0x0000004B }, // kTblSet
    {                       Qt::Key_F3,        0x0000002E }, // KZoom
    { Qt::ControlModifier | Qt::Key_F3,        0x00000057 }, // kFormat
    {                       Qt::Key_F4,        0x0000005A }, // KTrace
    { Qt::ControlModifier | Qt::Key_F4,        0x0000003B }, // kCalc
    {                       Qt::Key_F5,        0x00000044 }, // KGraph
    { Qt::ControlModifier | Qt::Key_F5,        0x0000004A }, // kTable
};
