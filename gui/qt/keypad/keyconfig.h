#ifndef KEYCONFIG_H
#define KEYCONFIG_H

#include "keycode.h"

#include <QtGui/QFont>
#include <QtGui/QColor>

struct KeyConfig {
    QFont labelFont, secondFont, alphaFont;
    QColor secondColor, alphaColor, graphColor, numColor, otherColor,
        blackColor, whiteColor, textColor;
    KeyCode key;
    KeyCode next() {
        KeyCode old = key;
        key = old.code() + 1;
        return old;
    }
};

#endif
