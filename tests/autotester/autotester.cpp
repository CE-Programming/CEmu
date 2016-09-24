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
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <regex>

#include "crc32.hpp"
#include "json11.hpp"

#include "autotester.h"

namespace autotester
{

/* The global config variable */
config_t config;

bool debugLogs = true;
bool ignoreROMfield = false;
bool configLoaded = false;
void (*stepCallback)(void) = nullptr;

#define DO_STEP_CALLBACK()  if (stepCallback) { stepCallback(); }

/* Will be incremented in case of matching CRC */
unsigned int hashesPassed = 0;
/* Will be incremented in case of non-matching CRC, and used as the return value */
unsigned int hashesFailed = 0;
/* Will be incremented at each `hash` command */
unsigned int hashesTested = 0;

struct coord2d { uint8_t x; uint8_t y; };
// Note: we could just store the string in a char*[8][8], then search for it and calculate its row/col at runtime, but meh.
static const std::unordered_map<std::string, coord2d> valid_keys = {
    {"graph", {0,1}}, {"trace", {1,1}}, { "zoom", {2,1}}, {"window", {3,1}}, {"y=", {4,1}}, {"2nd", {5,1}}, { "mode", {6,1}}, {  "del", {7,1}},
    {   "on", {0,2}}, {  "sto", {1,2}}, {   "ln", {2,2}}, {   "log", {3,2}}, {"^2", {4,2}}, { "-1", {5,2}}, { "math", {6,2}}, {"alpha", {7,2}},
    {    "0", {0,3}}, {    "1", {1,3}}, {    "4", {2,3}}, {     "7", {3,3}}, { ",", {4,3}}, {"sin", {5,3}}, { "apps", {6,3}}, { "xton", {7,3}},
    {  "(-)", {0,4}}, {    "2", {1,4}}, {    "5", {2,4}}, {     "8", {3,4}}, { "(", {4,4}}, {"cos", {5,4}}, { "prgm", {6,4}}, { "stat", {7,4}},
    {    ".", {0,5}}, {    "3", {1,5}}, {    "6", {2,5}}, {     "9", {3,5}}, { ")", {4,5}}, {"tan", {5,5}}, { "vars", {6,5}},
    {"enter", {0,6}}, {    "+", {1,6}}, {    "-", {2,6}}, {     "*", {3,6}}, { "/", {4,6}}, {  "^", {5,6}}, {"clear", {6,6}},
    { "down", {0,7}}, { "left", {1,7}}, {"right", {2,7}}, {    "up", {3,7}}
};

// Those aren't related to physical keys - they're keycodes for the OS.
#define CE_KEY_Enter    0x05
#define CE_KEY_Clear    0x09
#define CE_KEY_prgm     0xDA
#define CE_KEY_Asm      0x9CFC
#define CE_KEY_Classic  0xD3FB

// A few needed locations
#define CE_kbdKey       0xD0058C
#define CE_keyExtend    0xD0058E
void sendKey(uint16_t key)
{
    cemucore::sendKey(key);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    DO_STEP_CALLBACK();
}

void sendLetterKeyPress(char letter)
{
    cemucore::sendLetterKeyPress(letter);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    DO_STEP_CALLBACK();
}

typedef std::function<void(const std::string&)> seq_cmd_func_t;
typedef std::function<void(void)> seq_cmd_action_func_t;

static const std::unordered_map<std::string, seq_cmd_action_func_t> valid_actions = {
    {
        "launch", [] {
            // Assuming we're in the home screen...
            sendKey(CE_KEY_Clear);
            if (config.target.isASM) {
                sendKey(CE_KEY_Asm);
            }
            sendKey(CE_KEY_prgm);
            for (const char& c : config.target.name) {
                sendLetterKeyPress(c); // type program name
            }
            sendKey(CE_KEY_Enter);
        }
    },
    {
        "reset", [] {
            cemucore::cpuEvents |= EVENT_RESET;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    },
    {
        "useClassic", [] {
            // Assuming we're in the home screen...
            sendKey(CE_KEY_Clear);
            sendKey(CE_KEY_Classic);
            sendKey(CE_KEY_Enter);
            std::this_thread::sleep_for(std::chrono::milliseconds(125));
        }
    }
};

static const std::unordered_map<std::string, seq_cmd_func_t> valid_seq_commands = {
    {
        "action", [](const std::string &which_action) {
            valid_actions.at(which_action)();
        }
    },
    {
        "delay", [](const std::string& delay_str) {
            std::this_thread::sleep_for(std::chrono::milliseconds(std::stoul(delay_str)));
        }
    },
    {
        "hash", [](const std::string& which_hash) {
            const auto& tmp = config.hashes.find(which_hash);
            if (tmp != config.hashes.end())
            {
                const hash_params_t& param = tmp->second;
                const uint32_t real_hash = crc32(cemucore::phys_mem_ptr(param.start, param.size), param.size);
                if (std::find(param.expected_CRCs.begin(), param.expected_CRCs.end(), real_hash) != param.expected_CRCs.end())
                {
                    if (debugLogs) {
                        std::cout << "\t[Test passed!] Hash #" << which_hash << " had a matching CRC." << std::endl;
                    }
                    hashesPassed++;
                } else {
                    if (debugLogs) {
                        std::cout << "\t[Test failed!] Hash #" << which_hash << " (\"" << param.description << "\") did not match "
                                  << (param.expected_CRCs.size() > 1 ? "any of the expected CRCs" : "the expected CRC")
                                  << " (got " << std::uppercase << std::hex << real_hash << std::dec << ")." << std::endl;
                    }
                    hashesFailed++;
                }
                hashesTested++;
            } else {
                std::cerr << "\t[Error] hash #" << which_hash << " was not declared in the JSON file. Ignoring." << std::endl;
            }
        }
    },
    {
        "key", [](const std::string& which_key) {
            const auto& tmp = valid_keys.find(which_key);
            if (tmp != valid_keys.end())
            {
                const coord2d& key_coords = tmp->second;
                cemucore::keypad_key_event(key_coords.y, key_coords.x, true);
                std::this_thread::sleep_for(std::chrono::milliseconds(80));
                cemucore::keypad_key_event(key_coords.y, key_coords.x, false);
            } else {
                std::cerr << "\t[Error] unknown key \"" << which_key << "\" was not pressed." << std::endl;
            };
        }
    }
};

bool launchCommand(const std::pair<std::string, std::string>& command)
{
    const auto& func_it = valid_seq_commands.find(command.first);
    if (func_it != valid_seq_commands.end()) {
        (func_it->second)(command.second);
    } else {
        std::cerr << "\t[Error] invalid command \"" << command.first << "\"" << std::endl;
        return false;
    }
    return true;
}

/****** Utility functions ******/
inline bool file_exists(const std::string& name);
std::string str_replace_all(std::string str, const std::string& from, const std::string& to);

inline bool file_exists(const std::string& name)
{
    return std::ifstream(name.c_str()).good();
}

std::string str_replace_all(std::string str, const std::string& from, const std::string& to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

/*******************************/

bool loadJSONConfig(const std::string& jsonContents)
{
    std::string jsonError;
    json11::Json configJson = json11::Json::parse(jsonContents, jsonError);
    if (jsonError.empty())
    {
        if (debugLogs) {
            std::cout << "[OK] JSON parsed" << std::endl;
        }
    } else {
        std::cerr << "[Error] JSON parse error: " << jsonError << std::endl;
        return -1;
    }

    config = config_t();

    json11::Json tmp, tmp2;

    if (!ignoreROMfield)
    {
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
            std::string name_tmp = tmp2.string_value();
            if (std::regex_match(name_tmp, std::regex("[A-Z][0-9A-Zθ]{0,7}"))) {
                config.target.name = str_replace_all(name_tmp, "θ", "@");
            } else {
                std::cerr << "[Error] Target name parameter not a valid program name ([A-Z][0-9A-Zθ]{0,7})" << std::endl;
                return false;
            }
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

    tmp = configJson["sequence"];
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
                    std::string command = tmpSeqItem_str.substr(0, sep_pos);
                    std::string value = tmpSeqItem_str.substr(sep_pos+1);
                    if (valid_seq_commands.count(command))
                    {
                        if (command != "action" || valid_actions.count(value))
                        {
                            config.sequence.push_back(std::make_pair(command, value));
                        } else {
                            std::cerr << "[Error] bad value for \"action\": '" << value << "'" << std::endl;
                            return false;
                        }
                    } else {
                        std::cerr << "[Error] unknown sequence command in pair: '" << tmpSeqItem_str << "'" << std::endl;
                        return false;
                    }
                } else {
                    std::cerr << "[Error] an item in \"sequence\" was malformed: '" << tmpSeqItem_str << "'" << std::endl;
                    return false;
                }
            } else {
                std::cerr << "[Error] an item in \"sequence\" was not a string, or was malformed" << std::endl;
                return false;
            }
        }
    } else {
        std::cerr << "[Error] \"sequence\" parameter not given or invalid/empty" << std::endl;
        return false;
    }

