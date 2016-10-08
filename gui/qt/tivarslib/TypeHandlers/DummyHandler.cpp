/*
 * Part of tivars_lib_cpp
 * (C) 2016 Adrien 'Adriweb' Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "../autoloader.h"

namespace tivars
{

    data_t DummyHandler::makeDataFromString(const std::string& str, const options_t& options)
    {
        (void)str;
        (void)options;

        std::cerr << "This type is not supported / implemented (yet?)" << std::endl;
        return data_t();
    }

    std::string DummyHandler::makeStringFromData(const data_t& data, const options_t& options)
    {
        (void)data;
        (void)options;

        std::cerr << "This type is not supported / implemented (yet?)" << std::endl;
        return "";
    }

}
