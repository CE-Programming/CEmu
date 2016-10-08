/*
 * Part of tivars_lib_cpp
 * (C) 2015-2016 Adrien 'Adriweb' Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "TypeHandlers.h"

#ifndef TIVARS_LIB_CPP_TYPEHANDLERFUNCGETTER_H
#define TIVARS_LIB_CPP_TYPEHANDLERFUNCGETTER_H

namespace tivars
{

    class TypeHandlerFuncGetter
    {

    public:

        static decltype(&DummyHandler::makeStringFromData) getStringFromDataFunc(int type)
        {
            auto func = &DummyHandler::makeStringFromData;
            switch (type)
            {
                case 0x00:
                    func = &TH_0x00::makeStringFromData;
                    break;

                case 0x01:
                    func = &TH_0x01::makeStringFromData;
                    break;

                case 0x02:
                    func = &TH_0x02::makeStringFromData;
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

                case 0x0C:
                    func = &TH_0x0C::makeStringFromData;
                    break;

                case 0x0D:
                    func = &TH_0x0D::makeStringFromData;
                    break;

                case 0x1B:
                    func = &TH_0x1B::makeStringFromData;
                    break;

                case 0x1C:
                    func = &TH_0x1C::makeStringFromData;
                    break;

                case 0x1D:
                    func = &TH_0x1D::makeStringFromData;
                    break;

                case 0x1E:
                    func = &TH_0x1E::makeStringFromData;
                    break;

                case 0x1F:
                    func = &TH_0x1F::makeStringFromData;
                    break;

                case 0x20:
                    func = &TH_0x20::makeStringFromData;
                    break;

                case 0x21:
                    func = &TH_0x21::makeStringFromData;
                    break;

                default:
                    break;
            }
            return func;
        }

        static decltype(&DummyHandler::makeDataFromString) getDataFromStringFunc(int type)
        {
            auto func = &DummyHandler::makeDataFromString;
            switch (type)
            {
                case 0x00:
                    func = &TH_0x00::makeDataFromString;
                    break;

                case 0x01:
                    func = &TH_0x01::makeDataFromString;
                    break;

                case 0x02:
                    func = &TH_0x02::makeDataFromString;
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

                case 0x0C:
                    func = &TH_0x0C::makeDataFromString;
                    break;

                case 0x0D:
                    func = &TH_0x0D::makeDataFromString;
                    break;

                case 0x1B:
                    func = &TH_0x1B::makeDataFromString;
                    break;

                case 0x1C:
                    func = &TH_0x1C::makeDataFromString;
                    break;

                case 0x1D:
                    func = &TH_0x1D::makeDataFromString;
                    break;

                case 0x1E:
                    func = &TH_0x1E::makeDataFromString;
                    break;

                case 0x1F:
                    func = &TH_0x1F::makeDataFromString;
                    break;

                case 0x20:
                    func = &TH_0x20::makeDataFromString;
                    break;

                case 0x21:
                    func = &TH_0x21::makeDataFromString;
                    break;

                default:
                    break;
            }
            return func;
        }
    };

}

#endif //TIVARS_LIB_CPP_TYPEHANDLERFUNCGETTER_H
