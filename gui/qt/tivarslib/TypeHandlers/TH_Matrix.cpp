/*
 * Part of tivars_lib_cpp
 * (C) 2015-2019 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "TypeHandlers.h"
#include "../tivarslib_utils.h"

namespace tivars
{

    data_t TH_Matrix::makeDataFromString(const std::string& str, const options_t& options)
    {
        (void)options;

        data_t data(2); // reserve 2 bytes for size fields

        if (str.length() < 5 || str.substr(0, 2) != "[[" || str.substr(str.length()-2, 2) != "]]")
        {
            throw std::invalid_argument("Invalid input string. Needs to be a valid matrix");
        }

        size_t rowCount, colCount;
        std::vector<std::vector<std::string>> matrix;
        std::vector<std::string> rows;

        rows = explode(str.substr(2, str.length()-4), "][");
        rowCount = rows.size();
        matrix.resize(rowCount);

        colCount = (size_t) (count(rows[0].begin(), rows[0].end(), ',') + 1);

        if (colCount > 0xFF || rowCount > 0xFF)
        {
            throw std::invalid_argument("Invalid input string. Needs to be a valid matrix (max col/row = 255)");
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
                    throw std::invalid_argument("Invalid input string. Needs to be a valid matrix (real numbers inside)");
                }
            }
            if (tmp.size() != colCount)
            {
                throw std::invalid_argument("Invalid input string. Needs to be a valid matrix (consistent column count)");
            }
            matrix[counter++] = tmp;
        }

        data[0] = (uchar)(colCount & 0xFF);
        data[1] = (uchar)(rowCount & 0xFF);

        for (const std::vector<std::string>& row : matrix)
        {
            for (const auto& numStr : row)
            {
                const auto& tmp = TH_GenericReal::makeDataFromString(numStr, { {"_type", 0x00} });
                data.insert(data.end(), tmp.begin(), tmp.end());
            }
        }

        return data;
    }

    std::string TH_Matrix::makeStringFromData(const data_t& data, const options_t& options)
    {
        (void)options; // TODO: prettified option

        const size_t byteCount = data.size();
        if (byteCount < 2)
        {
            throw std::invalid_argument("Invalid data array. Needs to contain at least 2 bytes");
        }

        const size_t colCount = data[0];
        const size_t rowCount = data[1];

        if (data.size() < 2+TH_GenericReal::dataByteCount || colCount < 1 || rowCount < 1
            || ((byteCount - 2) % TH_GenericReal::dataByteCount != 0) || (colCount*rowCount != (byteCount - 2) / TH_GenericReal::dataByteCount))
        {
            throw std::invalid_argument("Invalid data array. Needs to contain 1+1+" + std::to_string(TH_GenericReal::dataByteCount) + "*n bytes");
        }

        std::string str = "[";

        for (uint i = 2, num = 0; i < byteCount; i += TH_GenericReal::dataByteCount, num++)
        {
            if (num % colCount == 0) // first column
            {
                str += "[";
            }
            str += TH_GenericReal::makeStringFromData(data_t(data.begin()+i, data.begin()+i+TH_GenericReal::dataByteCount));
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
