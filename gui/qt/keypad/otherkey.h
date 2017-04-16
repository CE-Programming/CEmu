#ifndef OTHERKEY_H
#define OTHERKEY_H

#include "rectkey.h"

class OtherKey : public RectKey {
public:
    OtherKey(KeyConfig &config, QString secondText, int right)
        : OtherKey{config, 13,
#ifdef Q_OS_MACX
    QStringLiteral("  ^ "),
#else
    QStringLiteral("^"),
#endif
        secondText, QStringLiteral("H"), right, 0, 12} {
        mLabelFont.setBold(false);
    }
    OtherKey(KeyConfig &config, int labelWidth,
             QString labelText, QString secondText = {}, QString alphaText = {},
             int right = 0, int left = 0, int labelHeight = 5)
        : OtherKey{config, labelWidth, config.key.row() * 27 - 36,
                   205 - 21 * config.key.col(),
                   labelText, secondText, alphaText, right, left, labelHeight} { }
    OtherKey(KeyConfig &config, int labelWidth, int x, int y,
             QString labelText, QString secondText, QString alphaText = {},
             int right = 0, int left = 0, int labelHeight = 5)
        : RectKey{config, {x - left, y - 9, left + 18 + right, 9}, {x, y, 18, 12},
                  {labelWidth, labelHeight}, 4, 4, 4, 4,
                  config.otherColor, config.textColor, labelText, secondText, alphaText,
                  Qt::AlignHCenter | (labelText.contains("^") ? Qt::AlignTop : Qt::AlignVCenter),
                  Qt::AlignVCenter | (alphaText.isNull() ? labelText == "on" ?
                  Qt::AlignRight : Qt::AlignHCenter : Qt::AlignLeft)} { }
};

#endif
