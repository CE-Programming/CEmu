#include <QtWidgets/QApplication>
#include <QtGui/QPaintEvent>
#include <QtGui/QScreen>
#include <QtGui/QFontDatabase>

#include "keypadwidget.h"
#include "graphkey.h"
#include "secondkey.h"
#include "alphakey.h"
#include "operkey.h"
#include "otherkey.h"
#include "numkey.h"
#include "arrowkey.h"

#include "../../../core/asic.h"
#include "../../../core/keypad.h"

const QRect KeypadWidget::sBaseRect{{}, QSize{162, 235}};

void KeypadWidget::addKey(Key *key) {
    const KeyCode code = key->keycode();
    unsigned char row = code.row();
    unsigned char col = code.col();
    delete mKeys[row][col];
    mKeys[row][col] = key;
}

unsigned KeypadWidget::getCurrColor(void) {
    return color;
}

void KeypadWidget::setType(bool is83, unsigned int color_scheme) {
    color = color_scheme;

    cNum   = QColor::fromRgb(0xeeeeee);
    cText  = cNum;
    cOther = QColor::fromRgb(0x1d1d1d);
    cGraph = QColor::fromRgb(0xeeeeee);

    switch(color_scheme) {
        default:
        case KEYPAD_BLACK:
            cCenter = QColor::fromRgb(0x191919);
            cSides  = QColor::fromRgb(0x3b3b3b);
            break;
        case KEYPAD_WHITE:
            cCenter = QColor::fromRgb(0xe8e8e8);
            cSides  = QColor::fromRgb(0xc4c4c4);
            cNum    = QColor::fromRgb(0x707880);
            cText   = QColor::fromRgb(0x222222);
            cOther  = QColor::fromRgb(0xc0c0c0);
            break;
        case KEYPAD_TRUE_BLUE:
            cCenter = QColor::fromRgb(0x385E9D);
            cSides  = cCenter.lighter(130);
            cNum    = QColor::fromRgb(0xdedede);
            cOther  = QColor::fromRgb(0x274F91);
            break;
        case KEYPAD_DENIM:
            cCenter = QColor::fromRgb(0x003C71);
            cSides  = cCenter.lighter(130);
            cOther  = QColor::fromRgb(0x013766);
            break;
        case KEYPAD_SILVER:
            cCenter = QColor::fromRgb(0x7C878E);
            cSides  = cCenter.lighter(130);
            cOther  = QColor::fromRgb(0x191919);
            cGraph  = QColor::fromRgb(0xD0D3D4);
            break;
        case KEYPAD_PINK:
            cCenter = QColor::fromRgb(0xDF1995);
            cSides  = cCenter.lighter(130);
            cOther  = QColor::fromRgb(0xAA0061);
            break;
        case KEYPAD_PLUM:
            cCenter = QColor::fromRgb(0x830065);
            cSides  = cCenter.lighter(130);
            cOther  = QColor::fromRgb(0x5E2751);
            break;
        case KEYPAD_RED:
            cCenter = QColor::fromRgb(0xAB2328);
            cSides  = cCenter.lighter(130);
            cOther  = QColor::fromRgb(0x8A2A2B);
            break;
        case KEYPAD_LIGHTNING:
            cCenter = QColor::fromRgb(0x0077C8);
            cSides  = cCenter.lighter(130);
            cOther  = QColor::fromRgb(0x0077C8);
            break;
        case KEYPAD_GOLDEN:
            cCenter = QColor::fromRgb(0xD8D3B6);
            cSides  = cCenter.lighter(130);
            cOther  = QColor::fromRgb(0xD8D3B6);
            break;
        case KEYPAD_SPACEGREY:
            cCenter = QColor::fromRgb(0xDBDBDB);
            cSides  = cCenter.darker(130);
            cOther  = QColor::fromRgb(53, 53, 53);
            cGraph  = QColor::fromRgb(0xD0D3D4);
            break;
        case KEYPAD_CORAL:
            cCenter = QColor::fromRgb(0xFD6D99);
            cSides  = cCenter.lighter(120);
            cOther  = QColor::fromRgb(53, 53, 53);
            cGraph  = QColor::fromRgb(0xD0D3D4);
            break;
        case KEYPAD_MINT:
            cCenter = QColor::fromRgb(0xD2EBE8);
            cSides  = cCenter.darker(115);
            cOther  = QColor::fromRgb(53, 53, 53);
            cGraph  = QColor::fromRgb(0xD0D3D4);
            break;
    }

    mBackground = {sBaseRect.topLeft(), sBaseRect.topRight()};
    mBackground.setColorAt(0.00, cSides);
    mBackground.setColorAt(0.18, cCenter);
    mBackground.setColorAt(0.82, cCenter);
    mBackground.setColorAt(1.00, cSides);

    QFont font;
    font.setStyleHint(QFont::SansSerif, QFont::PreferOutline);
#ifndef Q_OS_WIN
    if (fontId == -2) {
        // Font not loaded yet, load it now!
        fontId = QFontDatabase::addApplicationFont(":/fonts/resources/custom_fonts/LiberationSansNarrow-Bold.ttf");
    }
    
    if (fontId != -1) {
        // Successfully loaded, use the internal font!
        QString family = QFontDatabase::applicationFontFamilies(fontId).at(0);
        font.setFamily(family);
    } else {
        // Fallback
        fprintf(stderr, "Failed to load internal font, using fallback... (%d)\n", fontId);
#endif
        font.setFamily("Helvetica Neue Bold");
        if (!font.exactMatch()) {
            font.setFamily("Open Sans Bold");
        }
#ifndef Q_OS_WIN
    }
#endif
    font.setBold(true);
    font.setPixelSize(5);

#ifdef _MSC_VER
/* Temporary hack... QStringLiteral mangles the UTF-8 string on MSVC for some reason */
#define Label(str)          QString::fromUtf8(str)
#else
#define Label(str)          QStringLiteral(str)
#endif

#ifdef Q_OS_MACX
    bool isMac = true;
#else
    bool isMac = false;
#endif
#ifdef Q_OS_WIN
    bool isWin = true;
#else
    bool isWin = false;
#endif

    if (isWin) {
        font.setWeight(QFont::Black);
    } else {
        font.setStretch(QFont::SemiCondensed);
    }

    mConfig.labelFont   = font,
    mConfig.secondFont  = font,
    mConfig.alphaFont   = font,
    mConfig.secondColor = QColor::fromRgb(0x93c3f3),
    mConfig.alphaColor  = QColor::fromRgb(0xa0ca1e),
    mConfig.graphColor  = cGraph,
    mConfig.numColor    = cNum,
    mConfig.otherColor  = cOther,
    mConfig.blackColor  = QColor::fromRgb(0x222222),
    mConfig.whiteColor  = QColor::fromRgb(0xeeeeee),
    mConfig.textColor   = cText,
    mConfig.key         = {1, 0};

#define LabelFrEn(fr, en)   (is83 ? Label(fr) : Label(en))

    addKey(new GraphKey{mConfig, LabelFrEn("graphe", "graph"), Label("table"), Label("f5"), 15, 2, 2 - is83});
    addKey(new GraphKey{mConfig, Label("trace"), LabelFrEn("calculs", "calc"), Label("f4"), 12, 2 + is83 * 2, 1});
    addKey(new GraphKey{mConfig, Label("zoom"), Label("format"), Label("f3"), 13, 2 + is83 * 2, is83 ? 1 : 5});
    addKey(new GraphKey{mConfig, LabelFrEn("fenêtre", "window"), LabelFrEn("déf table", "tblset"), Label("f2"), 17 - is83, is83 ? 8 : 2, 4 - is83});
    addKey(new GraphKey{mConfig, is83 ? Label("f(x)") : Label("y="), LabelFrEn("graph stats", "stat plot"), Label("f1"), 6 + is83, is83 ? 6 : 2, is83 ? 10 : 8});

    addKey(new SecondKey{mConfig, LabelFrEn("2nde", "2nd")});

    addKey(new OtherKey{mConfig, 16 - is83 * 2, 45, 37, isMac ? Label(" mode") : Label("mode"), LabelFrEn("quitter", "quit")});
    addKey(new OtherKey{mConfig, is83 ? 14 : 8, 72, 37, LabelFrEn("suppr", "del"), LabelFrEn("insérer", "ins")});
    addKey(new OtherKey{mConfig, 7, Label("on"), Label("off")});
    addKey(new OtherKey{mConfig, 13, Label("sto→"), LabelFrEn("rappel", "rcl"), Label("X"), is83 * 2, is83 * 3});
    addKey(new OtherKey{mConfig, 7, isMac ? Label(" ln") : Label("ln"), Label("eˣ"), Label("S"), is83 * 2, is83 * 2});
    addKey(new OtherKey{mConfig, 9, isMac ? Label(" log") : Label("log"), Label("10ˣ"), Label("N"), is83 * 2, is83 * 3});
    addKey(new OtherKey{mConfig, 6, Label("x²"), Label("√‾‾"), Label("I"), is83, is83 * 3});
    addKey(new OtherKey{mConfig, is83 ? 6 : 8, LabelFrEn("◀ ▶", "x⁻¹"), LabelFrEn("angle", "matrix"), Label("D"), is83 * 2, is83 ? 2 : 4});
    addKey(new OtherKey{mConfig, 14, isMac ? Label(" math") : Label("math"), LabelFrEn("tests", "test"), Label("A"), is83 * 2, is83 * 2});

    addKey(new AlphaKey{mConfig, LabelFrEn("verr A", "A-lock")});

    addKey(new NumKey{mConfig, Label("0"), Label("catalog"), Label("_"), is83 * 2, 6});
    addKey(new NumKey{mConfig, Label("1"), Label("L1"), Label("Y"), is83 * 2, is83 ? 3 : 1});
    addKey(new NumKey{mConfig, Label("4"), Label("L4"), Label("T"), is83 * 2, is83 ? 3 : 1});

    addKey(new NumKey{mConfig, Label("7"), (isMac || isWin) ? LabelFrEn("un", "u") : LabelFrEn("uₙ", "u"), Label("O"), is83 * 2, 1 + is83});
    addKey(new OtherKey{mConfig, 2, Label(","), Label("EE"), Label("J"), is83 * 2, 1 + is83});
    addKey(new OtherKey{mConfig, 8 + is83, LabelFrEn("trig", "sin"), LabelFrEn("π", "sin⁻¹"), Label("E"), is83 * 2, 1});
    addKey(new OtherKey{mConfig, 14 + is83*5, isMac ? LabelFrEn("matrice ", "  apps") : LabelFrEn("matrice", "apps"), LabelFrEn("x⁻¹", "angle"), Label("B"), is83 * 2, 1});

    addKey(new OtherKey{mConfig, 15, (isWin || isMac) ? Label("X,T,θ,n") : Label(" X,T,θ,n "), LabelFrEn("échanger", "link"), QString{}, (isWin || isMac) ? is83*2 : 1, (isWin || isMac) ? is83*3 : 1});
    addKey(new NumKey{mConfig, is83 * 2});
    addKey(new NumKey{mConfig, Label("2"), Label("L2"), Label("Z"), is83 * 2, is83 * 3});
    addKey(new NumKey{mConfig, Label("5"), Label("L5"), Label("U"), is83 * 2, is83 * 3});
    addKey(new NumKey{mConfig, Label("8"), (isMac || isWin) ? LabelFrEn("vn", "v") : LabelFrEn("vₙ", "v"), Label("P"), is83 * 2, is83 * 2});

    addKey(new OtherKey{mConfig, 3, isMac ? Label(" (") : Label("("), Label("{"), Label("K"), is83 * 2, is83});
    addKey(new OtherKey{mConfig, is83 ? 12 : 9, LabelFrEn("résol", "cos"), LabelFrEn("apps", "cos⁻¹"), Label("F"), is83 * 2, is83 * 2});
    addKey(new OtherKey{mConfig, 14, isMac ? Label(" prgm") : Label("prgm"), LabelFrEn("dessin", "draw"), Label("C"), is83 * 2, is83 * 2});
    addKey(new OtherKey{mConfig, 11 + is83, isMac ? LabelFrEn("stats", " stat") : LabelFrEn("stats", "stat"), LabelFrEn("listes", "list")});

    addKey(new NumKey{mConfig, Label("(-)"), LabelFrEn("rép", "ans"), Label("?"), is83 * 2, is83 * 3, 11});
    addKey(new NumKey{mConfig, Label("3"), Label("L3"), Label("θ"), is83 * 2, is83 * 3});
    addKey(new NumKey{mConfig, Label("6"), Label("L6"), Label("V"), is83 * 2, is83 * 3});
    addKey(new NumKey{mConfig, Label("9"), (isMac || isWin) ? LabelFrEn("wn", "w") : LabelFrEn("wₙ", "w"), Label("Q"), is83 * 2, is83 * 3});

    addKey(new OtherKey{mConfig, 3, isMac ? Label(" )") : Label(")"), Label("}"), Label("L"), is83 * 2, is83});

    addKey(new OtherKey{mConfig, 9, LabelFrEn("▫/▫", "tan"), LabelFrEn("∫⸋d▫‣", "tan⁻¹"), Label("G"), is83 * 2, is83 * 2});
    addKey(new OtherKey{mConfig, is83 ? 8 : 12, LabelFrEn("var", "vars"), LabelFrEn("distrib", "distr"), Label(""), 0, is83});
    mConfig.next();
    addKey(new OperKey{mConfig, isMac ? LabelFrEn("  entrer", "  enter") : LabelFrEn("entrer", "enter"), LabelFrEn("précéd", "entry"), is83 ? QString{} : Label("solve"), 6, is83 ? 0 : 5, {16, 5}});
    addKey(new OperKey{mConfig, isMac ? Label(" + ") : Label("+"), LabelFrEn("mém", "mem"), Label("“"), is83 * 2, is83 * 5});
    addKey(new OperKey{mConfig, Label("—"), Label("]"), Label("W"), is83 * 2, is83 * 2});
    addKey(new OperKey{mConfig, isMac ? Label(" × ") : Label("×"), Label("["), Label("R"), is83 * 2, is83 * 2});
    addKey(new OperKey{mConfig, Label("÷"), Label("e"), Label("M"), is83 * 2, is83 * 2});

    addKey(new OtherKey{mConfig, is83 ? QString{} : Label("π"), is83 * 2});
    addKey(new OtherKey{mConfig, 15, isMac ? LabelFrEn(" annul", "  clear") : LabelFrEn("annul", "clear")});

#undef Label
#undef LabelFrEn

    mConfig.next();
    QPoint center{121, 53};
    QRect outer, inner;
    outer.setSize({35, 35});
    inner.setSize({16, 16});
    outer.moveCenter(center);
    inner.moveCenter(center);
    addKey(new ArrowKey{mConfig, outer, inner, 3, QStringLiteral("down")});
    addKey(new ArrowKey{mConfig, outer, inner, 2, QStringLiteral("left")});
    addKey(new ArrowKey{mConfig, outer, inner, 0, QStringLiteral("right")});
    addKey(new ArrowKey{mConfig, outer, inner, 1, QStringLiteral("up")});

    repaint();
}
KeypadWidget::~KeypadWidget() {
    for (uint8_t row = 0; row != sRows; ++row) {
        for (uint8_t col = 0; col != sCols; ++col) {
            delete mKeys[row][col];
        }
    }
}

