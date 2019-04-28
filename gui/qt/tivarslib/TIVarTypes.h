/*
 * Part of tivars_lib_cpp
 * (C) 2015-2019 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#ifndef TIVARTYPES_H
#define TIVARTYPES_H

#include "CommonTypes.h"
#include "TIVarType.h"
#include <unordered_map>

namespace tivars
{
    extern std::unordered_map<std::string, TIVarType> types;

    class TIVarTypes
    {

    public:
        static void initTIVarTypesArray();

        static bool isValidName(const std::string& name);

        static bool isValidID(int id);

        static std::vector<std::string> getExtensionsFromName(const std::string& name);
        static std::vector<std::string> getExtensionsFromTypeID(int id);
        static int getIDFromName(const std::string& name);
        static std::string getNameFromID(int id);

    private:
        static void insertType(const std::string& name, int id, const std::vector<std::string>& exts, const handler_pair_t& handlers = make_handler_pair(DummyHandler));

    };
}

#endif //TIVARTYPES_H
