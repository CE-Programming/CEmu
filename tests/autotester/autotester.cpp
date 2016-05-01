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


static const std::vector<std::string> seq_valid_keys = { "action", "delay", "hash", "key" };

static const std::vector<std::string> valid_actions = { "launch", "reset" };

/* TODO */
struct coord2d { uint8_t y; uint8_t x; };
static const std::unordered_map<std::string, coord2d> valid_keys = {
    { "2nd",    { 3 , 4 } },
    { "alpha",  { 3 , 4 } },
    { "mode",   { 3 , 4 } },
    { "del",    { 3 , 4 } },
    { "clean",  { 3 , 4 } },
    { "enter",  { 3 , 4 } }
    /* ... */
};

struct hash_params_t {
    std::string description;
    std::string start;
    std::string size;
    std::vector<std::string> expected_CRCs;
};

struct config_t {
    std::string rom;
    std::vector<std::string> transfer_files;
    struct {
        std::string name;
        bool isASM;
    } target;
    std::vector<std::pair<std::string, std::string>> setup_sequence;
    std::unordered_map<std::string, hash_params_t> hashes;
};


/****** Utility functions ******/

inline bool file_exists(const std::string& name)
{
    std::ifstream f(name.c_str());
    return f.good();
}

template<typename T>
inline bool is_in_vector(const T& element, const std::vector<T>& v)
{
    return find(v.begin(), v.end(), element) != v.end();
}

/*******************************/

