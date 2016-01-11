#ifndef KEYCODE_H
#define KEYCODE_H

struct KeyCode {
    KeyCode() : m_code(~0) {}
    KeyCode(unsigned code) : m_code(code) {}
    KeyCode(unsigned row, unsigned col) : m_code(((row & 7) << 3) |
                                                 ((col & 7) << 0)) {}

    bool valid() const { return m_code >> 6 == 0; }
    unsigned char code() const { return m_code; }
    unsigned char row() const { return (m_code >> 3) & 7; }
    unsigned char col() const { return (m_code >> 0) & 7; }

    bool operator==(KeyCode other) const { return code() == other.code(); }
    bool operator!=(KeyCode other) const { return !(*this == other); }

private:
    unsigned char m_code;
};

#endif
