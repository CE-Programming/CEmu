/*
 * Part of tivars_lib_cpp
 * (C) 2015-2018 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "TypeHandlers.h"
#include "../tivarslib_utils.h"

using namespace std;

namespace tivars
{
    
    data_t TH_0x02::makeDataFromString(const string& str, const options_t& options)
    {
        (void)options;

        data_t data(2); // reserve 2 bytes for size fields

        if (str.length() < 5 || str.substr(0, 2) != "[[" || str.substr(str.length()-2, 2) != "]]")
        {
            throw invalid_argument("Invalid input string. Needs to be a valid matrix");
        }

        size_t rowCount, colCount;
        vector<vector<string>> matrix;
        vector<string> rows;

        rows = explode(str.substr(2, str.length()-4), "][");
        rowCount = rows.size();
        matrix.resize(rowCount);

        colCount = (size_t) (count(rows[0].begin(), rows[0].end(), ',') + 1);

        if (colCount > 0xFF || rowCount > 0xFF)
        {
            throw invalid_argument("Invalid input string. Needs to be a valid matrix (max col/row = 255)");
        }

        uint counter = 0;
        for (const auto& row : rows)
        {
            auto tmp = explode(row, ",");
            for (auto& numStr : tmp)
            {
                numStr = trim(numStr);
                if (!is_numeric(numStr))
                {
                    throw invalid_argument("Invalid input string. Needs to be a valid matrix (real numbers inside)");
                }
            }
            if (tmp.size() != colCount)
            {
                throw invalid_argument("Invalid input string. Needs to be a valid matrix (consistent column count)");
            }
            matrix[counter++] = tmp;
        }

        data[0] = (uchar)(colCount & 0xFF);
        data[1] = (uchar)(rowCount & 0xFF);

        for (const vector<string>& row : matrix)
        {
            for (const auto& numStr : row)
            {
                const auto& tmp = TH_0x00::makeDataFromString(numStr);
                data.insert(data.end(), tmp.begin(), tmp.end());
            }
        }

        return data;
    }

    string TH_0x02::makeStringFromData(const data_t& data, const options_t& options)
    {
        (void)options; // TODO: prettified option

        size_t byteCount = data.size();
        size_t colCount = data[0];
        size_t rowCount = data[1];

        if (data.size() < 2+TH_0x00::dataByteCount || colCount < 1 || rowCount < 1 || colCount > 255 || rowCount > 255
            || ((byteCount - 2) % TH_0x00::dataByteCount != 0) || (colCount*rowCount != (byteCount - 2) / TH_0x00::dataByteCount))
        {
            throw invalid_argument("Invalid data array. Needs to contain 1+1+" + to_string(TH_0x00::dataByteCount) + "*n bytes");
        }

        string str = "[";

        for (uint i = 2, num = 0; i < byteCount; i += TH_0x00::dataByteCount, num++)
        {
            if (num % colCount == 0) // first column
            {
                str += "[";
            }
            str += TH_0x00::makeStringFromData(data_t(data.begin()+i, data.begin()+i+TH_0x00::dataByteCount));
            if (num % colCount < colCount - 1) // not last column
            {
                str += ",";
            } else {
                str += "]";
            }
        }

        str += "]";

        return str;
    }
}
