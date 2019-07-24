/*
 * Part of tivars_lib_cpp
 * (C) 2015-2019 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "TypeHandlers.h"
#include "../tivarslib_utils.h"
#include "../TIVarTypes.h"
#include <regex>

namespace tivars
{

    data_t STH_ExactRadical::makeDataFromString(const std::string& str, const options_t& options)
    {
        (void)options;

        throw std::runtime_error("Unimplemented");

        if (str.empty() || !is_numeric(str))
        {
            throw std::invalid_argument("Invalid input string. Needs to be a valid Exact Real Radical");
        }
    }

    // TODO: handle sign bit?
    std::string STH_ExactRadical::makeStringFromData(const data_t& data, const options_t& options)
    {
        (void)options;

        if (data.size() != dataByteCount)
        {
            throw std::invalid_argument("Invalid data array. Needs to contain " + std::to_string(dataByteCount) + " bytes");
        }

        std::string dataStr;
        for (uint i = 0; i < dataByteCount; i++)
        {
            dataStr += dechex(data[i]);
        }

        const auto type = data[0] & ~0x80; // sign bit discarded
        if (type != types["ExactRealRadical"].getId() && type != types["ExactComplexRadical"].getId())
        {
            throw std::invalid_argument("Invalid data bytes - invalid vartype: " + std::to_string(type));
        }

        const auto variant = hexdec(dataStr.substr(2, 1));
        if (variant > 3)
        {
            throw std::invalid_argument("Invalid data bytes - unknown type variant: " + std::to_string(variant));
        }

        const std::vector<std::string> parts = {
            (variant == 1 || variant == 3 ? "-" : "") + trimZeros(dataStr.substr(9, 3)),
            trimZeros(dataStr.substr(15, 3)),
            (variant == 2 || variant == 3 ? "-" : "+") + trimZeros(dataStr.substr(6, 3)),
            trimZeros(dataStr.substr(12, 3)),
            trimZeros(dataStr.substr(3, 3))
        };

        std::string str = "(" + parts[0] + "*√(" + parts[1] + ")" + parts[2] + "*√(" + parts[3]  + "))/" + parts[4];

        // Improve final display
        str = std::regex_replace(str, std::regex("\\+1\\*"), "+");  str = std::regex_replace(str, std::regex("\\(1\\*"),  "(");
        str = std::regex_replace(str, std::regex("-1\\*"),   "-");  str = std::regex_replace(str, std::regex("\\(-1\\*"), "(-");
        str = std::regex_replace(str, std::regex("\\+-"),    "-");

        return str;
    }

}