void KeypadWidget::resizeEvent(QResizeEvent *event) {
    QSize size{sBaseRect.size().scaled(event->size(), Qt::KeepAspectRatio)},
        origin{(event->size() - size) / 2};
    mTransform.setMatrix((qreal)size.width() / sBaseRect.width(), 0, 0, 0,
                          (qreal)size.height() / sBaseRect.height(), 0,
                          origin.width(), 0, 1);
    mInverseTransform = mTransform.inverted();
}

void KeypadWidget::paintEvent(QPaintEvent *event) {
    QRegion region{mInverseTransform.map(event->region())};
    QPainter painter{this};
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setTransform(mTransform);
    painter.fillRect(sBaseRect, mBackground);
    for (uint8_t row = 0; row != sRows; ++row) {
        for (uint8_t col = 0; col != sCols; ++col) {
            if (const Key *key = mKeys[row][col]) {
                if (region.intersects(key->textGeometry() | key->keyGeometry())) {
                    key->paint(painter);
                }
            }
        }
    }
}

void KeypadWidget::changeKeyState(KeyCode keycode, bool press, bool toggleHold) {
    if (Key *key = mKeys[keycode.row()][keycode.col()]) {
        bool wasSelected = key->isSelected();
        key->setPressed(press);
        if (toggleHold) {
            key->toggleHeld();
        }
        bool selected = key->isSelected();
        if (selected != wasSelected) {
            update(mTransform.mapRect(key->keyGeometry()));
            keypad_key_event(keycode.row(), keycode.col(), selected);
            if (selected) {
                QString out = QStringLiteral("[") + key->getLabel() + QStringLiteral("]");
                emit keyPressed(out.simplified());
            }
        }
    }
}

