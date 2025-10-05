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

#include "../../../core/keypad.h"

const QRect KeypadWidget::sBaseRect{{}, QSize{162, 238}};

void KeypadWidget::addKey(Key *key) {
    const KeyCode code = key->keycode();
    unsigned char row = code.row();
    unsigned char col = code.col();
    delete mKeys[row][col];
    mKeys[row][col] = key;
}

unsigned KeypadWidget::getCurrColor() const {
    return color;
}

void KeypadWidget::setType(emu_device_t device_type, unsigned int color_scheme) {
    color = color_scheme;

    cNum   = QColor::fromRgb(0xeeeeee);
    cText  = cNum;
    cOther = QColor::fromRgb(0x1d1d1d);
    cGraph = QColor::fromRgb(0xeeeeee);

    const bool is82AEP = (device_type == TI82AEP);
    const bool isFR = is82AEP || (device_type == TI83PCE);

    this->setAttribute(Qt::WA_TranslucentBackground, false);
    this->setAutoFillBackground(false);

    if (is82AEP) {
        color_scheme = KEYPAD_BLACK;
    }

    switch(color_scheme) {
        default:
        case KEYPAD_BLACK:
            cCenter = QColor::fromRgb(0x191919);
            cSides  = QColor::fromRgb(0x3b3b3b);
            break;
        case KEYPAD_WHITE:
            cCenter = QColor::fromRgb(0xe8e8e8);
            cSides  = QColor::fromRgb(0xdddddd);
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
        case KEYPAD_ROSEGOLD:
            cCenter = QColor::fromRgb(0xAF867C);
            cSides  = cCenter.darker(105);
            cOther  = QColor::fromRgb(0xD8D3B6);
            cText   = QColor::fromRgb(0x222222);
            cGraph  = QColor::fromRgb(0xD0D3D4);
            break;
        case KEYPAD_CRYSTALCLEAR:
            cCenter = QColor::fromRgb(0xACA7AE); cCenter.setAlpha(220);
            cSides  = cCenter.lighter(130);
            cOther  = QColor::fromRgb(0x191919); cOther.setAlpha(120);
            cGraph  = QColor::fromRgb(0xD0D3D4); cGraph.setAlpha(120);
            this->setAttribute(Qt::WA_TranslucentBackground, true);
            this->setAutoFillBackground(true);
            break;
        case KEYPAD_MATTEBLACK:
            cCenter = QColor::fromRgb(0x0F0F0F);
            cSides  = QColor::fromRgb(0x0F0F0F);
            break;
        case KEYPAD_TANGENTTEAL:
            cCenter = QColor::fromRgb(0x005062);
            cSides  = cCenter.lighter(150);
            cOther  = QColor::fromRgb(0x00272C);
            cGraph  = QColor::fromRgb(0x6C7F90);
            break;
        case KEYPAD_TOTALLYTEAL:
            cCenter = QColor::fromRgb(0x108798);
            cSides  = cCenter.darker(200);
            cOther  = QColor::fromRgb(0x125E68);
            cGraph  = QColor::fromRgb(0x2E4854);
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
        fontId = QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/resources/custom_fonts/LiberationSansNarrow-Bold.ttf"));
    }

    if (fontId != -1) {
        // Successfully loaded, use the internal font!
        QString family = QFontDatabase::applicationFontFamilies(fontId).at(0);
        font.setFamily(family);
    } else {
        // Fallback
        //fprintf(stderr, "Failed to load internal font, using fallback... (%d)\n", fontId);
#endif
        font.setFamily(QStringLiteral("Helvetica Neue Bold"));
        if (!font.exactMatch()) {
            font.setFamily(QStringLiteral("Open Sans Bold"));
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

#ifdef Q_OS_MACOS
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

    mConfig.labelFont   = font;
    mConfig.secondFont  = font;
    mConfig.alphaFont   = font;
    mConfig.secondColor = QColor::fromRgb(is82AEP ? 0xb02036 : 0x93c3f3);
    mConfig.alphaColor  = QColor::fromRgb(is82AEP ? 0x8cd0f6 : 0xa0ca1e);
    mConfig.graphColor  = cGraph;
    mConfig.numColor    = cNum;
    mConfig.otherColor  = cOther;
    mConfig.blackColor  = QColor::fromRgb(0x222222);
    mConfig.whiteColor  = QColor::fromRgb(0xeeeeee);
    mConfig.textColor   = cText;
    mConfig.key         = {1, 0};
    mConfig.alphaKeyTextColor = is82AEP ? mConfig.blackColor : mConfig.textColor;

#define LabelFrEn(fr, en)   (isFR ? Label(fr) : Label(en))

    addKey(new GraphKey{mConfig, LabelFrEn("graphe", "graph"), Label("table"), Label("f5"), 15, 2, 2 - isFR});
    addKey(new GraphKey{mConfig, Label("trace"), LabelFrEn("calculs", "calc"), Label("f4"), 12, 2 + isFR * 2, 1});
    addKey(new GraphKey{mConfig, Label("zoom"), Label("format"), Label("f3"), 13, 2 + isFR * 2, isFR ? 1 : 5});
    addKey(new GraphKey{mConfig, LabelFrEn("fenêtre", "window"), LabelFrEn("déf table", "tblset"), Label("f2"), 17 - isFR, isFR ? 8 : 2, 4 - isFR});
    addKey(new GraphKey{mConfig, isFR ? Label("f(x)") : Label("y="), LabelFrEn("graph stats", "stat plot"), Label("f1"), 6 + isFR, isFR ? 6 : 2, isFR ? 10 : 8});

    addKey(new SecondKey{mConfig, LabelFrEn("2nde", "2nd")});

    addKey(new OtherKey{mConfig, 16 - isFR * 2, 45, 37, isMac ? Label(" mode") : Label("mode"), LabelFrEn("quitter", "quit")});
    addKey(new OtherKey{mConfig, isFR ? 14 : 8, 72, 37, LabelFrEn("suppr", "del"), LabelFrEn("insérer", "ins")});
    addKey(new OtherKey{mConfig, 7, Label("on"), Label("off")});
    addKey(new OtherKey{mConfig, 13, Label("sto→"), LabelFrEn("rappel", "rcl"), Label("X"), isFR * 2, isFR * 3});
    addKey(new OtherKey{mConfig, 7, isMac ? Label(" ln") : Label("ln"), Label("eˣ"), Label("S"), isFR * 2, isFR * 2});
    addKey(new OtherKey{mConfig, 9, isMac ? Label(" log") : Label("log"), Label("10ˣ"), Label("N"), isFR * 2, isFR * 3});
    addKey(new OtherKey{mConfig, 6, Label("x²"), Label("√‾‾"), Label("I"), isFR, isFR * 3});
    addKey(new OtherKey{mConfig, isFR ? 6 : 8, LabelFrEn("◀ ▶", "x⁻¹"), LabelFrEn("angle", "matrix"), Label("D"), isFR * 2, isFR ? 2 : 4});
    addKey(new OtherKey{mConfig, 14, isMac ? Label(" math") : Label("math"), LabelFrEn("tests", "test"), Label("A"), isFR * 2, isFR * 2});

    addKey(new AlphaKey{mConfig, LabelFrEn("verr A", "A-lock")});

    addKey(new NumKey{mConfig, Label("0"), Label("catalog"), Label("_"), isFR * 2, 6});
    addKey(new NumKey{mConfig, Label("1"), Label("L1"), Label("Y"), isFR * 2, isFR ? 3 : 1});
    addKey(new NumKey{mConfig, Label("4"), Label("L4"), Label("T"), isFR * 2, isFR ? 3 : 1});

    addKey(new NumKey{mConfig, Label("7"), (isMac || isWin) ? LabelFrEn("un", "u") : LabelFrEn("uₙ", "u"), Label("O"), isFR * 2, 1 + isFR});
    addKey(new OtherKey{mConfig, 2, Label(","), Label("EE"), Label("J"), isFR * 2, 1 + isFR});
    addKey(new OtherKey{mConfig, 8 + isFR, LabelFrEn("trig", "sin"), LabelFrEn("π", "sin⁻¹"), Label("E"), isFR * 2, 1});
    addKey(new OtherKey{mConfig, 14 + isFR*5, isMac ? LabelFrEn("matrice ", "  apps") : LabelFrEn("matrice", "apps"), LabelFrEn("x⁻¹", "angle"), Label("B"), isFR * 2, 1});

    addKey(new OtherKey{mConfig, 15, (isWin || isMac) ? Label("X,T,θ,n") : Label(" X,T,θ,n "), LabelFrEn("échanger", "link"), QString{}, (isWin || isMac) ? isFR*2 : 1, (isWin || isMac) ? isFR*3 : 1});
    addKey(new NumKey{mConfig, isFR * 2});
    addKey(new NumKey{mConfig, Label("2"), Label("L2"), Label("Z"), isFR * 2, isFR * 3});
    addKey(new NumKey{mConfig, Label("5"), Label("L5"), Label("U"), isFR * 2, isFR * 3});
    addKey(new NumKey{mConfig, Label("8"), (isMac || isWin) ? LabelFrEn("vn", "v") : LabelFrEn("vₙ", "v"), Label("P"), isFR * 2, isFR * 2});

    addKey(new OtherKey{mConfig, 3, isMac ? Label(" (") : Label("("), Label("{"), Label("K"), isFR * 2, isFR});
    addKey(new OtherKey{mConfig, isFR ? 12 : 9, LabelFrEn("résol", "cos"), LabelFrEn("apps", "cos⁻¹"), Label("F"), isFR * 2, isFR * 2});
    addKey(new OtherKey{mConfig, 14, isMac ? Label(" prgm") : Label("prgm"), LabelFrEn("dessin", "draw"), Label("C"), isFR * 2, isFR * 2});
    addKey(new OtherKey{mConfig, 11 + isFR, isMac ? LabelFrEn("stats", " stat") : LabelFrEn("stats", "stat"), LabelFrEn("listes", "list")});

    addKey(new NumKey{mConfig, Label("(-)"), LabelFrEn("rép", "ans"), Label("?"), isFR * 2, isFR * 3, 11});
    addKey(new NumKey{mConfig, Label("3"), Label("L3"), Label("θ"), isFR * 2, isFR * 3});
    addKey(new NumKey{mConfig, Label("6"), Label("L6"), Label("V"), isFR * 2, isFR * 3});
    addKey(new NumKey{mConfig, Label("9"), (isMac || isWin) ? LabelFrEn("wn", "w") : LabelFrEn("wₙ", "w"), Label("Q"), isFR * 2, isFR * 3});

    addKey(new OtherKey{mConfig, 3, isMac ? Label(" )") : Label(")"), Label("}"), Label("L"), isFR * 2, isFR});

    addKey(new OtherKey{mConfig, 9, LabelFrEn("▫/▫", "tan"), LabelFrEn("∫⸋d▫‣", "tan⁻¹"), Label("G"), isFR * 2, isFR * 2});
    addKey(new OtherKey{mConfig, isFR ? 8 : 12, LabelFrEn("var", "vars"), LabelFrEn("distrib", "distr"), Label(""), 0, isFR});
    mConfig.next();
    addKey(new OperKey{mConfig, isMac ? LabelFrEn("  entrer", "  enter") : LabelFrEn("entrer", "enter"), LabelFrEn("précéd", "entry"), isFR ? QString{} : Label("solve"), 6, isFR ? 0 : 5, {16, 5}});
    addKey(new OperKey{mConfig, isMac ? Label(" + ") : Label("+"), LabelFrEn("mém", "mem"), Label("“"), isFR * 2, isFR * 5});
    addKey(new OperKey{mConfig, Label("—"), Label("]"), Label("W"), isFR * 2, isFR * 2});
    addKey(new OperKey{mConfig, isMac ? Label(" × ") : Label("×"), Label("["), Label("R"), isFR * 2, isFR * 2});
    addKey(new OperKey{mConfig, Label("÷"), Label("e"), Label("M"), isFR * 2, isFR * 2});

    addKey(new OtherKey{mConfig, isFR ? QString{} : Label("π"), isFR * 2});
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
    addKey(new ArrowKey{mConfig, outer, inner, 3, QStringLiteral("down"), 2});
    addKey(new ArrowKey{mConfig, outer, inner, 2, QStringLiteral("left"), 2});
    addKey(new ArrowKey{mConfig, outer, inner, 0, QStringLiteral("right"), 2});
    addKey(new ArrowKey{mConfig, outer, inner, 1, QStringLiteral("up"), 2});

    repaint();
}

void KeypadWidget::setHolding(bool enabled) {
    mHoldingEnabled = enabled;
    if (!enabled) {
        for (uint8_t row = 0; row != sRows; ++row) {
            for (uint8_t col = 0; col != sCols; ++col) {
                if (Key *key = mKeys[row][col]) {
                    if (key->isHeld()) {
                        bool wasSelected = key->isSelected();
                        key->toggleHeld();
                        updateKey(key, wasSelected);
                    }
                }
            }
        }
    }
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
    mTransform.setMatrix(static_cast<qreal>(size.width()) / sBaseRect.width(), 0, 0, 0,
                         static_cast<qreal>(size.height()) / sBaseRect.height(), 0,
                         origin.width(), 0, 1);
    mInverseTransform = mTransform.inverted();
}

void KeypadWidget::paintEvent(QPaintEvent *event) {
    QRegion region{mInverseTransform.map(event->region())};
    QPainter painter{this};
    painter.setRenderHint(QPainter::Antialiasing);
    if (color == KEYPAD_CRYSTALCLEAR) {
        painter.fillRect(this->rect(), cclrBackground);
    }
    painter.setTransform(mTransform);
    painter.fillPath(keypadPath, mBackground);
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

void KeypadWidget::changeKeyState(KeyCode code, bool press) {
    if (Key *key = mKeys[code.row()][code.col()]) {
        bool wasSelected = key->isSelected();
        key->setPressed(press);
        updateKey(key, wasSelected);
    }
}

void KeypadWidget::updateKey(Key *key, bool wasSelected) {
    bool selected = key->isSelected();
    if (selected != wasSelected) {
        update(mTransform.mapRect(key->keyGeometry()));
        emu_keypad_event(key->keycode().row(), key->keycode().col(), selected);
        if (selected) {
            QString out = QStringLiteral("[") + key->getLabel() + QStringLiteral("]");
            emit keyPressed(out.simplified().replace(" ",""));
        }
    }
}

void KeypadWidget::mouseUpdate(const QPointF &pos) {
    const QPainterPath area{pos * mInverseTransform};
    for (uint8_t row = 0; row != sRows; ++row) {
        for (uint8_t col = 0; col != sCols; ++col) {
            if (Key *key = mKeys[row][col]) {
                bool wasSelected = key->isSelected();
                if (key->isUnder(area)) {
                    if (!mClicked.contains(key->keycode())) {
                        mClicked.insert(key->keycode());
                        key->press();
                    }
                } else if (mClicked.remove(key->keycode())) {
                    key->release();
                }
                updateKey(key, wasSelected);
            }
        }
    }
}

void KeypadWidget::mouseEnd(bool toggleHeld) {
    for (KeyCode code : mClicked) {
        if (Key *key = mKeys[code.row()][code.col()]) {
            bool wasSelected = key->isSelected();
            key->release();
            if (key->isHeld() || toggleHeld) {
                key->toggleHeld();
            }
            updateKey(key, wasSelected);
        }
    }
    mClicked.clear();
}

void KeypadWidget::mouseEvent(QMouseEvent *event) {
    if (event->source() != Qt::MouseEventNotSynthesized) {
        event->accept();
        return;
    }
    switch (event->type()) {
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
            mouseUpdate(event->position());
            break;
        case QEvent::MouseButtonRelease:
            mouseEnd(mHoldingEnabled && event->button() == Qt::RightButton);
            break;
        default:
            abort();
    }
}

void KeypadWidget::touchUpdate(const QList<QTouchEvent::TouchPoint> &points) {
    for (uint8_t row = 0; row != sRows; ++row) {
        for (uint8_t col = 0; col != sCols; ++col) {
            if (Key *key = mKeys[row][col]) {
                bool wasSelected = key->isSelected();
                for (const QTouchEvent::TouchPoint &point : points) {
                    if (point.state() & Qt::TouchPointStationary) {
                        continue;
                    }
                    QRectF ellipse;
                    ellipse.setSize(point.ellipseDiameters());
                    ellipse.moveCenter(point.position());
                    ellipse = mInverseTransform.mapRect(ellipse);
                    if (ellipse.isEmpty()) {
                        ellipse += {4, 4, 4, 4};
                    }
                    QPainterPath area;
                    area.addEllipse(ellipse);
                    if (point.rotation() != 0.0) {
                        area = area * QTransform::fromTranslate(ellipse.center().x(),
                                                                ellipse.center().y()).
                            rotate(point.rotation()).
                            translate(-ellipse.center().x(), -ellipse.center().y());
                    }
                    if ((point.state() & (Qt::TouchPointMoved | Qt::TouchPointPressed)) && key->isUnder(area)) {
                        if (!mTouched.contains(key->keycode())) {
                            mTouched.insert(key->keycode());
                            key->press();
                        }
                    } else if (mTouched.remove(key->keycode())) {
                        key->release();
                    }
                }
                updateKey(key, wasSelected);
            }
        }
    }
}

void KeypadWidget::touchEnd() {
    for (KeyCode code : mTouched) {
        if (Key *key = mKeys[code.row()][code.col()]) {
            bool wasSelected = key->isSelected();
            key->release();
            updateKey(key, wasSelected);
        }
    }
    mTouched.clear();

    // release any other keys that may be stuck??
    for (uint8_t row = 0; row != sRows; ++row) {
        for (uint8_t col = 0; col != sCols; ++col) {
            if (Key *key = mKeys[row][col]) {
                bool selected = key->isSelected();
                if (selected) {
                    key->release();
                    updateKey(key, selected);
                }
            }
        }
    }
}

void KeypadWidget::touchEvent(QTouchEvent *event) {
    switch (event->type()) {
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
            if (event->device()->capabilities().testFlag(QInputDevice::Capability::Position)) {
                touchUpdate(event->points());
            } else {
                event->ignore();
            }
            break;
        case QEvent::TouchEnd:
        case QEvent::TouchCancel:
            touchEnd();
            break;
        default:
            abort();
    }
}

bool KeypadWidget::event(QEvent *event) {
    switch (event->type()) {
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
        case QEvent::MouseButtonRelease:
            mouseEvent(static_cast<QMouseEvent *>(event));
            break;
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        case QEvent::TouchCancel:
            touchEvent(static_cast<QTouchEvent *>(event));
            break;
        default:
            return QWidget::event(event);
    }
    return true;
}
