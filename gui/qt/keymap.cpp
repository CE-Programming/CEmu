#include "keymap.h"

// --------------
// Common section
// --------------
HostKey knone{{static_cast<Qt::Key>(0), static_cast<Qt::Key>(0)}, "", false};


// ------------
// CEmu section
// ------------
#define KEY1(x, y) HostKey x##_cemu{{Qt::Key_##y, static_cast<Qt::Key>(0), static_cast<Qt::Key>(0)}, #y, false}
#define KEY2(x, y, z) HostKey x##_cemu{{Qt::Key_##y, Qt::Key_##z, static_cast<Qt::Key>(0)}, #y, false}
#define KEY3(x, y, z, w) HostKey x##_cemu{{Qt::Key_##y, Qt::Key_##z, Qt::Key_##w}, #y, false}

KEY2(kenter, Return, Enter);
KEY2(k2nd, Tab, Semicolon);
KEY2(kalpha, Control, Apostrophe);
KEY2(kmode, Home, Escape);
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
KEY1(kclear, Backspace);

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

const HostKey keymap_83pce_cemu[8][8] =
{
{ knone, knone, knone, knone, knone, knone, knone, knone },
{ kgraph_cemu, ktrace_cemu, kzoom_cemu, kwindow_cemu, kyequ_cemu, k2nd_cemu, kmode_cemu, kdel_cemu },
{ kon_cemu, ksto_cemu, kln_cemu, klog_cemu, kx2_cemu, kxinv_cemu, kmath_cemu, kalpha_cemu },
{ k0_cemu, k1_cemu, k4_cemu, k7_cemu, kcomma_cemu, ksin_cemu, kapps_cemu, kxton_cemu },
{ kperiod_cemu, k2_cemu, k5_cemu, k8_cemu, klparen_cemu, kcos_cemu, kprgm_cemu, kstat_cemu },
{ kneg_cemu, k3_cemu, k6_cemu, k9_cemu, krparen_cemu, ktan_cemu, kvars_cemu, knone },
{ kenter_cemu, kadd_cemu, kminus_cemu, kmlt_cemu, kdivi_cemu, kcaret_cemu, kclear_cemu, knone },
{ kd_cemu, kl_cemu, kr_cemu, ku_cemu, knone, knone, knone, knone }
};

const HostKey keymap_84pce_cemu[8][8] =
{
{ knone, knone, knone, knone, knone, knone, knone, knone },
{ kgraph_cemu, ktrace_cemu, kzoom_cemu, kwindow_cemu, kyequ_cemu, k2nd_cemu, kmode_cemu, kdel_cemu },
{ kon_cemu, ksto_cemu, kln_cemu, klog_cemu, kx2_cemu, kxinv_cemu, kmath_cemu, kalpha_cemu },
{ k0_cemu, k1_cemu, k4_cemu, k7_cemu, kcomma_cemu, ksin_cemu, kapps_cemu, kxton_cemu },
{ kperiod_cemu, k2_cemu, k5_cemu, k8_cemu, klparen_cemu, kcos_cemu, kprgm_cemu, kstat_cemu },
{ kneg_cemu, k3_cemu, k6_cemu, k9_cemu, krparen_cemu, ktan_cemu, kvars_cemu, knone },
{ kenter_cemu, kadd_cemu, kminus_cemu, kmlt_cemu, kdivi_cemu, kcaret_cemu, kclear_cemu, knone },
{ kd_cemu, kl_cemu, kr_cemu, ku_cemu, knone, knone, knone, knone }
};

#undef KEY3
#undef KEY2
#undef KEY1

// -------------
// TilEm section
// -------------
#define KEY1(x, y) HostKey x##_tilem{{Qt::Key_##y, static_cast<Qt::Key>(0), static_cast<Qt::Key>(0)}, #y, false}
#define KEY2(x, y, z) HostKey x##_tilem{{Qt::Key_##y, Qt::Key_##z, static_cast<Qt::Key>(0)}, #y, false}
#define KEY3(x, y, z, w) HostKey x##_tilem{{Qt::Key_##y, Qt::Key_##z, Qt::Key_##w}, #y, false}

KEY2(kenter, Return, Enter);
KEY2(k2nd, Tab, Semicolon);
KEY2(kalpha, Control, Apostrophe);
KEY2(kmode, Home, Escape);
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
KEY1(kclear, Backspace);

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

const HostKey keymap_83pce_tilem[8][8] =
{
{ knone, knone, knone, knone, knone, knone, knone, knone },
{ kgraph_tilem, ktrace_tilem, kzoom_tilem, kwindow_tilem, kyequ_tilem, k2nd_tilem, kmode_tilem, kdel_tilem },
{ kon_tilem, ksto_tilem, kln_tilem, klog_tilem, kx2_tilem, kxinv_tilem, kmath_tilem, kalpha_tilem },
{ k0_tilem, k1_tilem, k4_tilem, k7_tilem, kcomma_tilem, ksin_tilem, kapps_tilem, kxton_tilem },
{ kperiod_tilem, k2_tilem, k5_tilem, k8_tilem, klparen_tilem, kcos_tilem, kprgm_tilem, kstat_tilem },
{ kneg_tilem, k3_tilem, k6_tilem, k9_tilem, krparen_tilem, ktan_tilem, kvars_tilem, knone },
{ kenter_tilem, kadd_tilem, kminus_tilem, kmlt_tilem, kdivi_tilem, kcaret_tilem, kclear_tilem, knone },
{ kd_tilem, kl_tilem, kr_tilem, ku_tilem, knone, knone, knone, knone }
};

