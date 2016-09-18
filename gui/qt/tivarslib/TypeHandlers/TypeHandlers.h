/*
 * Part of tivars_lib_cpp
 * (C) 2016 Adrien 'Adriweb' Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#ifndef TYPE_HANDLERS_H
#define TYPE_HANDLERS_H

#include "../utils_tivarslib.h"

namespace tivars
{
#define th()    data_t      makeDataFromString(const std::string& str,  const options_t options = {}); \
                std::string makeStringFromData(const data_t& data,      const options_t options = {})

    namespace DummyHandler
    { th(); }

    namespace TH_0x00   // Real
    {
        th();
        const constexpr size_t dataByteCount = 9;
    }

    namespace TH_0x01   // Real list
    { th(); }

    namespace TH_0x02   // Matrix
    { th(); }

    namespace TH_0x05   // Program
    {
        th();
        std::string reindentCodeString(const std::string& str_orig);
        void initTokens();
    }

    /* The following ones use the same handlers as 0x05 */
    namespace TH_0x03 = TH_0x05; // Y-Variable
    namespace TH_0x04 = TH_0x05; // String
    namespace TH_0x06 = TH_0x05; // Protected Program

#undef th


typedef decltype(&DummyHandler::makeDataFromString) dataFromString_handler_t;
typedef decltype(&DummyHandler::makeStringFromData) stringFromData_handler_t;
typedef std::pair<dataFromString_handler_t, stringFromData_handler_t> handler_pair_t;

#define make_handler_pair(cls)   make_pair(&cls::makeDataFromString, &cls::makeStringFromData)

}

#endif
