/*
 * Part of tivars_lib_cpp
 * (C) 2015-2019 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "TypeHandlers.h"
#include "../tivarslib_utils.h"
#include "../TIVarTypes.h"

namespace tivars
{
    // TODO: also make it detect the type correctly...
    data_t TH_GenericList::makeDataFromString(const std::string& str, const options_t& options)
    {
        const auto& typeIter = options.find("_type");
        if (typeIter == options.end())
        {
            throw std::runtime_error("Needs _type in options for TH_GenericReal::makeDataFromString");
        }

        const auto type = typeIter->second;
        if (type != types["Real"].getId() && type != types["Complex"].getId())
        {
            throw std::invalid_argument("Invalid type for given string");
        }

        auto arr = explode(trim(str, "{}"), ',');
        const size_t numCount = arr.size();

        for (auto& numStr : arr)
        {
            numStr = trim(numStr);
        }
        if (str.empty() || arr.empty() || numCount > 999)
        {
            throw std::invalid_argument("Invalid input string. Needs to be a valid real or complex list");
        }

        const auto handler = (type == 0x00) ? &TH_GenericReal::makeDataFromString : &TH_GenericComplex::makeDataFromString;

        data_t data(2); // reserve 2 bytes for size fields

        data[0] = (uchar) (numCount & 0xFF);
        data[1] = (uchar) ((numCount >> 8) & 0xFF);

        for (const auto& numStr : arr)
        {
            const auto& tmp = handler(numStr, options);
            data.insert(data.end(), tmp.begin(), tmp.end());
        }

        return data;
    }

    std::string TH_GenericList::makeStringFromData(const data_t& data, const options_t& options)
    {
        const size_t byteCount = data.size();
        if (byteCount < 2)
        {
            throw std::invalid_argument("Invalid data array. Needs to contain at least 2 bytes");
        }

        const size_t numCount = (size_t) ((data[0] & 0xFF) + ((data[1] & 0xFF) << 8));

        const bool isRealList    = (numCount == (size_t)((byteCount - 2) / TH_GenericReal::dataByteCount));
        const bool isComplexList = (numCount == (size_t)((byteCount - 2) / TH_GenericComplex::dataByteCount));

        if (!(isRealList ^ isComplexList))
        {
            throw std::invalid_argument("Invalid data array. Needs to contain 2+9*n or 2+18*n bytes");
        }

        const size_t typeByteCount = isRealList ? TH_GenericReal::dataByteCount : TH_GenericComplex::dataByteCount;

        if (byteCount < 2+typeByteCount || ((byteCount - 2) % typeByteCount != 0) || numCount > 999)
        {
            throw std::invalid_argument("Invalid data array. Needs to contain 2+" + std::to_string(typeByteCount) + "*n bytes");
        }

        const auto handler = isRealList ? &TH_GenericReal::makeStringFromData : &TH_GenericComplex::makeStringFromData;

        std::string str = "{";
        for (size_t i = 2, num = 0; i < byteCount; i += typeByteCount, num++)
        {
            str += handler(data_t(data.begin()+i, data.begin()+i+typeByteCount), options);
            if (num < numCount - 1) // not last num
            {
                str += ',';
            }
        }
        str += "}";

        return str;
    }
}