const HostKey keymap_84pce_tilem[8][8] =
{
{ knone, knone, knone, knone, knone, knone, knone, knone },
{ kgraph_tilem, ktrace_tilem, kzoom_tilem, kwindow_tilem, kyequ_tilem, k2nd_tilem, kmode_tilem, kdel_tilem },
{ kon_tilem, ksto_tilem, kln_tilem, klog_tilem, kx2_tilem, kxinv_tilem, kmath_tilem, kalpha_tilem },
{ k0_tilem, k1_tilem, k4_tilem, k7_tilem, kcomma_tilem, ksin_tilem, kapps_tilem, kxton_tilem },
{ kperiod_tilem, k2_tilem, k5_tilem, k8_tilem, klparen_tilem, kcos_tilem, kprgm_tilem, kstat_tilem },
{ kneg_tilem, k3_tilem, k6_tilem, k9_tilem, krparen_tilem, ktan_tilem, kvars_tilem, knone },
{ kenter_tilem, kadd_tilem, kminus_tilem, kmlt_tilem, kdivi_tilem, kcaret_tilem, kclear_tilem, knone },
{ kd_tilem, kl_tilem, kr_tilem, ku_tilem, knone, knone, knone, knone }
};

#undef KEY3
#undef KEY2
#undef KEY1

// -----------------
// WabbitEmu section
// -----------------
#define KEY1(x, y) HostKey x##_wabbitemu{{Qt::Key_##y, static_cast<Qt::Key>(0), static_cast<Qt::Key>(0)}, #y, false}
#define KEY2(x, y, z) HostKey x##_wabbitemu{{Qt::Key_##y, Qt::Key_##z, static_cast<Qt::Key>(0)}, #y, false}
#define KEY3(x, y, z, w) HostKey x##_wabbitemu{{Qt::Key_##y, Qt::Key_##z, Qt::Key_##w}, #y, false}

KEY2(kenter, Return, Enter);
KEY2(k2nd, Tab, Semicolon);
KEY2(kalpha, Control, Apostrophe);
KEY2(kmode, Home, Escape);
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
KEY1(kclear, Backspace);

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

const HostKey keymap_83pce_wabbitemu[8][8] =
{
{ knone, knone, knone, knone, knone, knone, knone, knone },
{ kgraph_wabbitemu, ktrace_wabbitemu, kzoom_wabbitemu, kwindow_wabbitemu, kyequ_wabbitemu, k2nd_wabbitemu, kmode_wabbitemu, kdel_wabbitemu },
{ kon_wabbitemu, ksto_wabbitemu, kln_wabbitemu, klog_wabbitemu, kx2_wabbitemu, kxinv_wabbitemu, kmath_wabbitemu, kalpha_wabbitemu },
{ k0_wabbitemu, k1_wabbitemu, k4_wabbitemu, k7_wabbitemu, kcomma_wabbitemu, ksin_wabbitemu, kapps_wabbitemu, kxton_wabbitemu },
{ kperiod_wabbitemu, k2_wabbitemu, k5_wabbitemu, k8_wabbitemu, klparen_wabbitemu, kcos_wabbitemu, kprgm_wabbitemu, kstat_wabbitemu },
{ kneg_wabbitemu, k3_wabbitemu, k6_wabbitemu, k9_wabbitemu, krparen_wabbitemu, ktan_wabbitemu, kvars_wabbitemu, knone },
{ kenter_wabbitemu, kadd_wabbitemu, kminus_wabbitemu, kmlt_wabbitemu, kdivi_wabbitemu, kcaret_wabbitemu, kclear_wabbitemu, knone },
{ kd_wabbitemu, kl_wabbitemu, kr_wabbitemu, ku_wabbitemu, knone, knone, knone, knone }
};

const HostKey keymap_84pce_wabbitemu[8][8] =
{
{ knone, knone, knone, knone, knone, knone, knone, knone },
{ kgraph_wabbitemu, ktrace_wabbitemu, kzoom_wabbitemu, kwindow_wabbitemu, kyequ_wabbitemu, k2nd_wabbitemu, kmode_wabbitemu, kdel_wabbitemu },
{ kon_wabbitemu, ksto_wabbitemu, kln_wabbitemu, klog_wabbitemu, kx2_wabbitemu, kxinv_wabbitemu, kmath_wabbitemu, kalpha_wabbitemu },
{ k0_wabbitemu, k1_wabbitemu, k4_wabbitemu, k7_wabbitemu, kcomma_wabbitemu, ksin_wabbitemu, kapps_wabbitemu, kxton_wabbitemu },
{ kperiod_wabbitemu, k2_wabbitemu, k5_wabbitemu, k8_wabbitemu, klparen_wabbitemu, kcos_wabbitemu, kprgm_wabbitemu, kstat_wabbitemu },
{ kneg_wabbitemu, k3_wabbitemu, k6_wabbitemu, k9_wabbitemu, krparen_wabbitemu, ktan_wabbitemu, kvars_wabbitemu, knone },
{ kenter_wabbitemu, kadd_wabbitemu, kminus_wabbitemu, kmlt_wabbitemu, kdivi_wabbitemu, kcaret_wabbitemu, kclear_wabbitemu, knone },
{ kd_wabbitemu, kl_wabbitemu, kr_wabbitemu, ku_wabbitemu, knone, knone, knone, knone }
};

#undef KEY3
#undef KEY2
#undef KEY1
