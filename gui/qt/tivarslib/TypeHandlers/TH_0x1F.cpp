/*
 * Part of tivars_lib_cpp
 * (C) 2015-2016 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "../autoloader.h"

using namespace std;

namespace tivars
{

    data_t TH_0x1F::makeDataFromString(const string& str, const options_t& options)
    {
        (void)options;

        std::cerr << "Unimplemented" << std::endl;
        return data_t();

        if (str == "" || !is_numeric(str))
        {
            std::cerr << "Invalid input string. Needs to be a valid Exact Complex Pi Fraction" << std::endl;
        }
    }

    string TH_0x1F::makeStringFromData(const data_t& data, const options_t& options)
    {
        (void)options;

        if (data.size() != dataByteCount)
        {
            std::cerr << ("Empty data array. Needs to contain " + to_string(dataByteCount) + " bytes") << std::endl;
            return "";
        }

        string coeffR = TH_0x00::makeStringFromData(data_t(data.begin(), data.begin() + TH_0x00::dataByteCount));
        string coeffI = TH_0x00::makeStringFromData(data_t(data.begin() + TH_0x00::dataByteCount, data.begin() + 2 * TH_0x00::dataByteCount));

        string str = ((coeffR != "0") ? (dec2frac(atof(coeffR.c_str())) + "*π+")  : "")
                   + ((coeffI != "0") ? (dec2frac(atof(coeffI.c_str())) + "*π*i") : "");

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
