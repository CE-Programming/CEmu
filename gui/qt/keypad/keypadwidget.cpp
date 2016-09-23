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

#include <QtWidgets/QApplication>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QScreen>

const QRect KeypadWidget::s_baseRect{{}, QSize{162, 248}};

void KeypadWidget::addKey(Key *key) {
    delete m_keys[key->keycode().row()][key->keycode().col()];
    m_keys[key->keycode().row()][key->keycode().col()] = key;
}

void KeypadWidget::setType(bool type) {
    m_type = type;

    m_background = {s_baseRect.topLeft(), s_baseRect.topRight()};
    m_background.setColorAt(0.00, QColor::fromRgb(type ? 0xc4c4c4 : 0x3b3b3b));
    m_background.setColorAt(0.18, QColor::fromRgb(type ? 0xe8e8e8 : 0x191919));
    m_background.setColorAt(0.82, QColor::fromRgb(type ? 0xe8e8e8 : 0x191919));
    m_background.setColorAt(1.00, QColor::fromRgb(type ? 0xc4c4c4 : 0x3b3b3b));

    QFont font;
    font.setStyleHint(QFont::SansSerif, QFont::PreferOutline);
    font.setFamily("Helvetica Neue Bold");
    if (!font.exactMatch()) {
        font.setFamily("Open Sans Bold");
    }

#ifdef _WIN32
    font.setWeight(QFont::Black);
    qreal screenDPI  = QApplication::primaryScreen()->physicalDotsPerInch();
    qreal RENDER_DPI = 96;

    int pixelSize = (int)((qreal)7 * screenDPI / RENDER_DPI);
    font.setPixelSize(pixelSize);
#else
    font.setBold(true);
    font.setPixelSize(5);
    font.setStretch(QFont::SemiCondensed);
#endif

    m_config = {
          .labelFont = font,
         .secondFont = font,
          .alphaFont = font,
        .secondColor = QColor::fromRgb(0x93c3f3),
         .alphaColor = QColor::fromRgb(0xa0ca1e),
         .graphColor = QColor::fromRgb(0xeeeeee),
           .numColor = QColor::fromRgb(type ? 0x707880 : 0xeeeeee),
         .otherColor = QColor::fromRgb(type ? 0xc0c0c0 : 0x1d1d1d),
         .blackColor = QColor::fromRgb(0x222222),
         .whiteColor = QColor::fromRgb(0xeeeeee),
          .textColor = QColor::fromRgb(type ? 0x222222 : 0xeeeeee),
                .key = {1, 0}
    };
    if (type) {
#ifndef _WIN32
        m_config.secondFont.setStretch(QFont::Condensed);
#endif
    }

    QString quart_space = QChar(0x2005);
    addKey(new GraphKey{m_config, type ? QStringLiteral("graphe") : QStringLiteral("graph"),
                        QStringLiteral("table"), QStringLiteral("f5"), 15, 2, 2 - type});
    addKey(new GraphKey{m_config, QStringLiteral("trace"),
                        type ? QStringLiteral("calculs") : QStringLiteral("calc"),
                        QStringLiteral("f4"), type ? 10 : 12, 2 + type * 2, 1});
    addKey(new GraphKey{m_config, QStringLiteral("zoom"), QStringLiteral("format"),
                        QStringLiteral("f3"), type ? 11 : 13, 2 + type * 2, type ? 1 : 5});
    addKey(new GraphKey{m_config, type ? QStringLiteral("fenêtre") : QStringLiteral("window"),
                        type ? QStringLiteral("déf")+quart_space+QStringLiteral("table") : QStringLiteral("tblset"),
                        QStringLiteral("f2"), 15 - type, type ? 6 : 2, 4 - type});
    addKey(new GraphKey{m_config, type ? QStringLiteral("f(x)") : QStringLiteral("y="),
                        type ? QStringLiteral("graph")+quart_space+QStringLiteral("stats") : QStringLiteral("stat")+quart_space+QStringLiteral("plot"),
                        QStringLiteral("f1"), 6 + type, type ? 6 : 2, type ? 10 : 8});
    addKey(new SecondKey{m_config, type ? QStringLiteral("2nde") : QStringLiteral("2nd")});
    addKey(new OtherKey{m_config, 16 - type * 2, 45, 37, QStringLiteral("mode"),
                        type ? QStringLiteral("quitter") : QStringLiteral("quit")});
    addKey(new OtherKey{m_config, type ? 14 : 8, 72, 37, type ? QStringLiteral("suppr") : QStringLiteral("del"),
                        type ? QStringLiteral("insérer") : QStringLiteral("ins")});
    addKey(new OtherKey{m_config, 7, QStringLiteral("on"), QStringLiteral("off")});
    addKey(new OtherKey{m_config, 13, QStringLiteral("sto→"), type ? QStringLiteral("rappel") :
                        QStringLiteral("rcl"), QStringLiteral("X"), type * 2, type * 3});
    addKey(new OtherKey{m_config, 7, QStringLiteral("ln"), QStringLiteral("eˣ"), QStringLiteral("S"), type * 2, type * 2});
    addKey(new OtherKey{m_config, 9, QStringLiteral("log"), QStringLiteral("10ˣ"), QStringLiteral("N"), type * 2, type * 3});
    addKey(new OtherKey{m_config, 6, QStringLiteral("x²"), QStringLiteral("√‾‾"), QStringLiteral("I"), type, type * 3});
    addKey(new OtherKey{m_config, type ? 6 : 8, type ? QStringLiteral("◀ ▶") : QStringLiteral("x⁻¹"), type ?
                        QStringLiteral("angle") : QStringLiteral("matrix"), QStringLiteral("D"), type * 2, type ? 2 : 4});
    addKey(new OtherKey{m_config, 14, QStringLiteral("math"), type ? QStringLiteral("tests") :
                        QStringLiteral("test"), QStringLiteral("A"), type * 2, type * 2});
    addKey(new AlphaKey{m_config, type ? QStringLiteral("verr A") : QStringLiteral("A-lock")});
    addKey(new NumKey{m_config, QStringLiteral("0"), QStringLiteral("catalog"), QStringLiteral("⎵"), type * 2, 6});
    addKey(new NumKey{m_config, QStringLiteral("1"), QStringLiteral("L1"), QStringLiteral("Y"), type * 2, type ? 3 : 1});
    addKey(new NumKey{m_config, QStringLiteral("4"), QStringLiteral("L4"), QStringLiteral("T"), type * 2, type ? 3 : 1});

#ifdef _WIN32
    addKey(new NumKey{m_config, QStringLiteral("7"), type ? QStringLiteral("un") : QStringLiteral("u"),
                      QStringLiteral("O"), type * 2, 1 + type});
#else
    addKey(new NumKey{m_config, QStringLiteral("7"), type ? QStringLiteral("uₙ") : QStringLiteral("u"),
                      QStringLiteral("O"), type * 2, 1 + type});
#endif
    addKey(new OtherKey{m_config, 2, QStringLiteral(","), QStringLiteral("EE"), QStringLiteral("J"), type * 2, 1 + type});
    addKey(new OtherKey{m_config, 8 + type, type ? QStringLiteral("trig") : QStringLiteral("sin"), type ?
                        QStringLiteral("π") : QStringLiteral("sin⁻¹"), QStringLiteral("E"), type * 2, 1});
    addKey(new OtherKey{m_config, 14 + type, type ? QStringLiteral("matrice") : QStringLiteral("apps"), type ?
                        QStringLiteral("x⁻¹") : QStringLiteral("angle"), QStringLiteral("B"), type * 2, 1});
#ifdef _WIN32
    addKey(new OtherKey{m_config, 15 + type, QStringLiteral("X,T,θ,n"),
                        type ? QStringLiteral("échanger") : QStringLiteral("link"), QString{}, type * 2, type * 3});
#else
    addKey(new OtherKey{m_config, 15 + type, QStringLiteral("X,T,θ,n"),
                        type ? QStringLiteral("échanger") : QStringLiteral("link"), QString{}, 1, 1});
#endif
    addKey(new NumKey{m_config, type * 2});
    addKey(new NumKey{m_config, QStringLiteral("2"), QStringLiteral("L2"), QStringLiteral("Z"), type * 2, type * 3});
    addKey(new NumKey{m_config, QStringLiteral("5"), QStringLiteral("L5"), QStringLiteral("U"), type * 2, type * 3});
#ifdef _WIN32
    addKey(new NumKey{m_config, QStringLiteral("8"), type ? QStringLiteral("vn") : QStringLiteral("v"),
                      QStringLiteral("P"), type * 2, type * 2});
#else
    addKey(new NumKey{m_config, QStringLiteral("8"), type ? QStringLiteral("vₙ") : QStringLiteral("v"),
                      QStringLiteral("P"), type * 2, type * 2});
#endif
    addKey(new OtherKey{m_config, 3, QStringLiteral("("), QStringLiteral("{"), QStringLiteral("K"), type * 2, type});
    addKey(new OtherKey{m_config, type ? 12 : 9, type ? QStringLiteral("résol") : QStringLiteral("cos"), type ?
                        QStringLiteral("apps") : QStringLiteral("cos⁻¹"), QStringLiteral("F"), type * 2, type * 2});
    addKey(new OtherKey{m_config, 14, QStringLiteral("prgm"), type ? QStringLiteral("dessin") :
                        QStringLiteral("draw"), QStringLiteral("C"), type * 2, type * 2});
    addKey(new OtherKey{m_config, 11 + type, type ? QStringLiteral("stats") : QStringLiteral("stat"),
                        type ? QStringLiteral("listes") : QStringLiteral("list")});
    addKey(new NumKey{m_config, QStringLiteral("(-)"), type ? QStringLiteral("rép") :
                      QStringLiteral("ans"), QStringLiteral("?"), type * 2, type * 3, 11});
    addKey(new NumKey{m_config, QStringLiteral("3"), QStringLiteral("L3"), QStringLiteral("θ"), type * 2, type * 3});
    addKey(new NumKey{m_config, QStringLiteral("6"), QStringLiteral("L6"), QStringLiteral("V"), type * 2, type * 3});
#ifdef _WIN32
    addKey(new NumKey{m_config, QStringLiteral("9"), type ? QStringLiteral("wn") : QStringLiteral("w"),
                      QStringLiteral("Q"), type * 2, type * 3});
#else
    addKey(new NumKey{m_config, QStringLiteral("9"), type ? QStringLiteral("wₙ") : QStringLiteral("w"),
                      QStringLiteral("Q"), type * 2, type * 3});
#endif
    addKey(new OtherKey{m_config, 3, QStringLiteral(")"), QStringLiteral("}"), QStringLiteral("L"), type * 2, type});
#ifdef _WIN32
    addKey(new OtherKey{m_config, 9, type ? QStringLiteral("⸋|⸋") : QStringLiteral("tan"), type ?
                        QStringLiteral("∫⸋|⸋d▫‣") : QStringLiteral("tan⁻¹"), QStringLiteral("G"), type * 2, type * 2});
#else
    addKey(new OtherKey{m_config, 9, type ? QStringLiteral("  ⸋ ̵̻ ") : QStringLiteral("tan"), type ?
                        QStringLiteral("∫⸋̻◻d▫‣") : QStringLiteral("tan⁻¹"), QStringLiteral("G"), type * 2, type * 2});
#endif
    addKey(new OtherKey{m_config, type ? 9 : 12, type ? QStringLiteral("var") : QStringLiteral("vars"), type ?
                        QStringLiteral("distrib") : QStringLiteral("distr"), QStringLiteral(""), 0, type});
    m_config.next();
    addKey(new OperKey{m_config, type ? QStringLiteral("entrer") : QStringLiteral("enter"),
                        type ? QStringLiteral("précéd") : QStringLiteral("entry"),
                        type ? QString{} : QStringLiteral("solve"), 6, type ? 0 : 5, {16, 5}});
    addKey(new OperKey{m_config, QStringLiteral("+"), type ? QStringLiteral("mém") :
                       QStringLiteral("mem"), QStringLiteral("“"), type * 2, type * 5});
    addKey(new OperKey{m_config, QStringLiteral("–"), QStringLiteral("]"), QStringLiteral("W"), type * 2, type * 2});
    addKey(new OperKey{m_config, QStringLiteral("×"), QStringLiteral("["), QStringLiteral("R"), type * 2, type * 2});
    addKey(new OperKey{m_config, QStringLiteral("÷"), QStringLiteral("e"), QStringLiteral("M"), type * 2, type * 2});
    addKey(new OtherKey{m_config, type ? QString{} : QStringLiteral("π"), type * 2});
    addKey(new OtherKey{m_config, 15, type ? QStringLiteral("annul") : QStringLiteral("clear")});
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
                          origin.width(), origin.height(), 1);
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
