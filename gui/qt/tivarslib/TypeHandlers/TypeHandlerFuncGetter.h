/*
 * Part of tivars_lib_cpp
 * (C) 2015 Adrien 'Adriweb' Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "ITIVarTypeHandler.h"
#include "TH_0x00.h"
#include "TH_0x01.h"
#include "TH_0x03.h"
#include "TH_0x04.h"
#include "TH_0x05.h"
#include "TH_0x06.h"

#ifndef TIVARS_LIB_CPP_TYPEHANDLERFUNCGETTER_H
#define TIVARS_LIB_CPP_TYPEHANDLERFUNCGETTER_H

namespace tivars
{

    class TypeHandlerFuncGetter
    {

    public:

        static decltype(&ITIVarTypeHandler::makeStringFromData) getStringFromDataFunc(int type)
        {
            auto func = &ITIVarTypeHandler::makeStringFromData;
            switch (type)
            {
                case 0x00:
                    func = &TH_0x00::makeStringFromData;
                    break;

                case 0x01:
                    func = &TH_0x01::makeStringFromData;
                    break;

                case 0x03:
                    func = &TH_0x03::makeStringFromData; // We could actually directly use TH_0x05 here, but that might be confusing.
                    break;

                case 0x04:
                    func = &TH_0x04::makeStringFromData; // Same comment as 0x03 here.
                    break;

                case 0x05:
                case 0x06:
                    func = &TH_0x05::makeStringFromData;
                    break;

                default:
                    break;
            }
            return func;
        }

        static decltype(&ITIVarTypeHandler::makeDataFromString) getDataFromStringFunc(int type)
        {
            auto func = &ITIVarTypeHandler::makeDataFromString;
            switch (type)
            {
                case 0x00:
                    func = &TH_0x00::makeDataFromString;
                    break;

                case 0x01:
                    func = &TH_0x01::makeDataFromString;
                    break;

                case 0x03:
                    func = &TH_0x03::makeDataFromString; // We could actually directly use TH_0x05 here, but that might be confusing.
                    break;

                case 0x04:
                    func = &TH_0x04::makeDataFromString; // Same comment as 0x03 here.
                    break;

                case 0x05:
                case 0x06:
                    func = &TH_0x05::makeDataFromString;
                    break;

                default:
                    break;
            }
            return func;
        }
    };

}

#endif //TIVARS_LIB_CPP_TYPEHANDLERFUNCGETTER_H
