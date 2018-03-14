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
        if (!filePath.empty())
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
            throw invalid_argument("Empty file path given");
        }
    }

    /**
     * Returns an array of bytes bytes read from the file
     *
     * @param   uint bytes
     * @return  data_t
     * @throws  runtime_error
     */
    data_t BinaryFile::get_raw_bytes(uint bytes)
    {
        if (file)
        {
            data_t v(bytes);
            size_t n = fread(v.data(), sizeof(uchar), bytes, file);
            if (n != bytes || ferror(file))
            {
                throw runtime_error("Error in get_raw_bytes");
            }
            return v;
        } else {
            throw runtime_error("No file loaded");
        }
    }

    /**
     * Returns a string of bytes bytes read from the file (doesn't stop at NUL)
     *
     * @param   uint bytes The number of bytes to read
     * @return  string
     * @throws  runtime_error
     */
    string BinaryFile::get_string_bytes(uint bytes)
    {
        if (file)
        {
            string buf(bytes, '\0');
            size_t n = fread(&buf[0], sizeof(char), bytes, file);
            if (n != bytes || ferror(file))
            {
                throw runtime_error("Error in get_string_bytes");
            }
            return buf;
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
