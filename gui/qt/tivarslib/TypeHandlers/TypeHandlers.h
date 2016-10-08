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

#define th()    data_t      makeDataFromString(const std::string& str,  const options_t& options = options_t()); \
                std::string makeStringFromData(const data_t& data,      const options_t& options = options_t())

    namespace DummyHandler { th(); }


    namespace TH_0x00 { th(); }  // Real

    namespace TH_0x01 { th(); }  // Real list

    namespace TH_0x02 { th(); }  // Matrix

    namespace TH_0x05 { th(); }  // Program

    /* The following ones use the same handlers as 0x05 */
    namespace TH_0x03 = TH_0x05; // Y-Variable
    namespace TH_0x04 = TH_0x05; // String
    namespace TH_0x06 = TH_0x05; // Protected Program

    namespace TH_0x0C { th(); }  // Complex

    namespace TH_0x0D { th(); }  // Complex list

    namespace TH_0x1B { th(); }  // Exact Complex Fraction

    namespace TH_0x1C { th(); }  // Exact Real Radical

    namespace TH_0x1D { th(); }  // Exact Complex Radical

    namespace TH_0x1E { th(); }  // Exact Complex Pi

    namespace TH_0x1F { th(); }  // Exact Complex Pi Fraction

    namespace TH_0x20 { th(); }  // Exact Real Pi

    namespace TH_0x21 { th(); }  // Exact Real Pi Fraction

#undef th


    /* Additional things */

    namespace TH_0x00   // Real
    {
        const constexpr size_t dataByteCount = 9;
        const std::string validPattern = "([-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]{1,2})?)";
    }

    namespace TH_0x05   // Program
    {
        std::string reindentCodeString(const std::string& str_orig);
        void initTokens();
    }

    namespace TH_0x0C   // Complex
    {
        const constexpr size_t dataByteCount = 2 * TH_0x00::dataByteCount;
        bool checkValidString(const std::string& str);
        bool checkValidStringAndGetMatches(const std::string& str, std::smatch& matches);
    }

    namespace TH_0x1B   // Exact Complex Fraction
    {
        const constexpr size_t dataByteCount = 2 * TH_0x00::dataByteCount; // 18
    }

    namespace TH_0x1C   // Exact Real Radical
    {
        const constexpr size_t dataByteCount =     TH_0x00::dataByteCount; // 9;
    }

    namespace TH_0x1D   // Exact Complex Radical
    {
        const constexpr size_t dataByteCount = 2 * TH_0x1C::dataByteCount; // 18
    }

    namespace TH_0x1E   // Exact Complex Pi
    {
        const constexpr size_t dataByteCount = 2 * TH_0x00::dataByteCount; // 18
    }

    namespace TH_0x1F   // Exact Complex Pi Fraction
    {
        const constexpr size_t dataByteCount = 2 * TH_0x00::dataByteCount; // 18
    }

    namespace TH_0x20   // Exact Real Pi
    {
        const constexpr size_t dataByteCount =     TH_0x00::dataByteCount; // 9;
    }

    namespace TH_0x21   // Exact Real Pi Fraction
    {
        const constexpr size_t dataByteCount =     TH_0x00::dataByteCount; // 9;
    }


    typedef decltype(&DummyHandler::makeDataFromString) dataFromString_handler_t;
    typedef decltype(&DummyHandler::makeStringFromData) stringFromData_handler_t;
    typedef std::pair<dataFromString_handler_t, stringFromData_handler_t> handler_pair_t;

#define make_handler_pair(cls)   make_pair(&cls::makeDataFromString, &cls::makeStringFromData)

}

#endif
