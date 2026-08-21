#include "rectkey.h"

#include <QtCore/QHash>
#include <QtCore/QRegularExpression>

namespace {

enum LabelPosition {
    LabelMain,
    LabelSecond,
    LabelAlpha,
    LabelCount,
    LabelUnknown = -1,
};

QString normalizedLabel(QString text) {
    text = text.toCaseFolded();
    text.replace(QChar{0x2014}, QLatin1Char('-'));
    text.replace(QChar{0x2212}, QLatin1Char('-'));
    text.replace(QChar{0x00d7}, QLatin1Char('*'));
    text.replace(QChar{0x00f7}, QLatin1Char('/'));
    text.replace(QChar{0x201c}, QLatin1Char('"'));
    text.replace(QChar{0x201d}, QLatin1Char('"'));
    text.remove(QRegularExpression{QStringLiteral("[\\s\\x{2000}-\\x{200f}]")});
    return text;
}

bool bindingMatchesLabel(const QString &binding, const QString &label) {
    const QString key = normalizedLabel(binding);
    const QString legend = normalizedLabel(label);
    if (key == legend) {
        return true;
    }

    if ((key == QStringLiteral("return") || key == QStringLiteral("enter")) &&
        (legend.contains(QStringLiteral("enter")) || legend.contains(QStringLiteral("entrer")))) {
        return true;
    }
    if (key == QStringLiteral("escape") &&
        (legend.contains(QStringLiteral("clear")) || legend.contains(QStringLiteral("annul")))) {
        return true;
    }
    if (key == QStringLiteral("delete") &&
        (legend.contains(QStringLiteral("del")) || legend.contains(QStringLiteral("suppr")))) {
        return true;
    }
    if (key == QStringLiteral("insert") && legend.contains(QStringLiteral("ins"))) {
        return true;
    }
    if (key == QStringLiteral("home") && legend == QStringLiteral("mode")) {
        return true;
    }
    if (key == QStringLiteral("backspace") &&
        (legend == QStringLiteral("quit") || legend == QStringLiteral("quitter"))) {
        return true;
    }
    if (key == QStringLiteral("pageup") &&
        (legend.contains(QStringLiteral("apps")) || legend.contains(QStringLiteral("matrice")))) {
        return true;
    }
    if (key == QStringLiteral("pagedown") && legend.contains(QStringLiteral("prgm"))) {
        return true;
    }
    if (key == QStringLiteral("end") && legend.contains(QStringLiteral("stat"))) {
        return true;
    }
    if ((key == QStringLiteral("=") || key == QStringLiteral("equal")) &&
        (legend == QStringLiteral("test") || legend == QStringLiteral("tests"))) {
        return true;
    }
    if ((key == QStringLiteral("tab") || key == QStringLiteral(";") || key == QStringLiteral("semicolon")) &&
        (legend == QStringLiteral("2nd") || legend == QStringLiteral("2nde"))) {
        return true;
    }
    if ((key == QStringLiteral("'") || key == QStringLiteral("apostrophe")) &&
        legend == QStringLiteral("alpha")) {
        return true;
    }
    if ((key == QStringLiteral(">") || key == QStringLiteral("greater")) && legend.startsWith(QStringLiteral("sto"))) {
        return true;
    }

    static const QHash<QString, QStringList> functionKeyLegends = {
        {QStringLiteral("f6"), {QStringLiteral("math")}},
        {QStringLiteral("f7"), {QStringLiteral("apps"), QStringLiteral("matrice")}},
        {QStringLiteral("f8"), {QStringLiteral("prgm")}},
        {QStringLiteral("f9"), {QStringLiteral("vars"), QStringLiteral("var")}},
        {QStringLiteral("f10"), {QStringLiteral("stat"), QStringLiteral("stats")}},
        {QStringLiteral("f11"), {QStringLiteral("mode")}},
        {QStringLiteral("f12"), {QStringLiteral("on")}},
    };
    const auto functionLegends = functionKeyLegends.constFind(key);
    return functionLegends != functionKeyLegends.cend() && functionLegends->contains(legend);
}

QFont fittedFont(QFont font, const QString &text, const QRect &bounds) {
    if (text.isEmpty()) {
        return font;
    }
    const int maximumPixelSize = qMax(1, font.pixelSize());
    for (int pixelSize = maximumPixelSize; pixelSize > 1; --pixelSize) {
        font.setPixelSize(pixelSize);
        if (QFontMetricsF{font}.horizontalAdvance(text) <= bounds.width() - 1) {
            break;
        }
    }
    return font;
}

}

