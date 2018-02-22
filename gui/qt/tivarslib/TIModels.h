/*
 * Part of tivars_lib_cpp
 * (C) 2015-2018 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#ifndef TIMODELS_H
#define TIMODELS_H

#include "CommonTypes.h"
#include "TIModel.h"
#include "TIVarType.h"

namespace tivars
{

    enum TIFeatureFlags
    {
        has82things  = 0b00000001, // (1 << 0);
        hasComplex   = 0b00000010, // (1 << 1);
        hasFlash     = 0b00000100, // (1 << 2);
        hasApps      = 0b00001000, // (1 << 3);
        hasClock     = 0b00010000, // (1 << 4);
        hasColorLCD  = 0b00100000, // (1 << 5);
        hasEZ80CPU   = 0b01000000, // (1 << 6);
        hasExactMath = 0b10000000, // (1 << 7);
    };

    class TIModels
    {

    public:

        static void initTIModelsArray();

        static std::string getDefaultNameFromFlags(uint flags);

        /**
         * @param   std::string  name   The model name
         * @return  int             The model flags for that name
         */
        static uint getFlagsFromName(const std::string& name);

        /**
         * @param   int     flags  The model flags
         * @return  std::string          The signature for those flags
         */
        static std::string getSignatureFromFlags(uint flags);

        /**
         * @param   std::string  name
         * @return  std::string          The signature for that name
         */
        static std::string getSignatureFromName(const std::string& name);
        /**
         * @param   std::string  sig    The signature
         * @return  std::string          The default calc name whose file formats use that signature
         */
        static std::string getDefaultNameFromSignature(const std::string& sig);

        /**
         * @param   std::string  sig    The signature
         * @return  int             The default calc order ID whose file formats use that signature
         */
        static int getDefaultOrderIDFromSignature(const std::string& sig);

        /**
         * @param   std::string  name
         * @return  int             The default calc order ID whose file formats use that signature
         */
        static int getOrderIDFromName(std::string name);

        /**
         * @param   int     flags  The model flags
         * @return  int             The default calc order ID whose file formats use that signature
         */
        static int getDefaultOrderIDFromFlags(uint flags);

        /**
         * @param   std::string  sig    The signature
         * @return  std::string          The minimum compatibility flags for that signaure
         */
        static uint getMinFlagsFromSignature(const std::string& sig);


        static bool isValidFlags(uint flags);

        static bool isValidName(const std::string& name);

        static bool isValidSignature(const std::string& sig);

    private:
        static void insertModel(int orderID, uint flags, const std::string& name, const std::string& sig);

    };

}

#endif
