/*
 * Part of tivars_lib_cpp
 * (C) 2015 Adrien 'Adriweb' Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#ifndef TH_0x05_H
#define TH_0x05_H

#include "ITIVarTypeHandler.h"

namespace tivars
{

    enum { LANG_EN = 0, LANG_FR };

    // Type Handler for type 0x05: Program
    class TH_0x05 : public ITIVarTypeHandler
    {
    public:

        /**
         * Tokenizer
         *
         * @param   string  str        The program source as a string
         * @param   array   options    Ignored here (French and English token names supported by default)
         * @return  array   The bytes (tokens) array (the two first ones are the size of the rest)
         */
        static data_t makeDataFromString(const std::string& str, const options_t options);

        /**
         * Detokenizer
         *
         * @param   array   data       The bytes (tokens) array
         * @param   array   options    Associative array of options such as ['lang' => 'fr']
         * @return  string  The program source as a string (detokenized)
         * @throws  \Exception
         */
        static std::string makeStringFromData(const data_t& data, const options_t options);

        static std::string reindentCodeString(const std::string& str);

        static void initTokens();
        
    };
}

#endif