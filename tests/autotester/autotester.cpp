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
#include <regex>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
  #include <windows.h>
#else
  #include <glob.h>
#endif

#include "crc32.hpp"
#include "json11.hpp"

#include "autotester.h"

static char* myrealpath(const char* file_name)
{
    static char path_buf[8192 + 1] = {};
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    char *p;
    char tmp[8192 + 1];
    strncpy(tmp, file_name, sizeof(tmp));
    p = tmp;
    while (*p) {
        if (*p == '/') { *p = '\\'; }
        p++;
    }
    return _fullpath(path_buf, tmp, 8192);
#else
    return realpath(file_name, path_buf);
#endif
}

namespace autotester
{

/* The global config variable */
config_t config;

bool debugMode = true;
bool ignoreROMfield = false;
bool configLoaded = false;

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

void sendCSC(uint8_t csc)
{
    int retry = 200;
    do {
        if (cemucore::sendCSC(csc)) {
            break;
        }
        cemucore::emu_run(50);
    } while(retry--);
}

void sendKey(uint16_t key)
{
    int retry = 200;
    do {
        if (cemucore::sendKey(key)) {
            break;
        }
        cemucore::emu_run(50);
    } while(retry--);
}

void sendLetterKeyPress(char letter)
{
    int retry = 200;
    do {
        if (cemucore::sendLetterKeyPress(letter)) {
            break;
        }
        cemucore::emu_run(50);
    } while(retry--);
}

typedef std::function<void(const std::string&)> seq_cmd_func_t;
typedef std::function<void(void)> seq_cmd_action_func_t;

static const std::unordered_map<std::string, seq_cmd_action_func_t> valid_actions = {
    {
        "launch", [] {
            // Assuming we're in the home screen...
            sendKey(CE_KEY_CLEAR);
            if (config.target.isASM) {
                sendKey(CE_KEY_ASM);
            }
            sendKey(CE_KEY_PRGM);
            for (const char& c : config.target.name) {
                sendLetterKeyPress(c); // type program name
            }
            sendKey(CE_KEY_ENTER);
        }
    },
    {
        "reset", [] {
            cemucore::cpu_crash("autotester action");
            cemucore::emu_run(1000);
        }
    },
    {
        "useClassic", [] {
            // Assuming we're in the home screen...
            sendKey(CE_KEY_CLEAR);
            sendKey(CE_KEY_CLASSIC);
            sendKey(CE_KEY_ENTER);
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
            unsigned long delay = std::stoul(delay_str);
            while (delay) {
                unsigned long amount = (std::min)(delay, 100ul);
                cemucore::emu_run(amount);
                delay -= amount;
            }
        }
    },
    {
        "hash", [](const std::string& which_hash) {
            const auto& tmp = config.hashes.find(which_hash);
            if (tmp != config.hashes.end())
            {
                void *temp_buffer = nullptr;
                void *temp_buffer_dup = nullptr;
                uint32_t real_hash;
                const hash_params_t& param = tmp->second;
                bool match = false;

                int32_t delay = param.timeout_ms;

                do
                {
                    temp_buffer = cemucore::virt_mem_dup(param.start, static_cast<int32_t>(param.size));
                    real_hash = crc32(temp_buffer, param.size);
                    match = (std::find(param.expected_CRCs.begin(), param.expected_CRCs.end(), real_hash) != param.expected_CRCs.end());
                    if (debugMode && !match) {
                        ::free(temp_buffer_dup);
                        temp_buffer_dup = malloc(param.size);
                        memcpy(temp_buffer_dup, temp_buffer, param.size);
                    }
                    ::free(temp_buffer);
                    int32_t amount = (std::min)(delay, 100);
                    cemucore::emu_run(amount);
                    delay -= amount;
                } while (delay > 0 && !match);

                if (match)
                {
                    if (debugMode) {
                        std::cout << "\t[Test passed!] Hash #" << which_hash << " had a matching CRC." << std::endl;
                    }
                    hashesPassed++;
                } else {
                    char buf[20] = {0};
                    sprintf(buf, "%X", param.expected_CRCs[0]);
                    const std::string expected_hash_str(buf);
                    sprintf(buf, "%X", real_hash);
                    const std::string real_hash_str(buf);
                    std::cout << "\t[Test failed!] Hash #" << which_hash << " (\"" << param.description << "\") did not match "
                              << (param.expected_CRCs.size() > 1 ? "any of the expected CRCs" : ("the expected CRC " + expected_hash_str))
                              << " (got " << real_hash_str << ")." << std::endl;
                    if (debugMode) {
                        char dump_path[150] = {0};
                        sprintf(dump_path, "failure_hash%s_num%d_dump.bin", which_hash.c_str(), hashesTested+1);
                        FILE *dump_file = fopen(dump_path, "wb");
                        fwrite(temp_buffer_dup, param.size, 1, dump_file);
                        fclose(dump_file);
                        std::cout << "\tDumped memory into " << dump_path << std::endl;
                    }
                    hashesFailed++;
                }
                if (debugMode) {
                    ::free(temp_buffer_dup);
                }
                hashesTested++;
            } else {
                std::cerr << "\t[Error] hash #" << which_hash << " was not declared in the JSON file. Ignoring." << std::endl;
            }
        }
    },
    {
        "hashWait", [](const std::string& which_hash) {
            const auto& tmp = config.hashes.find(which_hash);
            if (tmp != config.hashes.end())
            {
                hash_params_t& param = tmp->second;
                if (param.timeout_ms < 0)
                {
                    if (debugMode) {
                        std::cout << "\t[Info] hash #" << which_hash << " is a hashWait without a proper timeout value. Using 1000ms." << std::endl;
                    }
                    param.timeout_ms = 1000; // default
                }
                valid_seq_commands.at("hash")(which_hash);
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
                cemucore::emu_keypad_event(key_coords.y, key_coords.x, true);
                cemucore::emu_run(80);
                cemucore::emu_keypad_event(key_coords.y, key_coords.x, false);
                if (config.delay_after_key > 0)
                {
                    cemucore::emu_run(config.delay_after_key);
                }
            } else {
                std::cerr << "\t[Error] unknown key \"" << which_key << "\" was not pressed." << std::endl;
            };
        }
    },
    {
        "hold", [](const std::string& which_key) {
            const auto& tmp = valid_keys.find(which_key);
            if (tmp != valid_keys.end())
            {
                const coord2d& key_coords = tmp->second;
                cemucore::emu_keypad_event(key_coords.y, key_coords.x, true);
            } else {
                std::cerr << "\t[Error] unknown key \"" << which_key << "\" was not hold." << std::endl;
            };
        }
    },
    {
        "release", [](const std::string& which_key) {
            const auto& tmp = valid_keys.find(which_key);
            if (tmp != valid_keys.end())
            {
                const coord2d& key_coords = tmp->second;
                cemucore::emu_keypad_event(key_coords.y, key_coords.x, false);
            } else {
                std::cerr << "\t[Error] unknown key \"" << which_key << "\" was not released." << std::endl;
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
        if (debugMode) {
            std::cout << "[OK] JSON parsed" << std::endl;
        }
    } else {
        std::cerr << "[Error] JSON parse error: " << jsonError << std::endl;
        return false;
    }

    config = config_t();

    json11::Json tmp, tmp2;

    const char* forced_rom_path = getenv("AUTOTESTER_ROM");
    if (forced_rom_path)
    {
        const std::string tmp_path(forced_rom_path);
        if (file_exists(tmp_path))
        {
            config.rom = forced_rom_path;
            ignoreROMfield = true;
        } else {
            std::cerr << "[Error] AUTOTESTER_ROM was given but the file does not exist!" << std::endl;
        }
    }

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
                const std::string& tmpFileStr = myrealpath(tmpFile.string_value().c_str());
                if (!file_exists(tmpFileStr))
                {
                    std::cerr << "[Error] The file to transfer '" << tmpFileStr << "' doesn't seem to exist (or requires higher permissions?)" << std::endl;
                    return false;
                }
                config.transfer_files.push_back(tmpFileStr);
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
            const std::string& name_tmp = tmp2.string_value();
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

    if (configJson.object_items().count("delay_after_key"))
    {
        tmp = configJson["delay_after_key"];
        if (tmp.is_number())
        {
            unsigned int delay = static_cast<unsigned int>(tmp.int_value());
            if (delay < 10000)
            {
                config.delay_after_key = delay;
            } else {
                std::cerr << "[Error] bad value for \"delay_after_key\": '" << delay << "'. Range: 0<x<10000." << std::endl;
                return false;
            }
        } else {
            std::cerr << "[Error] bad type for \"delay_after_key\", unsigned integer needed." << std::endl;
            return false;
        }
    }

    if (configJson.object_items().count("delay_after_step"))
    {
        tmp = configJson["delay_after_step"];
        if (tmp.is_number())
        {
            unsigned int delay = static_cast<unsigned int>(tmp.int_value());
            if (delay < 10000)
            {
                config.delay_after_step = delay;
            } else {
                std::cerr << "[Error] bad value for \"delay_after_step\": '" << delay << "'. Range: 0<x<10000." << std::endl;
                return false;
            }
        } else {
            std::cerr << "[Error] bad type for \"delay_after_step\", unsigned integer needed." << std::endl;
            return false;
        }
    }

    tmp = configJson["sequence"];
    if (tmp.is_array() && !tmp.array_items().empty())
    {
        for (const auto& tmpSeqItem : tmp.array_items())
        {
            if (tmpSeqItem.is_string() && tmpSeqItem.string_value().find('|') != std::string::npos)
            {
                const std::string& tmpSeqItem_str = tmpSeqItem.string_value();
                size_t sep_pos = tmpSeqItem.string_value().find('|');
                if (sep_pos > 2 && sep_pos < tmpSeqItem_str.length()-1)
                {
                    std::string command = tmpSeqItem_str.substr(0, sep_pos);
                    std::string value = tmpSeqItem_str.substr(sep_pos+1);
                    if (valid_seq_commands.count(command))
                    {
                        if (command != "action" || valid_actions.count(value))
                        {
                            config.sequence.emplace_back(command, value);
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

                hash_param.timeout_ms = tmpHash["timeout"].is_number() ? tmpHash["timeout"].int_value() : -1;

                if (tmpHash["start"].is_string() && !tmpHash["start"].string_value().empty())
                {
                    std::string start_tmp = tmpHash["start"].string_value();
                    const auto& start_tmp_const = hash_consts.find(start_tmp);
                    if (start_tmp_const != hash_consts.end())
                    {
                        hash_param.start = start_tmp_const->second;
                    } else if (std::regex_match(start_tmp, std::regex("^(0x[0-9a-fA-F]+)|\\d+$"))) {
                        hash_param.start = static_cast<uint32_t>(std::stoul(start_tmp, nullptr, (start_tmp.substr(0, 2) == "0x") ? 16 : 10));
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
                        hash_param.size = static_cast<uint32_t>(std::stoul(size_tmp, nullptr, (size_tmp.substr(0, 2) == "0x") ? 16 : 10));
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
                            const std::string& crc_tmp = tmpHashCRC.string_value();
                            if (std::regex_match(crc_tmp, std::regex("^[0-9a-fA-F]+$"))) {
                                hash_param.expected_CRCs.push_back(static_cast<uint32_t>(std::stoul(crc_tmp, nullptr, 16)));
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

std::vector<std::string> globVector(const std::string& pattern)
{
    std::vector<std::string> files;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    WIN32_FIND_DATAA find_result;
    HANDLE find_handle = FindFirstFileA(pattern.c_str(), &find_result);
    if (find_handle != INVALID_HANDLE_VALUE) {
        do {
            files.push_back(pattern.substr(0, pattern.find_last_of("/\\") + 1) + find_result.cFileName);
        } while (FindNextFileA(find_handle, &find_result));
        FindClose(find_handle);
    }
#else
#if defined(GLOB_SUPPORT)
    glob_t glob_result;
    if (!glob(pattern.c_str(), GLOB_TILDE, nullptr, &glob_result))
    {
        for (unsigned int i=0; i<glob_result.gl_pathc; i++)
        {
            files.emplace_back(glob_result.gl_pathv[i]);
        }
        globfree(&glob_result);
    }
#else
    #warning GLOB_SUPPORT is not defined, the vector returned by this function will be empty!
    static_cast<void>(pattern);
#endif
#endif
    return files;
}

bool sendFilesForTest()
{
    std::vector<std::string> forced_files;
    const char* forced_libs_group = getenv("AUTOTESTER_LIBS_GROUP");
    const char* forced_libs_dir   = getenv("AUTOTESTER_LIBS_DIR");

    if (forced_libs_group)
    {
        if (file_exists(forced_libs_group))
        {
            forced_files.emplace_back(forced_libs_group);
        } else {
            std::cerr << "[Error] Env var for libs group given, but no such file...?" << std::endl;
            forced_libs_group = nullptr;
        }
    }

    if (!forced_libs_group && forced_libs_dir)
    {
        forced_files = globVector(std::string(forced_libs_dir) + "/*.8xv");
    }

    if ((forced_libs_group || forced_libs_dir) && forced_files.empty())
    {
        std::cerr << "[Error] Env var for libs-dir/group given, but no files found...?" << std::endl;
        return false;
    } else {
        for (const auto& file : forced_files)
        {
            if (debugMode)
            {
                std::cout << "- Sending forced file " << file << "... ";
            }
            if (cemucore::emu_send_variable(file.c_str(), cemucore::LINK_FILE) != cemucore::LINK_GOOD)
            {
                if (debugMode)
                {
                    std::cout << std::endl;
                }
                std::cerr << "[Error] Forced file couldn't be sent" << std::endl;
                return false;
            }
            if (debugMode)
            {
                std::cout << "[OK]" << std::endl;
            }
        }
    }

    for (const auto& file : config.transfer_files)
    {
        if (debugMode)
        {
            std::cout << "- Sending file " << file << "... ";
        }
        if (cemucore::emu_send_variable(file.c_str(), cemucore::LINK_FILE) != cemucore::LINK_GOOD)
        {
            if (debugMode)
            {
                std::cout << std::endl;
            }
            std::cerr << "[Error] File couldn't be sent" << std::endl;
            return false;
        }
        if (debugMode)
        {
            std::cout << "[OK]" << std::endl;
        }
    }
    return true;
}

bool doTestSequence()
{
    hashesPassed = hashesFailed = hashesTested = 0;
    cemucore::keypad_reset();

    for (const auto& command : config.sequence)
    {
        if (debugMode)
        {
            std::cout << "Launching command " << command.first << " | " << command.second << std::endl;
        }
        if (!launchCommand(command))
        {
            return false;
        }
        cemucore::emu_run(config.delay_after_step);
    }
    return true;
}

} // namespace autotester
