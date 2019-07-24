/*
 * Part of tivars_lib_cpp
 * (C) 2015-2019 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "BinaryFile.h"
#include "tivarslib_utils.h"

namespace tivars
{

    /**
     * @param null filePath
     * @throws \Exception
     */
    BinaryFile::BinaryFile(const std::string& filePath)
    {
        if (!filePath.empty())
        {
            if (file_exists(filePath))
            {
                this->file = fopen(filePath.c_str(), "rb+");
                if (!this->file)
                {
                    throw std::runtime_error("Can't open the input file");
                }
                this->filePath = filePath;
                fseek(this->file, 0L, SEEK_END);
                this->fileSize = (size_t) ftell(this->file);
                fseek(this->file, 0L, SEEK_SET);
            } else {
                throw std::runtime_error("No such file");
            }
        } else {
            throw std::invalid_argument("Empty file path given");
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
            const size_t n = fread(v.data(), sizeof(uchar), bytes, file);
            if (n != bytes || ferror(file))
            {
                throw std::runtime_error("Error in get_raw_bytes");
            }
            return v;
        } else {
            throw std::runtime_error("No file loaded");
        }
    }

    /**
     * Returns a string of bytes bytes read from the file (doesn't stop at NUL)
     *
     * @param   uint bytes The number of bytes to read
     * @return  string
     * @throws  runtime_error
     */
    std::string BinaryFile::get_string_bytes(uint bytes)
    {
        if (file)
        {
            std::string buf(bytes, '\0');
            const size_t n = fread(&buf[0], sizeof(char), bytes, file);
            if (n != bytes || ferror(file))
            {
                throw std::runtime_error("Error in get_string_bytes");
            }
            return buf;
        } else {
            throw std::runtime_error("No file loaded");
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