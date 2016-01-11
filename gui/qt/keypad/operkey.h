#ifndef OPERKEY_H
#define OPERKEY_H

#include "rectkey.h"

class OperKey : public RectKey {
    OperKey(KeyConfig &config, QString labelText, QString secondText, QString alphaText,
            int right, int left, QSize labelSize, int y)
        : RectKey{config, {126 - left, y - 9, left + 18 + right, 9}, {126, y, 18, 12},
                  labelSize, 4, 4, 4, 4, config.graphColor, config.blackColor,
                  labelText, secondText, alphaText} { }
public:
    OperKey(KeyConfig &config, QString labelText, QString secondText, QString alphaText,
            int right = 2, int left = 0, QSize labelSize = {8, 10})
        : OperKey{config, labelText, secondText, alphaText, right, left, labelSize,
                  205 - 21 * config.key.col()} { }
};

#endif
