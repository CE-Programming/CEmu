#ifndef SECONDKEY_H
#define SECONDKEY_H

#include "rectkey.h"

class SecondKey : public RectKey {
public:
    SecondKey(KeyConfig &config, QString labelText)
        : RectKey{config, {18, 28, 18, 9}, {18, 37, 18, 12}, {10, 5},
                  config.secondColor, config.whiteColor, labelText, {}, {}} { }
};

#endif
