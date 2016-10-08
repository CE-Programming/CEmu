/*
 * Part of tivars_lib_cpp
 * (C) 2015-2016 Adrien 'Adriweb' Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "../autoloader.h"

// TODO : check if the models have different exponent offsets

using namespace std;

namespace tivars
{
    
    data_t TH_0x00::makeDataFromString(const string& str, const options_t& options)
    {
        data_t data(TH_0x00::dataByteCount);

        if (str == "" || !is_numeric(str))
        {
            std::cerr << "Invalid input string. Needs to be a valid real number" << endl;
            return data;
        }

        double number = atof(str.c_str());

        int exponent = (int)floor(log10(abs(number)));
        number *= pow(10.0, -exponent);
        char tmp[20];
        sprintf(tmp, "%0.14f", number);
        string newStr = stripchars(tmp, "-.");

        uchar flags = 0;
        flags |= (number < 0) ? (1 << 7) : 0;
        flags |= (has_option(options, "seqInit") && options.at("seqInit") == 1) ? 1 : 0;

        data[0] = flags;
        data[1] = (uchar)(exponent + 0x80);
        for (uint i = 2; i < TH_0x00::dataByteCount; i++)
        {
            data[i] = (uchar)(hexdec(newStr.substr(2*(i-2), 2)) & 0xFF);
        }

        return data;
    }

    string TH_0x00::makeStringFromData(const data_t& data, const options_t& options)
    {
        (void)options;

        if (data.size() != TH_0x00::dataByteCount)
        {
            std::cerr << "Invalid data array. Needs to contain " + to_string(TH_0x00::dataByteCount) + " bytes" << endl;
            return "";
        }
        uint flags      = data[0];
        bool isNegative = (flags >> 7 == 1);
//      bool isSeqInit  = (flags  & 1 == 1); // if true, "used for initial sequence values"
        int exponent    = data[1] - 0x80;
        string number   = "";
        for (uint i = 2; i < TH_0x00::dataByteCount; i++)
        {
            number += (data[i] < 0x10 ? "0" : "") + dechex(data[i]); // zero left pad
        }
        number = number.substr(0, 1) + "." + number.substr(1);

        char buf[35] = {0};
        sprintf(buf, "%.*f", DECIMAL_DIG, pow(10, exponent) * atof(number.c_str()));
        string str(buf);

        // Cleanup
        if (str.length() > 12)
        {
            str.erase(str.begin() + 12, str.end());
        }
        if (str.find('.') != std::string::npos)
        {
            while (str.back() == '0') str.pop_back();
        }
        if (str.back() == '.')
        {
            str.pop_back();
        }

        str = (isNegative ? "-" : "") + str;

        return str;
    }
}
