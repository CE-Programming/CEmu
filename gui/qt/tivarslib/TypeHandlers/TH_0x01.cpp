/*
 * Part of tivars_lib_cpp
 * (C) 2016 Adrien 'Adriweb' Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "../autoloader.h"

using namespace std;

namespace tivars
{
    
    data_t TH_0x01::makeDataFromString(const string& str, const options_t options)
    {
        (void)options;

        data_t data(2); // reserve 2 bytes for size fields

        auto arr = explode(trim(str, "{}"), ',');
        size_t numCount = arr.size();

        bool formatOk = true;
        for (auto& numStr : arr)
        {
            numStr = trim(numStr);
            if (!is_numeric(numStr))
            {
                formatOk = false;
                break;
            }
        }
        if (str.empty() || arr.empty() || !formatOk || numCount > 999)
        {
            std::cerr << "Invalid input string. Needs to be a valid real list" << endl;
            return data;
        }

        data[0] = (uchar) (numCount & 0xFF);
        data[1] = (uchar) ((numCount >> 8) & 0xFF);

        for (auto& numStr : arr)
        {
            const auto& tmp = TH_0x00::makeDataFromString(numStr);
            data.insert(data.end(), tmp.begin(), tmp.end());
        }

        return data;
    }

    string TH_0x01::makeStringFromData(const data_t& data, const options_t options)
    {
        (void)options;

        string str;

        size_t byteCount = data.size();
        size_t numCount = (size_t) ((data[0] & 0xFF) + ((data[1] << 8) & 0xFF00));
        if (byteCount < 2+TH_0x00::dataByteCount || ((byteCount - 2) % TH_0x00::dataByteCount != 0)
            || (numCount != (size_t)((byteCount - 2) / TH_0x00::dataByteCount)) || numCount > 999)
        {
            std::cerr << "Invalid data array. Needs to contain 2+" + to_string(TH_0x00::dataByteCount) + "*n bytes" << endl;
            return "";
        }

        str = "{";

        for (size_t i = 2, num = 0; i < byteCount; i += TH_0x00::dataByteCount, num++)
        {
            str += TH_0x00::makeStringFromData(data_t(data.begin()+i, data.begin()+i+TH_0x00::dataByteCount));
            if (num < numCount - 1) // not last num
            {
                str += ',';
            }
        }

        str += "}";

        return str;
    }
}
