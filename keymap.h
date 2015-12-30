#ifndef KEYMAP_H
#define KEYMAP_H

// Don't use this as header file.

#include <QtGui/QKeyEvent>

struct HostKey {
    Qt::Key key[3];
    QString name;
    bool alt;
};

#define KEY1(x, y) HostKey x{{Qt::Key_##y, static_cast<Qt::Key>(0), static_cast<Qt::Key>(0)}, #y, false}
#define KEY2(x, y, z) HostKey x{{Qt::Key_##y, Qt::Key_##z, static_cast<Qt::Key>(0)}, #y, false}
#define KEY3(x, y, z, w) HostKey x{{Qt::Key_##y, Qt::Key_##z, Qt::Key_##w}, #y, false}

KEY2(kenter, Return, Enter);
KEY2(k2nd, Tab, Semicolon);
KEY2(kalpha, Control, Apostrophe);
KEY2(kclear, Backspace, Escape);
KEY2(kneg, AsciiTilde, Question);
KEY2(kadd, Plus, QuoteDbl);
KEY3(kminus, Minus, BracketRight, W);
KEY3(kmlt, Asterisk, BracketLeft, R);
KEY2(kdivi, Slash, M);
KEY2(ksto, Greater, X);
KEY2(kln, QuoteLeft, S);
KEY2(klog, Exclam, N);
KEY2(kx2, At, I);
KEY2(kxinv, Backslash, D);
KEY2(kmath, Equal, A);
KEY2(kapps, PageUp, B);
KEY2(kprgm, PageDown, C);
KEY1(kstat, End);
KEY1(kmode, Home);

KEY2(kvars, Less, Bar);
KEY2(kxton, Semicolon, Underscore);

KEY2(ksin, Dollar, E);
KEY2(kcos, Percent, F);
KEY2(ktan, Ampersand, G);
KEY2(kdel, Delete, Insert);

KEY1(ku, Up);
KEY1(kd, Down);
KEY1(kr, Right);
KEY1(kl, Left);
KEY2(kcaret, AsciiCircum, H);

KEY2(k0, 0, Space);
KEY2(k1, 1, Y);
KEY2(k2, 2, Z);
KEY2(k3, 3, NumberSign);
KEY2(k4, 4, T);
KEY2(k5, 5, U);
KEY2(k6, 6, V);
KEY2(k7, 7, O);
KEY2(k8, 8, P);
KEY2(k9, 9, Q);

KEY3(klparen, ParenLeft, BraceLeft, K);
KEY3(krparen, ParenRight, BraceRight, L);

KEY1(kyequ, F1);
KEY1(kwindow, F2);
KEY1(kzoom, F3);
KEY1(ktrace, F4);
KEY1(kgraph, F5);
KEY1(kon, F12);
KEY2(kcomma, Comma, J);
KEY2(kperiod, Period, Colon);

HostKey knone{{static_cast<Qt::Key>(0), static_cast<Qt::Key>(0)}, "", false};

HostKey keymap_tp[8][8] =
{
{ knone, knone, knone, knone, knone, knone, knone, knone },
{ kgraph, ktrace, kzoom, kwindow, kyequ, k2nd, kmode, kdel },
{ kon, ksto, kln, klog, kx2, kxinv, kmath, kalpha },
{ k0, k1, k4, k7, kcomma, ksin, kapps, kxton },
{ kperiod, k2, k5, k8, klparen, kcos, kprgm, kstat },
{ kneg, k3, k6, k9, krparen, ktan, kvars, knone },
{ kenter, kadd, kminus, kmlt, kdivi, kcaret, kclear, knone },
{ kd, kl, kr, ku, knone, knone, knone, knone }
};

#endif
