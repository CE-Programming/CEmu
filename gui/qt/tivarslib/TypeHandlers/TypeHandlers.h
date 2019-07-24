/*
 * Part of tivars_lib_cpp
 * (C) 2015-2019 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#ifndef TYPE_HANDLERS_H
#define TYPE_HANDLERS_H

#include "../CommonTypes.h"

namespace tivars
{

#define th()    data_t      makeDataFromString(const std::string& str,  const options_t& options = options_t()); \
                std::string makeStringFromData(const data_t& data,      const options_t& options = options_t())

    namespace DummyHandler { th(); }

    namespace STH_FP
    {
        th();
        const constexpr size_t dataByteCount = 9;
        static const constexpr char* validPattern = "([-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]{1,2})?)";
    }
    namespace STH_ExactFraction
    {
        th();
        const constexpr size_t dataByteCount = 9;
        static const constexpr char* validPattern = "===UNIMPLEMENTED==="; // TODO
    }
    namespace STH_ExactRadical
    {
        th();
        const constexpr size_t dataByteCount = 9;
        static const constexpr char* validPattern = "===UNIMPLEMENTED==="; // TODO
    }
    namespace STH_ExactPi
    {
        th();
        const constexpr size_t dataByteCount = 9;
        static const constexpr char* validPattern = "===UNIMPLEMENTED==="; // TODO
    }
    namespace STH_DataAppVar
    {
        th();
    }
    namespace STH_PythonAppVar
    {
        th();
        static const constexpr char ID_SCRIPT[] = "PYSC";
        static const constexpr char ID_CODE[] = "PYCD";
    }
    namespace STH_ExactFractionPi
    {
        th();
        const constexpr size_t dataByteCount = 9;
        static const constexpr char* validPattern = "===UNIMPLEMENTED==="; // TODO
    }

    namespace TH_GenericReal
    {
        th();
        const constexpr size_t dataByteCount = 9;
    }
    namespace TH_GenericComplex
    {
        th();
        const constexpr size_t dataByteCount = 2 * TH_GenericReal::dataByteCount;
    }

    namespace TH_GenericList   { th(); }

    namespace TH_Matrix        { th(); }

    namespace TH_GenericAppVar { th(); }

    // Program, Protected Program, Y-Variable, String
    namespace TH_Tokenized
    {
        th();
        enum lang { LANG_EN = 0, LANG_FR };
        enum typelang { PRGMLANG_BASIC = 0, PRGMLANG_AXE, PRGMLANG_ICE };
        std::string reindentCodeString(const std::string& str_orig, const options_t& options = options_t());
        void initTokens();
    }

    // Special temporary type that may appear as an equation, during basic program execution
    namespace TH_TempEqu     { th(); }

#undef th


    using dataFromString_handler_t = decltype(&DummyHandler::makeDataFromString);
    using stringFromData_handler_t = decltype(&DummyHandler::makeStringFromData);
    using handler_pair_t           = std::pair<dataFromString_handler_t, stringFromData_handler_t>;

#define make_handler_pair(cls)   make_pair(&cls::makeDataFromString, &cls::makeStringFromData)

}

#endif
