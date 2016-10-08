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

    data_t TH_0x1D::makeDataFromString(const string& str, const options_t& options)
    {
        (void)options;

        std::cerr << "Unimplemented" << std::endl;
        return data_t();

        if (str == "" || !is_numeric(str))
        {
            std::cerr << "Invalid input string. Needs to be a valid Exact Complex Radical" << std::endl;
        }
    }

    string TH_0x1D::makeStringFromData(const data_t& data, const options_t& options)
    {
        (void)options;

        if (data.size() != dataByteCount)
        {
            std::cerr << ("Empty data array. Needs to contain " + to_string(dataByteCount) + " bytes") << std::endl;
            return "";
        }

        string coeffR = TH_0x1C::makeStringFromData(data_t(data.begin(), data.begin() + TH_0x1C::dataByteCount));
        string coeffI = TH_0x1C::makeStringFromData(data_t(data.begin() + TH_0x1C::dataByteCount, data.begin() + 2 * TH_0x1C::dataByteCount));

        string str = "(" + coeffR + ")+(" + coeffI + ")*i";
        
        str = regex_replace(str, regex("\\+-"), "-");

        return str;
    }

}
