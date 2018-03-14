/*
 * Part of tivars_lib_cpp
 * (C) 2015-2018 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "TypeHandlers.h"
#include "../tivarslib_utils.h"
#include <regex>

using namespace std;

namespace tivars
{

    data_t TH_0x1B::makeDataFromString(const string& str, const options_t& options)
    {
        (void)options;

        throw runtime_error("Unimplemented");

        if (str.empty() || !is_numeric(str))
        {
            throw invalid_argument("Invalid input string. Needs to be a valid Exact Complex Fraction");
        }
    }

    string TH_0x1B::makeStringFromData(const data_t& data, const options_t& options)
    {
        (void)options;

        if (data.size() != dataByteCount)
        {
            throw invalid_argument("Empty data array. Needs to contain " + to_string(dataByteCount) + " bytes");
        }

        string coeffR = TH_0x00::makeStringFromData(data_t(data.begin(), data.begin() + TH_0x00::dataByteCount));
        string coeffI = TH_0x00::makeStringFromData(data_t(data.begin() + TH_0x00::dataByteCount, data.begin() + 2 * TH_0x00::dataByteCount));

        string str = dec2frac(atof(coeffR.c_str())) + "+" + dec2frac(atof(coeffI.c_str())) + "i";
        str = regex_replace(str, regex("\\+-"), "-");

        return str;
    }

}