#ifndef NUMKEY_H
#define NUMKEY_H

#include "rectkey.h"

class NumKey : public RectKey {
    NumKey(KeyConfig &config, int x, int y, QString labelText, QString secondText,
           QString alphaText, int right, int left, int width, int height,
           int topLeft = 4, int topRight = 4, int bottomLeft = 4, int bottomRight = 4)
        : RectKey{config, {x - left, y - 9, left + 18 + right, 9}, {x, y, 18, 14},
                  {width, height}, topLeft, topRight, bottomLeft, bottomRight,
                  config.numColor, config.blackColor, labelText, secondText, alphaText} { }
public:
    NumKey(KeyConfig &config, int right)
        : NumKey{config,
             #ifdef Q_OS_MACX
                 QStringLiteral(" ."),
             #else
                 QStringLiteral("."),
             #endif
                 QStringLiteral("i"), QStringLiteral(":"),
                 right, 0, 4} {
        m_secondFont.setStyle(QFont::StyleItalic);
    }
    NumKey(KeyConfig &config, QString labelText, QString secondText, QString alphaText,
           int right = 0, int left = 0, int width = 6, int height = 10)
        : NumKey{config, config.key.row() * 27 - 36, 211 - 23 * config.key.col(),
                 labelText, secondText, alphaText, right, left, width, height,
                 config.key != KeyCode{3, 3} ? 4 : 8, config.key != KeyCode{5, 3} ? 4 : 8,
                 config.key != KeyCode{3, 0} ? 4 : 8, config.key != KeyCode{5, 0} ? 4 : 8} { }
};

#endif