RectKey::RectKey(KeyCode keycode, const QRect &textGeometry, const QRect &keyGeometry,
                 const QSize &labelSize,
                 int topLeft, int topRight, int bottomLeft, int bottomRight,
                 const QColor &keyColor, const QColor &textColor, const QColor &secondColor, const QColor &alphaColor,
                 const QString &labelText, const QString &secondText, const QString &alphaText,
                 const QFont &labelFont, const QFont &secondFont, const QFont &alphaFont,
                 Qt::Alignment labelAlign, Qt::Alignment secondAlign, Qt::Alignment alphaAlign)
    : Key{keycode, textGeometry, keyGeometry, keyColor},
      mTextColor{textColor}, mSecondColor{secondColor}, mAlphaColor{alphaColor},
      mLabelAlign{labelAlign}, mSecondAlign{secondAlign}, mAlphaAlign{alphaAlign},
      mLabelFont{labelFont}, mSecondFont{secondFont.resolve(labelFont)},
                              mAlphaFont{alphaFont.resolve(labelFont)},
      mSecondText{secondText}, mAlphaText{alphaText} {
    QRect corner;
    mLabelText = labelText;
    mKeyShape.moveTo(keyGeometry.topLeft() + QPointF{0, topLeft * .5});
    corner.setSize({bottomLeft, bottomLeft});
    corner.moveBottomLeft(keyGeometry.bottomLeft());
    mKeyShape.arcTo(corner, 90 * 2, 90);
    corner.setSize({bottomRight, bottomRight});
    corner.moveBottomRight(keyGeometry.bottomRight());
    mKeyShape.arcTo(corner, 90 * 3, 90);
    corner.setSize({topRight, topRight});
    corner.moveTopRight(keyGeometry.topRight());
    mKeyShape.arcTo(corner, 90 * 0, 90);
    corner.setSize({topLeft, topLeft});
    corner.moveTopLeft(keyGeometry.topLeft());
    mKeyShape.arcTo(corner, 90 * 1, 90);

    mLabelFont.setPixelSize(labelSize.height());
    mLabelFont.setStretch(labelSize.width() * mLabelFont.stretch() /
                           QFontMetricsF(mLabelFont).size(Qt::TextSingleLine, mLabelText).width());
}

void RectKey::paint(QPainter &painter) const {
    Key::paint(painter);
    const QString &labelText = mMappingLabelsVisible ? mMappingLabelText : mLabelText;
    const QString &secondText = mMappingLabelsVisible ? mMappingSecondText : mSecondText;
    const QString &alphaText = mMappingLabelsVisible ? mMappingAlphaText : mAlphaText;

    painter.setFont(mMappingLabelsVisible ? fittedFont(mLabelFont, labelText, keyGeometry()) : mLabelFont);
    painter.setPen(mTextColor);
    painter.drawText(keyGeometry(), mLabelAlign, labelText);
    if (!secondText.isEmpty()) {
        painter.setPen(mSecondColor);
        QRect secondGeometry = textGeometry();
        if (mMappingLabelsVisible && !alphaText.isEmpty()) {
            secondGeometry.setWidth(secondGeometry.width() / 2);
        }
        painter.setFont(mMappingLabelsVisible ? fittedFont(mSecondFont, secondText, secondGeometry) : mSecondFont);
        painter.drawText(secondGeometry, mSecondAlign, secondText);
    }
    if (!alphaText.isEmpty()) {
        painter.setPen(mAlphaColor);
        QRect alphaGeometry = textGeometry();
        if (mMappingLabelsVisible && !secondText.isEmpty()) {
            const int halfWidth = alphaGeometry.width() / 2;
            alphaGeometry.setLeft(alphaGeometry.left() + halfWidth);
        }
        painter.setFont(mMappingLabelsVisible ? fittedFont(mAlphaFont, alphaText, alphaGeometry) : mAlphaFont);
        painter.drawText(alphaGeometry, mAlphaAlign, alphaText);
    }
}

void RectKey::setMappingLabels(const QStringList &bindings, bool visible) {
    mMappingLabelsVisible = visible;
    mMappingLabelText.clear();
    mMappingSecondText.clear();
    mMappingAlphaText.clear();
    if (!visible) {
        return;
    }

    const QString legends[LabelCount] = {mLabelText, mSecondText, mAlphaText};
    QStringList projected[LabelCount];
    QStringList unmatched;

    for (const QString &binding : bindings) {
        int position = LabelUnknown;
        for (int candidate = LabelMain; candidate < LabelCount; ++candidate) {
            if (!legends[candidate].isNull() && bindingMatchesLabel(binding, legends[candidate])) {
                position = candidate;
                break;
            }
        }
        if (position == LabelUnknown) {
            unmatched.append(binding);
        } else if (!projected[position].contains(binding)) {
            projected[position].append(binding);
        }
    }

    for (const QString &binding : unmatched) {
        int position = LabelUnknown;
        for (int candidate = LabelMain; candidate < LabelCount; ++candidate) {
            if (!legends[candidate].isNull() && projected[candidate].isEmpty()) {
                position = candidate;
                break;
            }
        }
        if (position == LabelUnknown) {
            position = LabelMain;
        }
        if (!projected[position].contains(binding)) {
            projected[position].append(binding);
        }
    }

    mMappingLabelText = projected[LabelMain].join(QStringLiteral(" / "));
    mMappingSecondText = projected[LabelSecond].join(QStringLiteral(" / "));
    mMappingAlphaText = projected[LabelAlpha].join(QStringLiteral(" / "));
}

bool RectKey::isUnder(const QPainterPath &area) const {
    return mKeyShape.intersects(area);
}
