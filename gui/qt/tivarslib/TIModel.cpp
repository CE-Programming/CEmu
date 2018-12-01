/*
 * Part of tivars_lib_cpp
 * (C) 2015-2018 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/TIs_lib
 * License: MIT
 */

#include "TIModel.h"
#include "TIModels.h"

namespace tivars
{
    
    bool TIModel::supportsType(const TIVarType& type)
    {
        const std::vector<std::string>& exts = type.getExts();
        return this->orderID >= 0 && this->orderID < (int)exts.size() && !exts[this->orderID].empty();
    }

    /*** "Constructors" ***/

    /**
     * @param   int     flags  The version compatibliity flags
     * @return  TIModel
     * @throws  \Exception
     */
    TIModel TIModel::createFromFlags(uint flags)
    {
        if (TIModels::isValidFlags(flags))
        {
            TIModel model;
            model.flags = flags;
            model.orderID = TIModels::getDefaultOrderIDFromFlags(flags);
            model.sig = TIModels::getSignatureFromFlags(flags);
            model.name = TIModels::getDefaultNameFromFlags(flags);
            return model;
        } else
        {
            throw std::invalid_argument("Invalid version ID");
        }
    }

    /**
     * @param   string  name   The version name
     * @return  TIModel
     * @throws  \Exception
     */
    TIModel TIModel::createFromName(const std::string& name)
    {
        if (TIModels::isValidName(name))
        {
            TIModel model;
            model.name = name;
            model.orderID = TIModels::getOrderIDFromName(name);
            model.flags = TIModels::getFlagsFromName(name);
            model.sig = TIModels::getSignatureFromName(name);
            return model;
        } else
        {
            throw std::invalid_argument("Invalid version name");
        }
    }

    /**
     * @param   string  sig    The signature (magic bytes)
     * @return  TIModel
     * @throws  \Exception
     */
    TIModel TIModel::createFromSignature(const std::string& sig)
    {
        if (TIModels::isValidSignature(sig))
        {
            TIModel model;
            model.sig = sig;
            model.orderID = TIModels::getDefaultOrderIDFromSignature(sig);
            model.flags = TIModels::getMinFlagsFromSignature(sig);
            model.name = TIModels::getDefaultNameFromSignature(sig);
            return model;
        } else
        {
            throw std::invalid_argument("Invalid version signature");
        }
    }
}

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
    using namespace emscripten;
    EMSCRIPTEN_BINDINGS(_timodel) {
            class_<tivars::TIModel>("TIModel")
                    .constructor<>()
                    .constructor<int, const std::string&, uint, const std::string&>()

                    .function("getOrderId"  , &tivars::TIModel::getOrderId)
                    .function("getName"     , &tivars::TIModel::getName)
                    .function("getFlags"    , &tivars::TIModel::getFlags)
                    .function("getSig"      , &tivars::TIModel::getSig)
                    .function("supportsType", &tivars::TIModel::supportsType)

                    .class_function("createFromFlags",      &tivars::TIModel::createFromFlags)
                    .class_function("createFromName",       &tivars::TIModel::createFromName)
                    .class_function("createFromSignature",  &tivars::TIModel::createFromSignature)
            ;
    }
#endif
