/*
 * Part of tivars_lib_cpp
 * (C) 2015-2019 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "TypeHandlers.h"
#include "../tivarslib_utils.h"

#include <unordered_map>

namespace tivars
{
    static const std::unordered_map<uchar, handler_pair_t> type2handlers = {
        { 0x00, make_handler_pair(STH_FP)              },
        { 0x18, make_handler_pair(STH_ExactFraction)   },
        { 0x1C, make_handler_pair(STH_ExactRadical)    },
        { 0x20, make_handler_pair(STH_ExactPi)         },
        { 0x21, make_handler_pair(STH_ExactFractionPi) },
    };

    // TODO: guess, by parsing, the type instead of reading it from the options
    data_t TH_GenericReal::makeDataFromString(const std::string& str, const options_t& options)
    {
        const auto& typeIter = options.find("_type");
        if (typeIter == options.end())
        {
            throw std::runtime_error("Needs _type in options for TH_GenericReal::makeDataFromString");
        }
        const uchar type = (uchar)typeIter->second;
        const auto& handlerIter = type2handlers.find(type);
        if (handlerIter == type2handlers.end())
        {
            throw std::runtime_error("Unknown/Invalid type for this TH_GenericReal: " + std::to_string(type));
        }
        return (handlerIter->second.first)(str, options);
    }

    std::string TH_GenericReal::makeStringFromData(const data_t& data, const options_t& options)
    {
        if (data.size() != dataByteCount)
        {
            throw std::invalid_argument("Invalid data array. Needs to contain " + std::to_string(dataByteCount) + " bytes");
        }
        const uchar type = (uchar)(data[0] & 0x7F);
        const auto& handlerIter = type2handlers.find(type);
        if (handlerIter == type2handlers.end())
        {
            throw std::runtime_error("Unknown/Invalid type in this TH_GenericReal data: " + std::to_string(type));
        }
        return (handlerIter->second.second)(data, options);
    }
}
