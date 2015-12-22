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
KEY(blue2nd, Space);
KEY(alpha, Control);
KEY(clear, Escape);
KEY(add, Plus);
KEY(minus, Minus);
KEY(mlt, Asterisk);
KEY(divi, Slash);
KEY(sto, X);
KEY(ln, S);
KEY(klog, N);
KEY(x2, I);
KEY(xinv, D);
KEY(math, A);
KEY(kapps, PageUp);
KEY(kprgm, PageDown);
KEY(kstat, End);
KEY(kmode, Home);

KEY(kvars, Less);
KEY(kxton, Greater);

KEY(ksin, E);
KEY(kcos, F);
KEY(ktan, G);
KEY(del, Delete);

KEY(ku, Up);
KEY(kd, Down);
KEY(kr, Right);
KEY(kl, Left);
KEY(caret, AsciiCircum);

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

KEY(paren_left, ParenLeft);
KEY(paren_right, ParenRight);

KEY(yequ, F1);
KEY(window, F2);
KEY(zoom, F3);
KEY(trace, F4);
KEY(graph, F5);
KEY(comma, Comma);
KEY(period, Period);

HostKey none{static_cast<Qt::Key>(0), "", false};

HostKey keymap_tp[8][8] =
{
{ none, none, none, none, none, none, none, none },
{ graph, trace, zoom, window, yequ, blue2nd, kmode, del },
{ none, sto, ln, klog, x2, xinv, math, alpha },
{ n0, n1, n4, n7, comma, ksin, kapps, kxton },
{ period, n2, n5, n8, paren_left, kcos, kprgm, kstat },
{ none, n3, n6, n9, paren_right, ktan, kvars, none },
{ enter, add, minus, mlt, divi, caret, clear, none },
{ kd, kl, kr, ku, none, none, none, none }
};

#endif // KEYMAP_H
