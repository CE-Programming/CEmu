#ifndef KEYMAP_H
#define KEYMAP_H

//Don't use this as header file.

#include <QKeyEvent>

struct HostKey {
    Qt::Key key;
    QString name;
    bool alt;
};

#define KEY(x, y) HostKey x{Qt::Key_##y, #y, false}
#define ALT(x, y, z) HostKey x{Qt::Key_##y, z, true}

KEY(enter, Enter);
KEY(blue2nd, Shift);
KEY(greenAlpha, Control);

KEY(n0, 0);
KEY(n1, 1);
KEY(n2, 2);
KEY(n3, 3);
KEY(n4, 4);
KEY(n5, 5);
KEY(n6, 6);
KEY(n7, 7);
KEY(n8, 8);
KEY(n9, 9);

KEY(minus, Minus);
KEY(plus, Plus);
KEY(paren_left, ParenLeft);
KEY(paren_right, ParenRight);

KEY(comma, Comma);
KEY(period, Period);

HostKey none{static_cast<Qt::Key>(0), "", false};

HostKey keymap_tp[8][11] =
{
{ none, none, none, none, none, none, none, none, none, none, none },
{ none, none, none, none, none, none, none, none, none, none, none },
{ none, none, none, none, none, none, none, none, none, none, none },
{ n0, n1, n4, n7, comma, none, none, none, none, none, none },
{ period, n2, n5, n8, none, none, none, none, none, none, none },
{ none, n3, n6, n9, none, none, none, none, none, none, none },
{ none, none, none, none, none, none, none, none, none, none, none },
{ none, none, none, none, none, none, none, none, none, none, none }
};

#endif // KEYMAP_H
