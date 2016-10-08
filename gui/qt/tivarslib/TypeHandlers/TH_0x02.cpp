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
    
    data_t TH_0x02::makeDataFromString(const string& str, const options_t& options)
    {
        (void)options;

        data_t data(2); // reserve 2 bytes for size fields

        if (str.length() < 5 || str.substr(0, 2) != "[[" || str.substr(str.length()-2, 2) != "]]")
        {
            std::cerr << "Invalid input string. Needs to be a valid matrix" << std::endl;
            return data_t();
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
            std::cerr << "Invalid input string. Needs to be a valid matrix (max col/row = 255)" << std::endl;
            return data_t();
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
                    std::cerr << "Invalid input string. Needs to be a valid matrix (real numbers inside)" << std::endl;
                    return data_t();
                }
            }
            if (tmp.size() != colCount)
            {
                std::cerr << "Invalid input string. Needs to be a valid matrix (consistent column count)" << std::endl;
                return data_t();
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
        (void)options;

        size_t byteCount = data.size();
        size_t colCount = data[0];
        size_t rowCount = data[1];

        if (data.size() < 2+TH_0x00::dataByteCount || colCount < 1 || rowCount < 1 || colCount > 255 || rowCount > 255
            || ((byteCount - 2) % TH_0x00::dataByteCount != 0) || (colCount*rowCount != (byteCount - 2) / TH_0x00::dataByteCount))
        {
            std::cerr << "Invalid data array. Needs to contain 1+1+" << TH_0x00::dataByteCount << "*n bytes" << std::endl;
            return "";
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

        // TODO: prettified option
        
        return str;
    }
}
