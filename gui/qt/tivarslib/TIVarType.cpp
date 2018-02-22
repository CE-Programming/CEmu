/*
 * Part of tivars_lib_cpp
 * (C) 2015-2018 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "TIVarType.h"
#include "TIVarTypes.h"

using namespace std;

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
            return types.at(to_string(id));
        } else {
            throw invalid_argument("Invalid type ID");
        }
    }

    /**
     * @param   string  name   The type name
     * @return  TIVarType
     * @throws  \Exception
     */
    TIVarType TIVarType::createFromName(string name)
    {
        if (TIVarTypes::isValidName(name))
        {
            return types.at(name);
        } else {
            throw invalid_argument("Invalid type name");
        }
    }


#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
    using namespace emscripten;
    EMSCRIPTEN_BINDINGS(_tivartype) {
            class_<TIVarType>("TIVarType")
                    .constructor<>()
                    .constructor<int, const std::string&, const std::vector<std::string>&, const handler_pair_t&>()

                    .function("getId"      , &TIVarType::getId)
                    .function("getName"    , &TIVarType::getName)
                    .function("getExts"    , &TIVarType::getExts)
                    .function("getHandlers", &TIVarType::getHandlers)

                    .class_function("createFromID",   &TIVarType::createFromID)
                    .class_function("createFromName", &TIVarType::createFromName)
            ;
    }
#endif


}