bool loadConfig(config_t& config, const Json& configJson)
{
    Json tmp, tmp2;

    tmp = configJson["rom"];
    if (tmp.is_string() && !tmp.string_value().empty())
    {
        config.rom = tmp.string_value();
        if (!file_exists(config.rom))
        {
            std::cerr << "[Error] The ROM file '" << config.rom << "' doesn't seem to exist (or requires higher permissions?)" << std::endl;
            return false;
        }
    } else {
        std::cerr << "[Error] \"rom\" parameter not given or invalid" << std::endl;
        return false;
    }

    tmp = configJson["transfer_files"];
    if (tmp.is_array() && !tmp.array_items().empty())
    {
        for (const auto& tmpFile : tmp.array_items())
        {
            if (tmpFile.is_string() && !tmpFile.string_value().empty())
            {
                std::string tmpFileStr = tmpFile.string_value();
                config.transfer_files.push_back(tmpFileStr);
                if (!file_exists(tmpFileStr))
                {
                    std::cerr << "[Error] The file to transfer '" << tmpFileStr << "' doesn't seem to exist (or requires higher permissions?)" << std::endl;
                    return false;
                }
            } else {
                std::cerr << "[Error] an item in \"transfer_files\" was not a string, or was empty" << std::endl;
                return false;
            }
        }
    } else {
        std::cerr << "[Error] \"transfer_files\" parameter not given or invalid/empty" << std::endl;
        return false;
    }

    tmp = configJson["target"];
    if (tmp.is_object())
    {
        tmp2 = tmp["name"];
        if (tmp2.is_string() && !tmp2.string_value().empty()) {
            config.target.name = tmp2.string_value();
        } else {
            std::cerr << "[Error] Target name parameter not given or invalid" << std::endl;
            return false;
        }
        tmp2 = configJson["target"]["isASM"];
        if (tmp2.is_bool()) {
            config.target.isASM = tmp2.bool_value();
        } else {
            std::cerr << "[Error] Target \"isASM\" parameter not given or invalid" << std::endl;
            return false;
        }
    } else {
        std::cerr << "[Error] \"target\" parameter not given or invalid" << std::endl;
        return false;
    }

    tmp = configJson["setup_sequence"];
    if (tmp.is_array() && !tmp.array_items().empty())
    {
        for (const auto& tmpSeqItem : tmp.array_items())
        {
            if (tmpSeqItem.is_string() && tmpSeqItem.string_value().find('|') != std::string::npos)
            {
                std::string tmpSeqItem_str = tmpSeqItem.string_value();
                size_t sep_pos = tmpSeqItem.string_value().find('|');
                if (sep_pos > 2 && sep_pos < tmpSeqItem_str.length()-1)
                {
                    config.setup_sequence.push_back(std::make_pair(tmpSeqItem_str.substr(0, sep_pos), tmpSeqItem_str.substr(sep_pos+1)));
                } else {
                    std::cerr << "[Error] an item in \"setup_sequence\" was malformed: '" << tmpSeqItem_str << "'" << std::endl;
                    return false;
                }
            } else {
                std::cerr << "[Error] an item in \"setup_sequence\" was not a string, or was malformed" << std::endl;
                return false;
            }
        }
    } else {
        std::cerr << "[Error] \"setup_sequence\" parameter not given or invalid/empty" << std::endl;
        return false;
    }

    tmp = configJson["hashes"];
    if (tmp.is_object())
    {
        for (const auto& tmpHashObj : tmp.object_items())
        {
            std::string tmpHashName = tmpHashObj.first;
            Json tmpHash = tmpHashObj.second;
            if (tmpHash.is_object())
            {
                hash_params_t hash_param = hash_params_t();

                if (tmpHash["description"].is_string() && !tmpHash["description"].string_value().empty()) {
                    hash_param.description = tmpHash["description"].string_value();
                } else {
                    std::cerr << "[Error] hash #" << tmpHashName << " config's description was not a string or was empty" << std::endl;
                    return false;
                }
                if (tmpHash["start"].is_string() && !tmpHash["start"].string_value().empty()) {
                    hash_param.start = tmpHash["start"].string_value();
                } else {
                    std::cerr << "[Error] hash #" << tmpHashName << " config's start was not a string or was empty" << std::endl;
                    return false;
                }
                if (tmpHash["size"].is_string() && !tmpHash["size"].string_value().empty()) {
                    hash_param.size = tmpHash["size"].string_value();
                } else {
                    std::cerr << "[Error] hash #" << tmpHashName << " config's size was not a string or was empty" << std::endl;
                    return false;
                }
                if (tmpHash["expected_CRCs"].is_array() && !tmpHash["expected_CRCs"].array_items().empty())
                {
                    for (const auto& tmpHashCRC : tmpHash["expected_CRCs"].array_items())
                    {
                        if (tmpHashCRC.is_string() && !tmpHashCRC.string_value().empty()) {
                            hash_param.expected_CRCs.push_back(tmpHashCRC.string_value());
                        } else {
                            std::cerr << "[Error] a CRC from hash #" << tmpHashName << "'s config was not a string or was empty" << std::endl;
                            return false;
                        }
                    }
                } else {
                    std::cerr << "[Error] hash #" << tmpHashName << " config's expected_CRCs was not an array or was empty" << std::endl;
                    return false;
                }

                config.hashes[tmpHashName] = hash_param;
            } else {
                std::cerr << "[Error] a hash config was not an object" << std::endl;
                return false;
            }
        }
    } else {
        std::cerr << "[Error] \"hashes\" parameter not given or invalid / not an object" << std::endl;
        return false;
    }

    return true;
}

int main(int argc, char* argv[])
{
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
    if (jsonError.empty())
    {
        std::cout << "[OK] JSON parsed" << std::endl;
    } else {
        std::cerr << "[Error] JSON parse error: " << jsonError << std::endl;
        return -1;
    }

    config_t config;
    if (loadConfig(config, configJson))
    {
        std::cout << "[OK] Test config loaded and verified" << std::endl;
    } else {
        std::cerr << "[Error] -> See the test config file format and make sure values are correct." << std::endl;
        return -1;
    }

/*
    TODO:
    - if ok, check presence of required files
    - if ok, launch cemucore with the rom specified in the json
    - if ok, follow the setup sequence specified in the json
    - return 0 if OK, 1 if test(s) failed, or -1 if program/json error.
*/

    return 0;
}