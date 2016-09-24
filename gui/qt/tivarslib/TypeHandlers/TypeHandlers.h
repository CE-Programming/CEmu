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
#define th()    data_t      makeDataFromString(const std::string& str,  const options_t options = options_t()); \
                std::string makeStringFromData(const data_t& data,      const options_t options = options_t())

    namespace DummyHandler
    { th(); }

    namespace TH_0x00   // Real
    { th(); }

    namespace TH_0x01   // Real list
    { th(); }

    namespace TH_0x02   // Matrix
    { th(); }

    namespace TH_0x05   // Program
    { th(); }

    /* The following ones use the same handlers as 0x05 */
    namespace TH_0x03 = TH_0x05; // Y-Variable
    namespace TH_0x04 = TH_0x05; // String
    namespace TH_0x06 = TH_0x05; // Protected Program

    namespace TH_0x0C   // Complex
    { th(); }

    namespace TH_0x0D   // Complex list
    { th(); }

#undef th


    /* Additional things */

    namespace TH_0x00
    {
        const constexpr size_t dataByteCount = 9;
        const std::string validPattern = "([-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]{1,2})?)";
    }

    namespace TH_0x05   // Program
    {
        std::string reindentCodeString(const std::string& str_orig);
        void initTokens();
    }

    namespace TH_0x0C
    {
        const constexpr size_t dataByteCount = 2 * TH_0x00::dataByteCount;
        bool checkValidString(const std::string& str);
        bool checkValidStringAndGetMatches(const std::string& str, std::smatch& matches);
    }

typedef decltype(&DummyHandler::makeDataFromString) dataFromString_handler_t;
typedef decltype(&DummyHandler::makeStringFromData) stringFromData_handler_t;
typedef std::pair<dataFromString_handler_t, stringFromData_handler_t> handler_pair_t;

#define make_handler_pair(cls)   make_pair(&cls::makeDataFromString, &cls::makeStringFromData)

}

#endif
