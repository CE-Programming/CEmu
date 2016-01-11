#ifndef GRAPHKEY_H
#define GRAPHKEY_H

#include "rectkey.h"

class GraphKey : public RectKey {
    GraphKey(KeyConfig &config, int x, int leftCorners, int rightCorners,
             QString labelText, QString secondText, QString alphaText,
             int labelWidth, int right, int left)
        : RectKey{config, {x - left, 5, left + 18 + right, 9}, {x, 14, 18, 9},
                  {labelWidth, 5}, leftCorners, rightCorners,
                  leftCorners, rightCorners, config.graphColor, config.blackColor,
                  labelText, secondText, alphaText} { }
public:
    GraphKey(KeyConfig &config, QString labelText, QString secondText, QString alphaText,
             int labelWidth, int right = 2, int left = 0)
        : GraphKey{config, 126 - 27 * config.key.col(), config.key.col() != 4 ? 4 : 8,
                   config.key.col() ? 4 : 8, labelText, secondText, alphaText, labelWidth,
                   right, left} { }
};

#endif
