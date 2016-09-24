/*
 * Part of tivars_lib_cpp
 * (C) 2015-2016 Adrien 'Adriweb' Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "../autoloader.h"

#include <QtCore/QFile>

#include <fstream>
#include <sstream>
#include <regex>

using namespace std;

namespace tivars
{
    data_t TH_0x0C::makeDataFromString(const string& str, const options_t options)
    {
        (void)options;
        data_t data;

        string newStr = str;
        newStr = regex_replace(newStr, regex(" "), "");
        newStr = regex_replace(newStr, regex("\\+i"), "+1i");
        newStr = regex_replace(newStr, regex("-i"), "-1i");

        smatch matches;

        bool isValid = checkValidStringAndGetMatches(str, matches);

        if (!isValid || matches.size() != 3)
        {
            std::cerr << "Invalid input string. Needs to be a valid complex number (a+bi)" << endl;
            return data_t();
        }

        for (int i=0; i<2; i++)
        {
            string coeff = matches[i+1];
            if (coeff.empty())
            {
                coeff = "0";
            }

            const auto& tmp = TH_0x00::makeDataFromString(coeff);
            data.insert(data.end(), tmp.begin(), tmp.end());

            uchar flags = 0;
            flags |= (atof(coeff.c_str()) < 0) ? (1 << 7) : 0;
            flags |= (1 << 2); // Because it's a complex number
            flags |= (1 << 3); // Because it's a complex number

            data[i * TH_0x00::dataByteCount] = flags;
        }

        return data;
    }

    string TH_0x0C::makeStringFromData(const data_t& data, const options_t options)
    {
        (void)options;

        if (data.size() != 2 * TH_0x00::dataByteCount)
        {
            std::cerr << "Empty data array. Needs to contain " + to_string(2 * TH_0x00::dataByteCount) + " bytes" << endl;
            return "";
        }

        string coeffR = TH_0x00::makeStringFromData(data_t(data.begin(), data.begin() + TH_0x00::dataByteCount));
        string coeffI = TH_0x00::makeStringFromData(data_t(data.begin() + TH_0x00::dataByteCount, data.begin() + 2 * TH_0x00::dataByteCount));

        string str = coeffR + "+" + coeffI + "i";
        str = regex_replace(str, regex("\\+\\-"), "-");

        return str;
    }

    bool TH_0x0C::checkValidString(const string& str)
    {
        smatch matches;
        return checkValidStringAndGetMatches(str, matches);
    }

    bool TH_0x0C::checkValidStringAndGetMatches(const string& str, smatch& matches)
    {
        if (str.empty())
        {
            return false;
        }

        // Handle real only, real+imag, image only.
        bool isValid = regex_match(str, matches, regex("^"   + TH_0x00::validPattern + "()$"))
                    || regex_match(str, matches, regex("^"   + TH_0x00::validPattern + TH_0x00::validPattern + "i$"))
                    || regex_match(str, matches, regex("^()" + TH_0x00::validPattern + "i$"));

        return isValid;
    }
}
