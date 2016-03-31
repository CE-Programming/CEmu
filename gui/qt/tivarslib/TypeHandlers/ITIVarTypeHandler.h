/*
 * Part of tivars_lib_cpp
 * (C) 2015 Adrien 'Adriweb' Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#ifndef ITIVARTYPEHANDLER_H
#define ITIVARTYPEHANDLER_H

#include "../utils_tivarslib.h"

namespace tivars
{
    class ITIVarTypeHandler
    {

    public:

        // We can't make virtual static methods...

        static data_t makeDataFromString(const std::string& str, const options_t options)
        {
            (void)str;
            (void)options;
            std::cerr << "This type is not supported / implemented (yet?)" << std::endl;
            return data_t();
        }

        static std::string makeStringFromData(const data_t& data, const options_t options)
        {
            (void)data;
            (void)options;
            std::cerr << "This type is not supported / implemented (yet?)" << std::endl;
            return "";
        }

    };
}

#endif
