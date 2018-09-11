/*
 * Part of tivars_lib_cpp
 * (C) 2015-2018 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "TIVarType.h"
#include "TIVarTypes.h"

namespace tivars
{
    /*** "Constructors" ***/

    /**
     * @param   int     id     The type ID
     * @return  TIVarType
     * @throws  \Exception
     */
    TIVarType TIVarType::createFromID(uint id)
    {
        if (TIVarTypes::isValidID(id))
        {
            return types.at(std::to_string(id));
        } else {
            throw std::invalid_argument("Invalid type ID");
        }
    }

    /**
     * @param   string  name   The type name
     * @return  TIVarType
     * @throws  \Exception
     */
    TIVarType TIVarType::createFromName(std::string name)
    {
        if (TIVarTypes::isValidName(name))
        {
            return types.at(name);
        } else {
            throw std::invalid_argument("Invalid type name");
        }
    }
}

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
    using namespace emscripten;
    EMSCRIPTEN_BINDINGS(_tivartype) {
            class_<tivars::TIVarType>("TIVarType")
                    .constructor<>()
                    .constructor<int, const std::string&, const std::vector<std::string>&, const tivars::handler_pair_t&>()

                    .function("getId"      , &tivars::TIVarType::getId)
                    .function("getName"    , &tivars::TIVarType::getName)
                    .function("getExts"    , &tivars::TIVarType::getExts)
                    .function("getHandlers", &tivars::TIVarType::getHandlers)

                    .class_function("createFromID",   &tivars::TIVarType::createFromID)
                    .class_function("createFromName", &tivars::TIVarType::createFromName)
            ;
    }
#endif