    tmp = configJson["hashes"];
    if (tmp.is_object())
    {
        for (const auto& tmpHashObj : tmp.object_items())
        {
            std::string tmpHashName = tmpHashObj.first;
            json11::Json tmpHash = tmpHashObj.second;
            if (tmpHash.is_object())
            {
                hash_params_t hash_param = hash_params_t();

                if (tmpHash["description"].is_string() && !tmpHash["description"].string_value().empty()) {
                    hash_param.description = tmpHash["description"].string_value();
                } else {
                    std::cerr << "[Error] hash #" << tmpHashName << " config's description was not a string or was empty" << std::endl;
                    return false;
                }
                if (tmpHash["start"].is_string() && !tmpHash["start"].string_value().empty())
                {
                    std::string start_tmp = tmpHash["start"].string_value();
                    const auto& start_tmp_const = hash_consts.find(start_tmp);
                    if (start_tmp_const != hash_consts.end())
                    {
                        hash_param.start = start_tmp_const->second;
                    } else if (std::regex_match(start_tmp, std::regex("^(0x[0-9a-fA-F]+)|\\d+$"))) {
                        hash_param.start = (uint32_t)std::stoul(start_tmp, nullptr, (start_tmp.substr(0, 2) == "0x") ? 16 : 10);
                    } else {
                        std::cerr << "[Error] hash #" << tmpHashName << " config's start was invalid" << std::endl;
                        return false;
                    }
                } else {
                    std::cerr << "[Error] hash #" << tmpHashName << " config's start was not a string or was empty" << std::endl;
                    return false;
                }
                if (tmpHash["size"].is_number() || (tmpHash["size"].is_string() && !tmpHash["size"].string_value().empty()))
                {
                    std::string size_tmp = tmpHash["size"].is_number() ? std::to_string(tmpHash["size"].int_value()) : tmpHash["size"].string_value();
                    const auto& size_tmp_const = hash_consts.find(size_tmp);
                    if (size_tmp_const != hash_consts.end())
                    {
                        hash_param.size = size_tmp_const->second;
                    } else if (std::regex_match(size_tmp, std::regex("^(0x[0-9a-fA-F]+)|\\d+$"))) {
                        hash_param.size = (uint32_t)std::stoul(size_tmp, nullptr, (size_tmp.substr(0, 2) == "0x") ? 16 : 10);
                    } else {
                        std::cerr << "[Error] hash #" << tmpHashName << " config's size was invalid" << std::endl;
                        return false;
                    }
                } else {
                    std::cerr << "[Error] hash #" << tmpHashName << " config's size was not a string or was empty" << std::endl;
                    return false;
                }
                if (tmpHash["expected_CRCs"].is_array() && !tmpHash["expected_CRCs"].array_items().empty())
                {
                    for (const auto& tmpHashCRC : tmpHash["expected_CRCs"].array_items())
                    {
                        if (tmpHashCRC.is_string() && !tmpHashCRC.string_value().empty()) {
                            const std::string crc_tmp = tmpHashCRC.string_value();
                            if (std::regex_match(crc_tmp, std::regex("^[0-9a-fA-F]+$"))) {
                                hash_param.expected_CRCs.push_back((uint32_t)std::stoul(crc_tmp, nullptr, 16));
                            } else {
                                std::cerr << "[Error] the CRC '" << crc_tmp << "' from hash #" << tmpHashName << "'s config is not a valid hex string" << std::endl;
                                return false;
                            }
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

    configLoaded = true;
    return true;
}

bool sendFilesForTest()
{
    for (const auto& file : config.transfer_files)
    {
        if (debugLogs) {
            std::cout << "- Sending file " << file << "... " << std::endl;
        }
        if (!cemucore::sendVariableLink(file.c_str(), cemucore::LINK_FILE))
        {
            std::cerr << "[Error] File couldn't be sent" << std::endl;
            return false;
        }
        DO_STEP_CALLBACK();
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
    return true;
}

bool doTestSequence()
{
    hashesPassed = hashesFailed = hashesTested = 0;
    cemucore::keypad_reset();

    for (const auto& command : config.sequence)
    {
        if (debugLogs) {
            std::cout << "Launching command " << command.first << " | " << command.second << std::endl;
        }
        if (!launchCommand(command)) {
            return false;
        }
        DO_STEP_CALLBACK();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return true;
}

} // namespace autotester
