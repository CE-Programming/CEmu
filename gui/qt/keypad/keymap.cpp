#include "keymap.h"

// --------------
// Common section
// --------------
#define KEYMAP_83PCE const HostKey *KEY(eymap_83pce)[8][8] = {                                      \
        { &none, &none, &none, &none, &none, &none, &none, &none },                                 \
        { KEY(graph), KEY(trace), KEY(zoom), KEY(wind), KEY(yequ), KEY(2nd), KEY(mode), KEY(del) }, \
        { KEY(on), KEY(sto), KEY(ln), KEY(log), KEY(sq), KEY(inv), KEY(math), KEY(alpha) },         \
        { KEY(0), KEY(1), KEY(4), KEY(7), KEY(comma), KEY(sin), KEY(apps), KEY(xton) },             \
        { KEY(dot), KEY(2), KEY(5), KEY(8), KEY(lpar), KEY(cos), KEY(prgm), KEY(stat) },            \
        { KEY(neg), KEY(3), KEY(6), KEY(9), KEY(rpar), KEY(tan), KEY(vars), &none },                \
        { KEY(enter), KEY(add), KEY(sub), KEY(mul), KEY(div), KEY(pow), KEY(clr), &none },          \
        { KEY(down), KEY(left), KEY(right), KEY(up), &none, &none, &none, &none }                   \
    }
#define KEYMAP_84PCE const HostKey *KEY(eymap_84pce)[8][8] = {                                      \
        { &none, &none, &none, &none, &none, &none, &none, &none },                                 \
        { KEY(graph), KEY(trace), KEY(zoom), KEY(wind), KEY(yequ), KEY(2nd), KEY(mode), KEY(del) }, \
        { KEY(on), KEY(sto), KEY(ln), KEY(log), KEY(sq), KEY(inv), KEY(math), KEY(alpha) },         \
        { KEY(0), KEY(1), KEY(4), KEY(7), KEY(comma), KEY(sin), KEY(apps), KEY(xton) },             \
        { KEY(dot), KEY(2), KEY(5), KEY(8), KEY(lpar), KEY(cos), KEY(prgm), KEY(stat) },            \
        { KEY(neg), KEY(3), KEY(6), KEY(9), KEY(rpar), KEY(tan), KEY(vars), &none },                \
        { KEY(enter), KEY(add), KEY(sub), KEY(mul), KEY(div), KEY(pow), KEY(clr), &none },          \
        { KEY(down), KEY(left), KEY(right), KEY(up), &none, &none, &none, &none }                   \
    }

#define HK(code, nativeCode, nativeMask, modifier, mask) { Qt::Key_##code, nativeCode, nativeMask, Qt::modifier##Modifier, Qt::mask##Modifier, QStringLiteral(#code) }
#define NRM(code) HK(code, 0, 0, No, No)
#define NAT(code, nativeCode, nativeMask) HK(code, nativeCode, nativeMask, No, No)
#define MOD(code, modifier, mask) HK(code, 0, 0, modifier, mask)

static const HostKey none = { static_cast<Qt::Key>(0), 0, 0, Qt::NoModifier, Qt::NoModifier, {} };

// ------------
// CEmu section
// ------------
#define KEY(key) cemu_k##key

static const HostKey KEY(enter)[] = { NRM(Return), NRM(Enter), none };
static const HostKey KEY(2nd)[] = { NRM(Tab), NRM(Semicolon), none };
static const HostKey KEY(alpha)[] = { NRM(Control), NRM(Apostrophe), none };
static const HostKey KEY(sto)[] = { NRM(Greater), NRM(X), none };
static const HostKey KEY(clr)[] = { NRM(Escape), none };
static const HostKey KEY(del)[] = { NRM(Delete), NRM(Insert), none };

static const HostKey KEY(math)[] = { NRM(Equal), NRM(A), none };
static const HostKey KEY(apps)[] = { NRM(PageUp), NRM(B), none };
static const HostKey KEY(prgm)[] = { NRM(PageDown), NRM(C), none };
static const HostKey KEY(vars)[] = { NRM(Less), NRM(Bar), none };
static const HostKey KEY(stat)[] = { NRM(End), none };
static const HostKey KEY(mode)[] = { NRM(Home), NRM(Backspace), none };
static const HostKey KEY(xton)[] = { NRM(Underscore), none };

