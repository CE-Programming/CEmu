/*
 * Part of tivars_lib_cpp
 * (C) 2015 Adrien 'Adriweb' Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#ifndef TH_0x01_H
#define TH_0x01_H

#include "ITIVarTypeHandler.h"

namespace tivars
{

    // Type Handler for type 0x01: Real list
    class TH_0x01 : public ITIVarTypeHandler
    {
    public:

        static data_t makeDataFromString(const std::string& str, const options_t options = {});

        static std::string makeStringFromData(const data_t& data, const options_t options = {});

    };
}

#endif