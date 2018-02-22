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

    data_t TH_0x20::makeDataFromString(const string& str, const options_t& options)
    {
        (void)options;

        throw runtime_error("Unimplemented");

        if (str == "" || !is_numeric(str))
        {
            throw invalid_argument("Invalid input string. Needs to be a valid Exact Real Pi");
        }
    }

    string TH_0x20::makeStringFromData(const data_t& data, const options_t& options)
    {
        (void)options;

        if (data.size() != dataByteCount)
        {
            throw invalid_argument("Empty data array. Needs to contain " + to_string(dataByteCount) + " bytes");
        }

        string coeff = TH_0x00::makeStringFromData(data);

        string str = ((coeff != "0") ? (coeff + "*π") : "");

        // Improve final display
        str = regex_replace(str, regex("\\+1\\*"), "+");  str = regex_replace(str, regex("\\(1\\*"),  "(");
        str = regex_replace(str, regex("-1\\*"),   "-");  str = regex_replace(str, regex("\\(-1\\*"), "(-");
        str = regex_replace(str, regex("\\+-"),    "-");

        // Shouldn't happen - I don't believe the calc generate such files.
        if (str == "")
        {
            str = "0";
        }

        return str;
    }

}