static const HostKey KEY(neg)[] = { NRM(AsciiTilde), NRM(Question), none };
static const HostKey KEY(add)[] = { NRM(Plus), NRM(QuoteDbl), none };
static const HostKey KEY(sub)[] = { NRM(Minus), NRM(BracketRight), NRM(W), none };
static const HostKey KEY(mul)[] = { NRM(Asterisk), NRM(BracketLeft), NRM(R), none };
static const HostKey KEY(div)[] = { NRM(Slash), NRM(M), none };
static const HostKey KEY(pow)[] = { NRM(AsciiCircum), NRM(H), none };

static const HostKey KEY(sq)[] = { NRM(At), NRM(I), none };
static const HostKey KEY(inv)[] = { NRM(Backslash), NRM(D), none };
static const HostKey KEY(ln)[] = { NRM(QuoteLeft), NRM(S), none };
static const HostKey KEY(log)[] = { NRM(Exclam), NRM(N), none };
static const HostKey KEY(sin)[] = { NRM(Dollar), NRM(E), none };
static const HostKey KEY(cos)[] = { NRM(Percent), NRM(F), none };
static const HostKey KEY(tan)[] = { NRM(Ampersand), NRM(G), none };

static const HostKey KEY(down)[] = { NRM(Down), none };
static const HostKey KEY(left)[] = { NRM(Left), none };
static const HostKey KEY(right)[] = { NRM(Right), none };
static const HostKey KEY(up)[] = { NRM(Up), none };

static const HostKey KEY(0)[] = { NRM(0), NRM(Space), none };
static const HostKey KEY(1)[] = { NRM(1), NRM(Y), none };
static const HostKey KEY(2)[] = { NRM(2), NRM(Z), none };
static const HostKey KEY(3)[] = { NRM(3), NRM(NumberSign), none };
static const HostKey KEY(4)[] = { NRM(4), NRM(T), none };
static const HostKey KEY(5)[] = { NRM(5), NRM(U), none };
static const HostKey KEY(6)[] = { NRM(6), NRM(V), none };
static const HostKey KEY(7)[] = { NRM(7), NRM(O), none };
static const HostKey KEY(8)[] = { NRM(8), NRM(P), none };
static const HostKey KEY(9)[] = { NRM(9), NRM(Q), none };
static const HostKey KEY(dot)[] = { NRM(Period), NRM(Colon), none };
static const HostKey KEY(comma)[] = { NRM(Comma), NRM(J), none };
static const HostKey KEY(lpar)[] = { NRM(ParenLeft), NRM(BraceLeft), NRM(K), none };
static const HostKey KEY(rpar)[] = { NRM(ParenRight), NRM(BraceRight), NRM(L), none };

static const HostKey KEY(yequ)[] = { NRM(F1), none };
static const HostKey KEY(wind)[] = { NRM(F2), none };
static const HostKey KEY(zoom)[] = { NRM(F3), none };
static const HostKey KEY(trace)[] = { NRM(F4), none };
static const HostKey KEY(graph)[] = { NRM(F5), none };
static const HostKey KEY(on)[] = { NRM(F12), none };

KEYMAP_83PCE;
KEYMAP_84PCE;

#undef KEY

// -------------
// TilEm section
// -------------
#define KEY(key) tilem_k##key

static const HostKey KEY(enter)[] = { NRM(Return), NRM(Enter), none };
static const HostKey KEY(2nd)[] = { NRM(Tab), none };
static const HostKey KEY(alpha)[] = { NRM(Apostrophe), none };
static const HostKey KEY(sto)[] = { NRM(Greater), NRM(Less), MOD(X, Shift, Shift), none };
static const HostKey KEY(clr)[] = { NRM(Escape), none };
static const HostKey KEY(del)[] = { NRM(Delete), NRM(Backspace), NRM(Insert), none };

static const HostKey KEY(math)[] = { NRM(F6), MOD(M, No, Shift), MOD(A, Shift, Shift), none };
static const HostKey KEY(apps)[] = { NRM(F7), MOD(A, No, Shift), MOD(B, Shift, Shift), none };
static const HostKey KEY(prgm)[] = { NRM(F8), MOD(P, No, Shift), MOD(C, Shift, Shift), none };
static const HostKey KEY(vars)[] = { NRM(F9), MOD(R, No, Shift), NRM(Less), NRM(Bar), none };
static const HostKey KEY(stat)[] = { NRM(F10), MOD(Q, No, Shift), none };
static const HostKey KEY(mode)[] = { NRM(F11), MOD(D, No, Shift), none };
static const HostKey KEY(xton)[] = { MOD(X, No, Shift), none };

