/*
 * Part of tivars_lib_cpp
 * (C) 2015 Adrien 'Adriweb' Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "../autoloader.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>

// TODO : check if the models have different exponent offsets

using namespace std;

namespace tivars
{
    
    data_t TH_0x00::makeDataFromString(const string& str, const options_t options)
    {
        data_t data(9);

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
        for (uint i = 2; i < 9; i++)
        {
            data[i] = (uchar)(hexdec(newStr.substr(2*(i-2), 2)) & 0xFF);
        }

        return data;
    }

    string TH_0x00::makeStringFromData(const data_t& data, const options_t options)
    {
        (void)options;

        if (data.size() != 9)
        {
            std::cerr << "Invalid data array. Needs to contain 9 bytes" << endl;
            return "";
        }
        uint flags      = data[0];
        bool isNegative = (flags >> 7 == 1);
//      bool isSeqInit  = (flags  & 1 == 1); // if true, "used for initial sequence values"
        uint exponent   = (uint)(data[1] - 0x80);
        string number   = "";
        for (uint i = 2; i < 9; i++)
        {
            number += dechex(data[i]);
        }
        number = number.substr(0, 1) + "." + number.substr(1);

        string str = to_string(pow(10, exponent) * std::stod(number));

        // Cleanup
        if (str.length() > 12)
        {
            str.erase(str.begin() + 12, str.end());
        }
        if (str.find('.') != std::string::npos)
        {
            while (str.back() == '0') str.pop_back();
        }

        str = (isNegative ? "-" : "") + str;

        return str;
    }
}
