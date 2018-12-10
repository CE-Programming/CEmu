#ifndef KEYCODE_H
#define KEYCODE_H

#include <QtCore/QHash>

struct KeyCode {
    KeyCode() : mCode(~0) {}
    KeyCode(unsigned code) : mCode(code) {}
    KeyCode(unsigned row, unsigned col) : mCode(((row & 7) << 3) |
                                                 ((col & 7) << 0)) {}

    bool valid() const { return mCode >> 6 == 0; }
    unsigned char code() const { return mCode; }
    unsigned char row() const { return (mCode >> 3) & 7; }
    unsigned char col() const { return (mCode >> 0) & 7; }

    bool operator==(KeyCode other) const { return code() == other.code(); }
    bool operator!=(KeyCode other) const { return !(*this == other); }
    friend uint qHash(KeyCode code, uint seed = 0) {
        return qHash(code.code(), seed);
    }

private:
    unsigned char mCode;
};

#endif