static const HostKey KEY(neg)[] = { NRM(AsciiTilde), NRM(Dollar), NRM(Question), none };
static const HostKey KEY(add)[] = { NRM(Plus), NRM(QuoteDbl), none };
static const HostKey KEY(sub)[] = { NRM(Minus), NRM(BracketRight), MOD(W, Shift, Shift), none };
static const HostKey KEY(mul)[] = { NRM(Asterisk), NRM(BracketLeft), MOD(R, Shift, Shift), none };
static const HostKey KEY(div)[] = { NRM(Slash), MOD(E, No, Shift), MOD(M, Shift, Shift), none };
static const HostKey KEY(pow)[] = { NRM(AsciiCircum), NRM(NumberSign), MOD(H, Shift, Shift), none };

static const HostKey KEY(sq)[] = { MOD(2, Control, Control), MOD(I, Shift, Shift), none };
static const HostKey KEY(inv)[] = { MOD(1, Control, Control), NRM(Backslash), MOD(D, Shift, Shift), none };
static const HostKey KEY(ln)[] = { MOD(L, No, Shift), MOD(S, Shift, Shift), none };
static const HostKey KEY(log)[] = { MOD(O, No, Shift), MOD(N, Shift, Shift), none };
static const HostKey KEY(sin)[] = { MOD(S, No, Shift), MOD(E, Shift, Shift), none };
static const HostKey KEY(cos)[] = { MOD(C, No, Shift), MOD(F, Shift, Shift), none };
static const HostKey KEY(tan)[] = { MOD(T, No, Shift), MOD(G, Shift, Shift), none };

static const HostKey KEY(down)[] = { NRM(Down), NRM(PageDown), none };
static const HostKey KEY(left)[] = { NRM(Left), NRM(Home), none };
static const HostKey KEY(right)[] = { NRM(Right), NRM(End), none };
static const HostKey KEY(up)[] = { NRM(Up), NRM(PageUp), none };

static const HostKey KEY(0)[] = { MOD(0, No, Control), NRM(Space), none };
static const HostKey KEY(1)[] = { MOD(1, No, Control), MOD(Y, Shift, Shift), none };
static const HostKey KEY(2)[] = { MOD(2, No, Control), MOD(Z, Shift, Shift), none };
static const HostKey KEY(3)[] = { MOD(3, No, Control), NRM(At), none };
static const HostKey KEY(4)[] = { MOD(4, No, Control), MOD(T, Shift, Shift), none };
static const HostKey KEY(5)[] = { MOD(5, No, Control), MOD(U, Shift, Shift), none };
static const HostKey KEY(6)[] = { MOD(6, No, Control), MOD(V, Shift, Shift), none };
static const HostKey KEY(7)[] = { MOD(7, No, Control), MOD(U, No, Shift), MOD(O, Shift, Shift), none };
static const HostKey KEY(8)[] = { MOD(8, No, Control), MOD(V, No, Shift), MOD(P, Shift, Shift), none };
static const HostKey KEY(9)[] = { MOD(9, No, Control), MOD(W, No, Shift), MOD(Q, Shift, Shift), none };
static const HostKey KEY(dot)[] = { NRM(Period), MOD(I, No, Shift), NRM(Colon), none };
static const HostKey KEY(comma)[] = { NRM(Comma), NRM(Ampersand), MOD(J, Shift, Shift), none };
static const HostKey KEY(lpar)[] = { NRM(ParenLeft), NRM(BraceLeft), MOD(K, Shift, Shift), none };
static const HostKey KEY(rpar)[] = { NRM(ParenRight), NRM(BraceRight), MOD(L, Shift, Shift), none };

