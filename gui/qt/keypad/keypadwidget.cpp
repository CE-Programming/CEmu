#include "keypadwidget.h"
#include "graphkey.h"
#include "secondkey.h"
#include "alphakey.h"
#include "operkey.h"
#include "otherkey.h"
#include "numkey.h"
#include "arrowkey.h"
#include "../../../core/asic.h"

#include <QtWidgets/QApplication>
#include <QtGui/QPaintEvent>
#include <QtGui/QScreen>

const QRect KeypadWidget::s_baseRect{{}, QSize{162, 235}};

void KeypadWidget::addKey(Key *key) {
    delete m_keys[key->keycode().row()][key->keycode().col()];
    m_keys[key->keycode().row()][key->keycode().col()] = key;
}

unsigned KeypadWidget::getCurrColor(void) {
    return curr_color;
}

void KeypadWidget::setType(bool is83, unsigned color_scheme) {
    QColor c_center;
    QColor c_sides;
    QColor c_num, c_text, c_other, c_graph;

    curr_color = color_scheme;

    c_num   = QColor::fromRgb(0xeeeeee);
    c_text  = QColor::fromRgb(0xeeeeee);
    c_other = QColor::fromRgb(0x1d1d1d);
    c_graph = QColor::fromRgb(0xeeeeee);

    switch(color_scheme) {
        default:
        case KEYPAD_BLACK:
            c_center = QColor::fromRgb(0x191919);
            c_sides  = QColor::fromRgb(0x3b3b3b);
            break;
        case KEYPAD_WHITE:
            c_center = QColor::fromRgb(0xe8e8e8);
            c_sides  = QColor::fromRgb(0xc4c4c4);
            c_num    = QColor::fromRgb(0x707880);
            c_text   = QColor::fromRgb(0x222222);
            c_other  = QColor::fromRgb(0xc0c0c0);
            break;
        case KEYPAD_TRUE_BLUE:
            c_center = QColor::fromRgb(0x385E9D);
            c_sides  = c_center.lighter(130);
            c_num    = QColor::fromRgb(0xdedede);
            c_other  = QColor::fromRgb(0x274F91);
            break;
        case KEYPAD_DENIM:
            c_center = QColor::fromRgb(0x003C71);
            c_sides  = c_center.lighter(130);
            c_other  = QColor::fromRgb(0x013766);
            break;
        case KEYPAD_SILVER:
            c_center = QColor::fromRgb(0x7C878E);
            c_sides  = c_center.lighter(130);
            c_other  = QColor::fromRgb(0x191919);
            c_graph  = QColor::fromRgb(0xD0D3D4);
            break;
        case KEYPAD_PINK:
            c_center = QColor::fromRgb(0xDF1995);
            c_sides  = c_center.lighter(130);
            c_other  = QColor::fromRgb(0xAA0061);
            break;
        case KEYPAD_PLUM:
            c_center = QColor::fromRgb(0x830065);
            c_sides  = c_center.lighter(130);
            c_other  = QColor::fromRgb(0x5E2751);
            break;
        case KEYPAD_RED:
            c_center = QColor::fromRgb(0xAB2328);
            c_sides  = c_center.lighter(130);
            c_other  = QColor::fromRgb(0x8A2A2B);
            break;
        case KEYPAD_LIGHTNING:
            c_center = QColor::fromRgb(0x0077C8);
            c_sides  = c_center.lighter(130);
            c_other  = QColor::fromRgb(0x0077C8);
            break;
        case KEYPAD_GOLDEN:
            c_center = QColor::fromRgb(0xD8D3B6);
            c_sides  = c_center.lighter(130);
            c_other  = QColor::fromRgb(0xD8D3B6);
            break;
    }

    m_background = {s_baseRect.topLeft(), s_baseRect.topRight()};
    m_background.setColorAt(0.00, c_sides);
    m_background.setColorAt(0.18, c_center);
    m_background.setColorAt(0.82, c_center);
    m_background.setColorAt(1.00, c_sides);

    QFont font;
    font.setStyleHint(QFont::SansSerif, QFont::PreferOutline);
    font.setFamily("Helvetica Neue Bold");
    if (!font.exactMatch()) {
        font.setFamily("Open Sans Bold");
    }

    font.setBold(true);
    font.setPixelSize(5);
#ifdef Q_OS_WIN
    font.setWeight(QFont::Black);
#else
    font.setStretch(QFont::SemiCondensed);
#endif

    m_config.labelFont   = font,
    m_config.secondFont  = font,
    m_config.alphaFont   = font,
    m_config.secondColor = QColor::fromRgb(0x93c3f3),
    m_config.alphaColor  = QColor::fromRgb(0xa0ca1e),
    m_config.graphColor  = c_graph,
    m_config.numColor    = c_num,
    m_config.otherColor  = c_other,
    m_config.blackColor  = QColor::fromRgb(0x222222),
    m_config.whiteColor  = QColor::fromRgb(0xeeeeee),
    m_config.textColor   = c_text,
    m_config.key         = {1, 0};

#ifdef _MSC_VER
/* Temporary hack... QStringLiteral mangles the UTF-8 string on MSVC for some reason */
#define Label(str)          QString::fromUtf8(str)
#else
#define Label(str)          QStringLiteral(str)
#endif

#define LabelFrEn(fr, en)   (is83 ? Label(fr) : Label(en))

    addKey(new GraphKey{m_config, LabelFrEn("graphe", "graph"), Label("table"), Label("f5"), 15, 2, 2 - is83});
    addKey(new GraphKey{m_config, Label("trace"), LabelFrEn("calculs", "calc"), Label("f4"), is83 ? 10 : 12, 2 + is83 * 2, 1});
    addKey(new GraphKey{m_config, Label("zoom"), Label("format"), Label("f3"), is83 ? 11 : 13, 2 + is83 * 2, is83 ? 1 : 5});
    addKey(new GraphKey{m_config, LabelFrEn("fenêtre", "window"), LabelFrEn("déf table", "tblset"),
                        Label("f2"), 15 - is83, is83 ? 6 : 2, 4 - is83});
    addKey(new GraphKey{m_config, is83 ? Label("f(x)") : Label("y="), LabelFrEn("graph stats", "stat plot"),
                        Label("f1"), 6 + is83, is83 ? 6 : 2, is83 ? 10 : 8});

    addKey(new SecondKey{m_config, LabelFrEn("2nde", "2nd")});

#ifdef Q_OS_MACX
    addKey(new OtherKey{m_config, 16 - is83 * 2, 45, 37, Label(" mode"), LabelFrEn("quitter", "quit")});
#else
    addKey(new OtherKey{m_config, 16 - is83 * 2, 45, 37, Label("mode"), LabelFrEn("quitter", "quit")});
#endif
    addKey(new OtherKey{m_config, is83 ? 14 : 8, 72, 37, LabelFrEn("suppr", "del"), LabelFrEn("insérer", "ins")});
    addKey(new OtherKey{m_config, 7, Label("on"), Label("off")});
    addKey(new OtherKey{m_config, 13, Label("sto→"), LabelFrEn("rappel", "rcl"), Label("X"), is83 * 2, is83 * 3});
#ifdef Q_OS_MACX
    addKey(new OtherKey{m_config, 7, Label(" ln"), Label("eˣ"), Label("S"), is83 * 2, is83 * 2});
    addKey(new OtherKey{m_config, 9, Label(" log"), Label("10ˣ"), Label("N"), is83 * 2, is83 * 3});
#else
    addKey(new OtherKey{m_config, 7, Label("ln"), Label("eˣ"), Label("S"), is83 * 2, is83 * 2});
    addKey(new OtherKey{m_config, 9, Label("log"), Label("10ˣ"), Label("N"), is83 * 2, is83 * 3});
#endif
    addKey(new OtherKey{m_config, 6, Label("x²"), Label("√‾‾"), Label("I"), is83, is83 * 3});
    addKey(new OtherKey{m_config, is83 ? 6 : 8, LabelFrEn("◀ ▶", "x⁻¹"), LabelFrEn("angle", "matrix"), Label("D"), is83 * 2, is83 ? 2 : 4});
#ifdef Q_OS_MACX
    addKey(new OtherKey{m_config, 14, Label(" math"), LabelFrEn("tests", "test"), Label("A"), is83 * 2, is83 * 2});
#else
    addKey(new OtherKey{m_config, 14, Label("math"), LabelFrEn("tests", "test"), Label("A"), is83 * 2, is83 * 2});
#endif

    addKey(new AlphaKey{m_config, LabelFrEn("verr A", "A-lock")});

#ifdef Q_OS_MACX
    addKey(new NumKey{m_config, Label("0"), Label("catalog"), Label("_"), is83 * 2, 6});
#else
    addKey(new NumKey{m_config, Label("0"), Label("catalog"), Label("⎵"), is83 * 2, 6});
#endif
    addKey(new NumKey{m_config, Label("1"), Label("L1"), Label("Y"), is83 * 2, is83 ? 3 : 1});
    addKey(new NumKey{m_config, Label("4"), Label("L4"), Label("T"), is83 * 2, is83 ? 3 : 1});

#if defined(_WIN32) || defined(Q_OS_MACX)
    addKey(new NumKey{m_config, Label("7"), LabelFrEn("un", "u"), Label("O"), is83 * 2, 1 + is83});
#else
    addKey(new NumKey{m_config, Label("7"), LabelFrEn("uₙ", "u"), Label("O"), is83 * 2, 1 + is83});
#endif
    addKey(new OtherKey{m_config, 2, Label(","), Label("EE"), Label("J"), is83 * 2, 1 + is83});
    addKey(new OtherKey{m_config, 8 + is83, LabelFrEn("trig", "sin"), LabelFrEn("π", "sin⁻¹"), Label("E"), is83 * 2, 1});
#ifdef Q_OS_MACX
    addKey(new OtherKey{m_config, 14 + is83, LabelFrEn("matrice ", "  apps"), LabelFrEn("x⁻¹", "angle"), Label("B"), is83 * 2, 1});
#else
    addKey(new OtherKey{m_config, 14 + is83, LabelFrEn("matrice", "apps"), LabelFrEn("x⁻¹", "angle"), Label("B"), is83 * 2, 1});
#endif
#ifdef _WIN32
    addKey(new OtherKey{m_config, 15 + is83, Label("X,T,θ,n"), LabelFrEn("échanger", "link"), QString{}, is83 * 2, is83 * 3});
#elif defined(Q_OS_MACX)
    addKey(new OtherKey{m_config, 15 + is83, Label(" X,T,θ,n "), LabelFrEn("échanger", "link"), QString{}, 1, 1});
#else
    addKey(new OtherKey{m_config, 15 + is83, Label("X,T,θ,n"), LabelFrEn("échanger", "link"), QString{}, 1, 1});
#endif
    addKey(new NumKey{m_config, is83 * 2});
    addKey(new NumKey{m_config, Label("2"), Label("L2"), Label("Z"), is83 * 2, is83 * 3});
    addKey(new NumKey{m_config, Label("5"), Label("L5"), Label("U"), is83 * 2, is83 * 3});
#if defined(_WIN32) || defined(Q_OS_MACX)
    addKey(new NumKey{m_config, Label("8"), LabelFrEn("vn", "v"), Label("P"), is83 * 2, is83 * 2});
#else
    addKey(new NumKey{m_config, Label("8"), LabelFrEn("vₙ", "v"), Label("P"), is83 * 2, is83 * 2});
#endif
    addKey(new OtherKey{m_config, 3, Label("("), Label("{"), Label("K"), is83 * 2, is83});
#ifdef Q_OS_MACX
    addKey(new OtherKey{m_config, is83 ? 12 : 9, LabelFrEn("résol", "cos"), LabelFrEn("apps", "cos⁻¹"), Label("F"), is83 * 2, is83 * 2});
#else
    addKey(new OtherKey{m_config, is83 ? 12 : 9, LabelFrEn("résol", "cos"), LabelFrEn("apps", "cos⁻¹"), Label("F"), is83 * 2, is83 * 2});
#endif
#ifdef Q_OS_MACX
    addKey(new OtherKey{m_config, 14, Label(" prgm"), LabelFrEn("dessin", "draw"), Label("C"), is83 * 2, is83 * 2});
    addKey(new OtherKey{m_config, 11 + is83, LabelFrEn("stats", " stat"), LabelFrEn("listes", "list")});
#else
    addKey(new OtherKey{m_config, 14, Label("prgm"), LabelFrEn("dessin", "draw"), Label("C"), is83 * 2, is83 * 2});
    addKey(new OtherKey{m_config, 11 + is83, LabelFrEn("stats", "stat"), LabelFrEn("listes", "list")});
#endif

    addKey(new NumKey{m_config, Label("(-)"), LabelFrEn("rép", "ans"), Label("?"), is83 * 2, is83 * 3, 11});
    addKey(new NumKey{m_config, Label("3"), Label("L3"), Label("θ"), is83 * 2, is83 * 3});
    addKey(new NumKey{m_config, Label("6"), Label("L6"), Label("V"), is83 * 2, is83 * 3});
#if defined(_WIN32) || defined(Q_OS_MACX)
    addKey(new NumKey{m_config, Label("9"), LabelFrEn("wn", "w"), Label("Q"), is83 * 2, is83 * 3});
#else
    addKey(new NumKey{m_config, Label("9"), LabelFrEn("wₙ", "w"), Label("Q"), is83 * 2, is83 * 3});
#endif
    addKey(new OtherKey{m_config, 3, Label(")"), Label("}"), Label("L"), is83 * 2, is83});
#ifdef _WIN32
    addKey(new OtherKey{m_config, 9, LabelFrEn("⸋|⸋", "tan"), LabelFrEn("∫⸋|⸋d▫‣", "tan⁻¹"), Label("G"), is83 * 2, is83 * 2});
#elif defined(Q_OS_MACX)
    addKey(new OtherKey{m_config, 9, LabelFrEn("▫/▫", "tan"), LabelFrEn("∫⸋d▫‣", "tan⁻¹"), Label("G"), is83 * 2, is83 * 2});
#else
    addKey(new OtherKey{m_config, 9, LabelFrEn("▫/▫", "tan"), LabelFrEn("∫⸋̻◻d▫‣", "tan⁻¹"), Label("G"), is83 * 2, is83 * 2});
#endif
    addKey(new OtherKey{m_config, is83 ? 9 : 12, LabelFrEn("var", "vars"), LabelFrEn("distrib", "distr"), Label(""), 0, is83});
    m_config.next();
#ifdef Q_OS_MACX
    addKey(new OperKey{m_config, LabelFrEn("  entrer", "  enter"), LabelFrEn("précéd", "entry"), is83 ? QString{} : Label("solve"), 6, is83 ? 0 : 5, {16, 5}});
#else
    addKey(new OperKey{m_config, LabelFrEn("entrer", "enter"), LabelFrEn("précéd", "entry"), is83 ? QString{} : Label("solve"), 6, is83 ? 0 : 5, {16, 5}});
#endif
    addKey(new OperKey{m_config, Label("+"), LabelFrEn("mém", "mem"), Label("“"), is83 * 2, is83 * 5});
    addKey(new OperKey{m_config, Label("—"), Label("]"), Label("W"), is83 * 2, is83 * 2});
    addKey(new OperKey{m_config, Label("×"), Label("["), Label("R"), is83 * 2, is83 * 2});
    addKey(new OperKey{m_config, Label("÷"), Label("e"), Label("M"), is83 * 2, is83 * 2});

    addKey(new OtherKey{m_config, is83 ? QString{} : Label("π"), is83 * 2});
#ifdef Q_OS_MACX
    addKey(new OtherKey{m_config, 15, LabelFrEn(" annul", "  clear")});
#else
    addKey(new OtherKey{m_config, 15, LabelFrEn("annul", "clear")});
#endif

#undef Label
#undef LabelFrEn

    m_config.next();
    QPoint center{121, 53};
    QRect outer, inner;
    outer.setSize({35, 35});
    inner.setSize({16, 16});
    outer.moveCenter(center);
    inner.moveCenter(center);
    addKey(new ArrowKey{m_config, outer, inner, 3});
    addKey(new ArrowKey{m_config, outer, inner, 2});
    addKey(new ArrowKey{m_config, outer, inner, 0});
    addKey(new ArrowKey{m_config, outer, inner, 1});

    update();
}
KeypadWidget::~KeypadWidget() {
    for (uint8_t row = 0; row != s_rows; ++row) {
        for (uint8_t col = 0; col != s_cols; ++col) {
            delete m_keys[row][col];
        }
    }
}

