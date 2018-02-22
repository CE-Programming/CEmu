/*
 * Part of tivars_lib_cpp
 * (C) 2015-2018 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "BinaryFile.h"
#include "tivarslib_utils.h"

using namespace std;

namespace tivars
{

    /**
     * @param null filePath
     * @throws \Exception
     */
    BinaryFile::BinaryFile(const string& filePath)
    {
        if (filePath != "")
        {
            if (file_exists(filePath))
            {
                this->file = fopen(filePath.c_str(), "rb+");
                if (!this->file)
                {
                    throw runtime_error("Can't open the input file");
                }
                this->filePath = filePath;
                fseek(this->file, 0L, SEEK_END);
                this->fileSize = (size_t) ftell(this->file);
                fseek(this->file, 0L, SEEK_SET);
            } else {
                throw runtime_error("No such file");
            }
        } else {
            throw invalid_argument("No file path given");
        }
    }

    /**
     * Returns an array of bytes bytes read from the file
     *
     * @param   int bytes
     * @return  array
     * @throws  \Exception
     */
    data_t BinaryFile::get_raw_bytes(uint bytes)
    {
        if (file)
        {
            if (bytes > 0)
            {
                uchar buf[bytes];
                size_t n = fread(buf, sizeof(uchar), bytes, file);
                if (n < 1) {
                    return data_t{};
                }
                data_t v(buf, buf + bytes);
                return v;
            } else {
                throw invalid_argument("Invalid number of bytes to read");
            }
        } else {
            throw runtime_error("No file loaded");
        }
    }

    /**
     * Returns a string of bytes bytes read from the file (doesn't stop at NUL)
     *
     * @param   int bytes The number of bytes to read
     * @return  string
     * @throws  \Exception
     */
    string BinaryFile::get_string_bytes(uint bytes)
    {
        if (file)
        {
            if (bytes > 0)
            {
                char buf[bytes+1];
                size_t n = fread(buf, sizeof(char), bytes, file);
                if (n < 1) {
                    return string{};
                }
                buf[bytes] = '\0';
                return string(buf);
            } else {
                throw invalid_argument("Invalid number of bytes to read");
            }
        } else {
            throw runtime_error("No file loaded");
        }
    }

    void BinaryFile::close()
    {
        if (file)
        {
            fclose(file);
            file = nullptr;
        }
    }

    size_t BinaryFile::size() const
    {
        return fileSize;
    }
}