static const HostKey KEY(yequ)[] = { NRM(F1), MOD(Y, No, Shift), none };
static const HostKey KEY(wind)[] = { NRM(F2), MOD(N, No, Shift), none };
static const HostKey KEY(zoom)[] = { NRM(F3), MOD(Z, No, Shift), none };
static const HostKey KEY(trace)[] = { NRM(F4), MOD(H, No, Shift), none };
static const HostKey KEY(graph)[] = { NRM(F5), MOD(G, No, Shift), none };
static const HostKey KEY(on)[] = { NRM(F12), MOD(F, No, Shift), none };

KEYMAP_83PCE;
KEYMAP_84PCE;

#undef KEY

// -----------------
// Wabbitemu section
// -----------------
#define KEY(key) wabbitemu_k##key

static const HostKey KEY(enter)[] = { NRM(Return), NRM(Enter), none };
static const HostKey KEY(2nd)[] = { NAT(Shift, 0, 4), none };
static const HostKey KEY(alpha)[] = { NRM(Control), none };
static const HostKey KEY(sto)[] = { NRM(X), none };
static const HostKey KEY(clr)[] = { NAT(Shift, 4, 4), none };
static const HostKey KEY(del)[] = { MOD(Delete, No, Keypad), none };

static const HostKey KEY(math)[] = { NRM(Home), NRM(A), none };
static const HostKey KEY(apps)[] = { NRM(PageUp), NRM(B), none };
static const HostKey KEY(prgm)[] = { NRM(PageDown), NRM(C), none };
static const HostKey KEY(vars)[] = { NRM(Insert), none };
static const HostKey KEY(stat)[] = { NRM(End), none };
static const HostKey KEY(mode)[] = { NRM(Escape), none };
static const HostKey KEY(xton)[] = { NRM(Equal), none };

static const HostKey KEY(neg)[] = { MOD(Minus, No, Keypad), MOD(Slash, No, Keypad), none };
static const HostKey KEY(add)[] = { MOD(Plus, Keypad, Keypad), none };
static const HostKey KEY(sub)[] = { MOD(Minus, Keypad, Keypad), NRM(W), none };
static const HostKey KEY(mul)[] = { MOD(Asterisk, Keypad, Keypad), NRM(R), none };
static const HostKey KEY(div)[] = { MOD(Slash, Keypad, Keypad), NRM(M), none };
static const HostKey KEY(pow)[] = { NRM(H), none };

static const HostKey KEY(sq)[] = { NRM(I), none };
static const HostKey KEY(inv)[] = { NRM(D), none };
static const HostKey KEY(ln)[] = { NRM(S), none };
static const HostKey KEY(log)[] = { NRM(N), none };
static const HostKey KEY(sin)[] = { NRM(E), none };
static const HostKey KEY(cos)[] = { NRM(F), none };
static const HostKey KEY(tan)[] = { NRM(G), none };

static const HostKey KEY(down)[] = { NRM(Down), none };
static const HostKey KEY(left)[] = { NRM(Left), none };
static const HostKey KEY(right)[] = { NRM(Right), none };
static const HostKey KEY(up)[] = { NRM(Up), none };

static const HostKey KEY(0)[] = { NRM(0), NRM(Space), none };
static const HostKey KEY(1)[] = { NRM(1), NRM(Y), none };
static const HostKey KEY(2)[] = { NRM(2), NRM(Z), none };
static const HostKey KEY(3)[] = { NRM(3), none };
static const HostKey KEY(4)[] = { NRM(4), NRM(T), none };
static const HostKey KEY(5)[] = { NRM(5), NRM(U), none };
static const HostKey KEY(6)[] = { NRM(6), NRM(V), none };
static const HostKey KEY(7)[] = { NRM(7), NRM(O), none };
static const HostKey KEY(8)[] = { NRM(8), NRM(P), none };
static const HostKey KEY(9)[] = { NRM(9), NRM(Q), none };
static const HostKey KEY(dot)[] = { NRM(Period), MOD(Delete, Keypad, Keypad), none };
static const HostKey KEY(comma)[] = { NRM(Comma), NRM(J), none };
static const HostKey KEY(lpar)[] = { NRM(BracketLeft), NRM(K), none };
static const HostKey KEY(rpar)[] = { NRM(BracketRight), NRM(L), none };

static const HostKey KEY(yequ)[] = { NRM(F1), none };
static const HostKey KEY(wind)[] = { NRM(F2), none };
static const HostKey KEY(zoom)[] = { NRM(F3), none };
static const HostKey KEY(trace)[] = { NRM(F4), none };
static const HostKey KEY(graph)[] = { NRM(F5), none };
static const HostKey KEY(on)[] = { NRM(F12), none };