void KeypadWidget::resizeEvent(QResizeEvent *event) {
    QSize size{s_baseRect.size().scaled(event->size(), Qt::KeepAspectRatio)},
        origin{(event->size() - size) / 2};
    m_transform.setMatrix((qreal)size.width() / s_baseRect.width(), 0, 0, 0,
                          (qreal)size.height() / s_baseRect.height(), 0,
                          origin.width(), 0, 1);
    m_inverseTransform = m_transform.inverted();
}

void KeypadWidget::paintEvent(QPaintEvent *event) {
    QRegion region{m_inverseTransform.map(event->region())};
    QPainter painter{this};
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setTransform(m_transform);
    painter.fillRect(s_baseRect, m_background);
    for (uint8_t row = 0; row != s_rows; ++row) {
        for (uint8_t col = 0; col != s_cols; ++col) {
            if (const Key *key = m_keys[row][col]) {
                if (region.intersects(key->textGeometry() | key->keyGeometry())) {
                    key->paint(painter);
                }
            }
        }
    }
}

void KeypadWidget::changeKeyState(KeyCode keycode, bool press, bool toggleHold) {
    if (Key *key = m_keys[keycode.row()][keycode.col()]) {
        bool wasSelected = key->isSelected();
        key->setPressed(press);
        if (toggleHold) {
            key->toggleHeld();
        }
        bool selected = key->isSelected();
        if (selected != wasSelected) {
            update(m_transform.mapRect(key->keyGeometry()));
            keypad_key_event(keycode.row(), keycode.col(), selected);
        }
    }
}

void KeypadWidget::mousePressEvent(QMouseEvent *event) {
    QPointF point{event->localPos() * m_inverseTransform};
    for (uint8_t row = 0; row != s_rows; ++row) {
        for (uint8_t col = 0; col != s_cols; ++col) {
            if (Key *key = m_keys[row][col]) {
                if (key->accept(point)) {
                    changeKeyState(key->keycode(), true);
                }
            }
        }
    }
}

void KeypadWidget::mouseMoveEvent(QMouseEvent *event) {
    QPointF point{event->localPos() * m_inverseTransform};
    for (uint8_t row = 0; row != s_rows; ++row) {
        for (uint8_t col = 0; col != s_cols; ++col) {
            if (Key *key = m_keys[row][col]) {
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
    for (uint8_t row = 0; row != s_rows; ++row) {
        for (uint8_t col = 0; col != s_cols; ++col) {
            if (Key *key = m_keys[row][col]) {
                if (key->unaccept()) {
                    changeKeyState(key->keycode(), false, toggleHold);
                }
            }
        }
    }
}
