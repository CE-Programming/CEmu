#ifndef ALPHAKEY_H
#define ALPHAKEY_H

#include "rectkey.h"

class AlphaKey : public RectKey {
public:
    AlphaKey(KeyConfig &config, QString secondText)
        : RectKey{config, {18, 49, 18, 9}, {18, 58, 18, 12}, {15, 5},
                  config.alphaColor, config.whiteColor,
              #ifdef Q_OS_MACX
                  QStringLiteral(" alpha"),
              #else
                  QStringLiteral("alpha"),
              #endif
                  secondText, {},
                  Qt::AlignCenter, Qt::AlignCenter} {}
};

#endif