KEYMAP_83PCE;
KEYMAP_84PCE;

#undef KEY

// ----------------
// jsTIfied section
// ----------------
#define KEY(key) jstified_k##key

static const HostKey KEY(enter)[] = { NRM(Return), NRM(Enter), none };
static const HostKey KEY(2nd)[] = { NRM(Shift), NRM(AsciiTilde), none };
static const HostKey KEY(alpha)[] = { NRM(Control), none };
static const HostKey KEY(sto)[] = { NRM(X), none };
static const HostKey KEY(clr)[] = { MOD(Slash, No, Keypad), none };
static const HostKey KEY(del)[] = { NRM(Delete), none };

static const HostKey KEY(math)[] = { NRM(Home), NRM(A), none };
static const HostKey KEY(apps)[] = { NRM(PageUp), NRM(B), none };
static const HostKey KEY(prgm)[] = { NRM(PageDown), NRM(C), none };
static const HostKey KEY(vars)[] = { NRM(Insert), none };
static const HostKey KEY(stat)[] = { NRM(End), none };
static const HostKey KEY(mode)[] = { NRM(Escape), none };
static const HostKey KEY(xton)[] = { NRM(Equal), none };

static const HostKey KEY(neg)[] = { MOD(Minus, No, Keypad), none };
static const HostKey KEY(add)[] = { MOD(Plus, Keypad, Keypad), none };
static const HostKey KEY(sub)[] = { MOD(Minus, Keypad, Keypad), NRM(W), none };
static const HostKey KEY(mul)[] = { MOD(Asterisk, Keypad, Keypad), NRM(R), none };
static const HostKey KEY(div)[] = { MOD(Slash, Keypad, Keypad), NRM(M), none };
static const HostKey KEY(pow)[] = { NRM(H), none };

static const HostKey KEY(sq)[] = { NRM(I), none };
static const HostKey KEY(inv)[] = { NRM(D), none };
static const HostKey KEY(ln)[] = { NRM(S), none };
static const HostKey KEY(log)[] = { NRM(N), none };
static const HostKey KEY(sin)[] = { NRM(E), none };
static const HostKey KEY(cos)[] = { NRM(F), none };
static const HostKey KEY(tan)[] = { NRM(G), none };

static const HostKey KEY(down)[] = { NRM(Down), none };
static const HostKey KEY(left)[] = { NRM(Left), none };
static const HostKey KEY(right)[] = { NRM(Right), none };
static const HostKey KEY(up)[] = { NRM(Up), none };

static const HostKey KEY(0)[] = { NRM(0), NRM(Space), none };
static const HostKey KEY(1)[] = { NRM(1), NRM(Y), none };
static const HostKey KEY(2)[] = { NRM(2), NRM(Z), none };
static const HostKey KEY(3)[] = { NRM(3), none };
static const HostKey KEY(4)[] = { NRM(4), NRM(T), none };
static const HostKey KEY(5)[] = { NRM(5), NRM(U), none };
static const HostKey KEY(6)[] = { NRM(6), NRM(V), none };
static const HostKey KEY(7)[] = { NRM(7), NRM(O), none };
static const HostKey KEY(8)[] = { NRM(8), NRM(P), none };
static const HostKey KEY(9)[] = { NRM(9), NRM(Q), none };
static const HostKey KEY(dot)[] = { NRM(Period), none };
static const HostKey KEY(comma)[] = { NRM(Comma), NRM(J), none };
static const HostKey KEY(lpar)[] = { NRM(BracketLeft), NRM(K), none };
static const HostKey KEY(rpar)[] = { NRM(BracketRight), NRM(L), none };

static const HostKey KEY(yequ)[] = { NRM(F1), none };
static const HostKey KEY(wind)[] = { NRM(F2), none };
static const HostKey KEY(zoom)[] = { NRM(F3), none };
static const HostKey KEY(trace)[] = { NRM(F4), none };
static const HostKey KEY(graph)[] = { NRM(F5), none };
static const HostKey KEY(on)[] = { NRM(F12), none };

KEYMAP_83PCE;
KEYMAP_84PCE;

#undef KEY
