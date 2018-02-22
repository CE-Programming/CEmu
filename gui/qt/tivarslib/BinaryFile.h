/*
 * Part of tivars_lib_cpp
 * (C) 2015-2018 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#ifndef TIVARS_LIB_CPP_BINARYFILE_H
#define TIVARS_LIB_CPP_BINARYFILE_H

#include "CommonTypes.h"

namespace tivars
{
    class BinaryFile
    {
    public:
        BinaryFile() = default;

        explicit BinaryFile(const std::string& filePath);

        BinaryFile(const BinaryFile&) = delete;
        BinaryFile& operator=(const BinaryFile) = delete;

        BinaryFile(BinaryFile&& o) noexcept : file(o.file), fileSize(o.fileSize)
        { o.file = nullptr; }

        ~BinaryFile()
        {
            close();
        }

        data_t get_raw_bytes(uint bytes);
        std::string get_string_bytes(uint bytes);
        size_t size() const;
        void close();

    protected:
        FILE* file = nullptr;
        std::string filePath = "";
        size_t fileSize = 0;

    };
}

#endif //TIVARS_LIB_CPP_BINARYFILE_H
