/*
 * Part of tivars_lib_cpp
 * (C) 2015-2018 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "TIVarTypes.h"
#include "utils.h"

namespace tivars
{
    std::unordered_map<std::string, TIVarType> types;

// Wrap the makeDataFromStr function by one that adds the type/subtype in the options
// Ideally, the handlers would parse the string and select the correct handler to dispatch...
#define GenericHandlerPair(which, type) make_pair([](const std::string& str, const options_t& options) -> data_t { \
    options_t options_withType = options;                                                                          \
    options_withType["_type"] = type;                                                                              \
    return (TH_Generic##which::makeDataFromString)(str, options_withType);                                         \
}, &TH_Generic##which::makeStringFromData)

    /**
     *  Make and insert the associative arrays for the type.
     *
     * @param string    name        The name of the type
     * @param int       id          The ID of the type
     * @param vector    exts        The extensions the type can have, ordered by feature flags.
     * @param pair      handlers    The data2str and str2data funcs
     */
    void TIVarTypes::insertType(const std::string& name, int id, const std::vector<std::string>& exts, const handler_pair_t& handlers)
    {
        TIVarType varType(id, name, exts, handlers);
        std::string id_str = std::to_string(id);
        types[name]   = varType;
        types[id_str] = varType;
        for (const std::string& ext : exts)
        {
            if (!ext.empty() && !types.count(ext))
            {
                types[ext] = varType;
            }
        }
    }

    // 82+/83+/84+ are grouped since only the clock is the difference, and it doesn't have an actual varType.
    // For number vartypes, Real and Complex are the generic handlers we use, they'll dispatch to specific ones.
    void TIVarTypes::initTIVarTypesArray() // order: 82     83    82A  82+/83+  84+C  84+CE  83PCE
                                           //                     84+T   84+         84+CE-T
    {
        insertType("Unknown",                -1,  {  "" ,   "" ,   "" ,   "" ,   "" ,   "" ,   "" });

        /* Standard types */
        insertType("Real",                 0x00,  {"82n", "83n", "8xn", "8xn", "8xn", "8xn", "8xn"},  GenericHandlerPair(Real, 0x00)  );
        insertType("RealList",             0x01,  {"82l", "83l", "8xl", "8xl", "8xl", "8xl", "8xl"},  GenericHandlerPair(List, 0x00)  );
        insertType("Matrix",               0x02,  {"82m", "83m", "8xm", "8xm", "8xm", "8xm", "8xm"},  make_handler_pair(TH_Matrix)    );
        insertType("Equation",             0x03,  {"82y", "83y", "8xy", "8xy", "8xy", "8xy", "8xy"},  make_handler_pair(TH_Tokenized) );
        insertType("String",               0x04,  {"82s", "83s", "8xs", "8xs", "8xs", "8xs", "8xs"},  make_handler_pair(TH_Tokenized) );
        insertType("Program",              0x05,  {"82p", "83p", "8xp", "8xp", "8xp", "8xp", "8xp"},  make_handler_pair(TH_Tokenized) );
        insertType("ProtectedProgram",     0x06,  {"82p", "83p", "8xp", "8xp", "8xp", "8xp", "8xp"},  make_handler_pair(TH_Tokenized) );
        insertType("Picture",              0x07,  {  "" ,   "" , "8xi", "8xi", "8ci", "8ci", "8ci"});
        insertType("GraphDataBase",        0x08,  {"82d", "83d", "8xd", "8xd", "8xd", "8xd", "8xd"});
        insertType("Complex",              0x0C,  {  "" , "83c", "8xc", "8xc", "8xc", "8xc", "8xc"},  GenericHandlerPair(Complex, 0x0C) );
        insertType("ComplexList",          0x0D,  {  "" , "83l", "8xl", "8xl", "8xl", "8xl", "8xl"},  GenericHandlerPair(List,    0x0C) );
        insertType("WindowSettings",       0x0F,  {"82w", "83w", "8xw", "8xw", "8xw", "8xw", "8xw"});
        insertType("RecallWindow",         0x10,  {"82z", "83z", "8xz", "8xz", "8xz", "8xz", "8xz"});
        insertType("TableRange",           0x11,  {"82t", "83t", "8xt", "8xt", "8xt", "8xt", "8xt"});
        insertType("Backup",               0x13,  {"82b", "83b",   "" , "8xb", "8cb",   "" ,   "" });
        insertType("AppVar",               0x15,  {  "" ,   "" ,   "" , "8xv", "8xv", "8xv", "8xv"},  GenericHandlerPair(AppVar, 0x15) );
        insertType("PythonAppVar",         0x15,  {  "" ,   "" ,   "" ,   "" ,   "" ,   "" , "8xv"},  make_handler_pair(STH_PythonAppVar) );
        insertType("TemporaryItem",        0x16,  {  "" ,   "" ,   "" ,   "" ,   "" ,   "" ,   "" });
        insertType("GroupObject",          0x17,  {"82g", "83g", "8xg", "8xg", "8xg", "8cg", "8cg"});
        insertType("RealFraction",         0x18,  {  "" ,   "" ,   "" , "8xn", "8xn", "8xn", "8xn"},  GenericHandlerPair(Real, 0x18) );
        insertType("Image",                0x1A,  {  "" ,   "" ,   "" ,   "" ,   "" , "8ca", "8ca"});

        /* Exact values (TI-83 Premium CE) */
        /* See https://docs.google.com/document/d/1P_OUbnZMZFg8zuOPJHAx34EnwxcQZ8HER9hPeOQ_dtI */
        insertType("ExactComplexFrac",     0x1B,  {  "" ,   "" ,   "" ,   "" ,   "" ,   "" , "8xc"},  GenericHandlerPair(Complex, 0x1B) );
        insertType("ExactRealRadical",     0x1C,  {  "" ,   "" ,   "" ,   "" ,   "" ,   "" , "8xn"},  GenericHandlerPair(Real,    0x1C) );
        insertType("ExactComplexRadical",  0x1D,  {  "" ,   "" ,   "" ,   "" ,   "" ,   "" , "8xc"},  GenericHandlerPair(Complex, 0x1D) );
        insertType("ExactComplexPi",       0x1E,  {  "" ,   "" ,   "" ,   "" ,   "" ,   "" , "8xc"},  GenericHandlerPair(Complex, 0x1E) );
        insertType("ExactComplexPiFrac",   0x1F,  {  "" ,   "" ,   "" ,   "" ,   "" ,   "" , "8xc"},  GenericHandlerPair(Complex, 0x1F) );
        insertType("ExactRealPi",          0x20,  {  "" ,   "" ,   "" ,   "" ,   "" ,   "" , "8xn"},  GenericHandlerPair(Real,    0x20) );
        insertType("ExactRealPiFrac",      0x21,  {  "" ,   "" ,   "" ,   "" ,   "" ,   "" , "8xn"},  GenericHandlerPair(Real,    0x21) );

        /* System/Flash-related things */
        insertType("OperatingSystem",      0x23,  {"82u", "83u", "82u", "8xu", "8cu", "8eu", "8pu"});
        insertType("FlashApp",             0x24,  {  "" ,   "" ,   "" , "8xk", "8ck", "8ek", "8ek"});
        insertType("Certificate",          0x25,  {  "" ,   "" ,   "" , "8xq", "8cq",   "" ,   "" });
        insertType("CertificateMemory",    0x27,  {  "" ,   "" ,   "" ,   "" ,   "" ,   "" ,   "" });
        insertType("Clock",                0x29,  {  "" ,   "" ,   "" ,   "" ,   "" ,   "" ,   "" });
        insertType("FlashLicense",         0x3E,  {  "" ,   "" ,   "" ,   "" ,   "" ,   "" ,   "" });

        // WindowSettings clone thing
        types["0x0B"] = types["0x0F"];
    }

    /**
     * @param   int     id     The type ID
     * @return  string          The type name for that ID
     */
    std::string TIVarTypes::getNameFromID(int id)
    {
        std::string id_str = std::to_string(id);
        if (id != -1 && types.count(id_str))
        {
            return types[id_str].getName();
        } else {
            return "Unknown";
        }
    }

    /**
     * @param   string  name   The type name
     * @return  int             The type ID for that name
     */
    int TIVarTypes::getIDFromName(const std::string& name)
    {
        if (!name.empty() && types.count(name))
        {
            return types[name].getId();
        } else {
            return -1;
        }
    }

    /**
     * @param   int     id     The type ID
     * @return  string[]        The array of extensions for that ID
     */
    std::vector<std::string> TIVarTypes::getExtensionsFromTypeID(int id)
    {
        std::string id_str = std::to_string(id);
        if (id != -1 && types.count(id_str))
        {
            return types[id_str].getExts();
        } else {
            return {};
        }
    }

    /**
     * @param   string  name
     * @return  string[]        The array of extensions for that ID
     */
    std::vector<std::string> TIVarTypes::getExtensionsFromName(const std::string& name)
    {
        if (!name.empty() && types.count(name))
        {
            return types[name].getExts();
        } else {
            return {};
        }
    }

    bool TIVarTypes::isValidID(int id)
    {
        std::string id_str = std::to_string(id);
        return (id != -1 && types.count(id_str));
    }

    bool TIVarTypes::isValidName(const std::string& name)
    {
        return (!name.empty() && types.count(name));
    }
}

//TIVarTypes::initTIVarTypesArray();