void KeypadWidget::mousePressEvent(QMouseEvent *event) {
    QPointF point{event->localPos() * mInverseTransform};
    for (uint8_t row = 0; row != sRows; ++row) {
        for (uint8_t col = 0; col != sCols; ++col) {
            if (Key *key = mKeys[row][col]) {
                if (key->accept(point)) {
                    changeKeyState(key->keycode(), true);
                }
            }
        }
    }
}

void KeypadWidget::mouseMoveEvent(QMouseEvent *event) {
    QPointF point{event->localPos() * mInverseTransform};
    for (uint8_t row = 0; row != sRows; ++row) {
        for (uint8_t col = 0; col != sCols; ++col) {
            if (Key *key = mKeys[row][col]) {
                if (key->accept(point)) {
                    changeKeyState(key->keycode(), true);
                } else if (key->unaccept()) {
                    changeKeyState(key->keycode(), false);
                }
            }
        }
    }
}

void KeypadWidget::mouseReleaseEvent(QMouseEvent *event) {
    bool toggleHold{event->button() == Qt::RightButton};
    for (uint8_t row = 0; row != sRows; ++row) {
        for (uint8_t col = 0; col != sCols; ++col) {
            if (Key *key = mKeys[row][col]) {
                if (key->unaccept()) {
                    changeKeyState(key->keycode(), false, toggleHold);
                }
            }
        }
    }
}
