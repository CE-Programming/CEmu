/*
 * Autotester
 * (C) Adrien 'Adriweb' Bertrand
 * Part of the CEmu project
 * License: GPLv3
 */

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include "crc32.hpp"
#include "json11.hpp"

using namespace json11;

struct hash_params_t {
    unsigned int id;
    std::string description;
    std::string start;
    std::string size;
    std::vector<std::string> expected_CRCs;
};

struct target_t {
    std::string name;
    bool isASM;
};

struct config_t {
    std::string rom;
    std::vector<std::string> transfer_files;
    target_t target;
    std::vector<std::string> setup_sequence;
    std::vector<hash_params_t> hashes;
};


/* Utility functions */

inline bool file_exists(const std::string& name)
{
    std::ifstream f(name.c_str());
    return f.good();
}


bool loadConfig(config_t& config, const Json& configJson)
{
    Json tmp;

    tmp = configJson["rom"];
    if (tmp.is_string()) {
        config.rom = tmp.string_value();
    } else {
        std::cerr << "[Error] \"rom\" parameter not given or invalid" << std::endl;
        return false;
    }

    tmp = configJson["transfer_files"];
    if (tmp.is_array()) {
        for (const auto& tmpFile : tmp.object_items())
        {
            if (tmpFile.second.is_string())
            {
                config.transfer_files.push_back(tmpFile.second.string_value());
            }
        }
    } else {
        std::cerr << "[Error] \"transfer_files\" parameter not given or invalid" << std::endl;
        return false;
    }

    tmp = configJson["target"];
    if (configJson["target"].is_object()) {
        tmp = configJson["target"]["name"];
        if (tmp.is_string()) {
            config.target.name = tmp.string_value();
        } else {
            std::cerr << "[Error] Target name parameter not given or invalid" << std::endl;
            return false;
        }
        tmp = configJson["target"]["type"];
        if (tmp.is_bool()) {
            config.target.isASM = tmp.bool_value();
        } else {
            std::cerr << "[Error] Target type parameter not given or invalid" << std::endl;
            return false;
        }
    } else {
        std::cerr << "[Error] \"target\" parameter not given or invalid" << std::endl;
        return false;
    }

    /* TODO: rest of the json structure */

    return true;
}

int main(int argc, char* argv[])
{
    /* We'll use the retVal as the number of failed tests.
       -1 indicate an error in the tester/input */
    int retval = 0;

    if (argc != 2)
    {
        std::cerr << "[Error] Need one argument: path to the test config JSON file" << std::endl;
        return -1;
    }

    const std::string jsonPath(argv[1]);
    std::string jsonContents;
    if (file_exists(jsonPath))
    {
        std::ifstream ifs(jsonPath);
        jsonContents = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
    } else {
        std::cerr << "[Error] Couldn't open JSON file at provided path" << std::endl;
        return -1;
    }

    std::string jsonError;
    Json configJson = Json::parse(jsonContents, jsonError);
    if (!jsonError.empty())
    {
        std::cerr << "[Error] Json parse error: " << jsonError << std::endl;
        return -1;
    }

    config_t config;
    if (!loadConfig(config, configJson))
    {
        std::cerr << "[Error] -> See the test config file format." << std::endl;
        return -1;
    }


/*
    TODO:
    - parse (loadConfig)
    - if ok, check presence of required files
    - if ok, launch cemucore with the rom specified in the json
    - if ok, follow the setup sequence specified in the json
    - return retval == number of fails, or -1 if program/json error
*/


    return retval;
}