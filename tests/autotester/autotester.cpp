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

namespace cemucore
{
    extern "C" {

#include "../../core/emu.h"
#include "../../core/link.h"

    bool throttleOn = true;
    auto lastTime = std::chrono::steady_clock::now();

    void gui_emu_sleep(void)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }

    void gui_do_stuff(void) { }

    void gui_set_busy(bool) { }

    void gui_console_vprintf(const char* fmt, va_list ap)
    {
        //vfprintf(stdout, fmt, ap);
        //fflush(stdout);
    }
    void gui_console_printf(const char* fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        gui_console_vprintf(fmt, ap);
        va_end(ap);
    }

    void throttle_timer_wait()
    {
        auto interval  = std::chrono::duration_cast<std::chrono::steady_clock::duration>
                            (std::chrono::duration<int, std::ratio<1, 60 * 1000000>>(1000000));
        auto cur_time  = std::chrono::steady_clock::now(),
             next_time = lastTime + interval;

        if (throttleOn && cur_time < next_time)
        {
            lastTime = next_time;
            std::this_thread::sleep_until(next_time);
        } else {
            lastTime = cur_time;
            std::this_thread::yield();
        }
    }

    }
}


struct hash_params_t {
    std::string description;
    uint32_t start;
    uint32_t size;
    std::vector<std::string> expected_CRCs;
};

struct config_t {
    std::string rom;
    std::vector<std::string> transfer_files;
    struct {
        std::string name;
        bool isASM;
    } target;
    std::vector<std::pair<std::string, std::string>> sequence;
    std::unordered_map<std::string, hash_params_t> hashes;
};

/* The global config variable */
config_t config;


/*
 * Constants usable in the "start" and "size" parameters of the JSON config for the hash params
 * See http://wikiti.brandonw.net/index.php?title=Category:84PCE:RAM:By_Address
 */
static const std::unordered_map<std::string, unsigned int> hash_consts = {
    { "vram_start",     0xD40000 }, { "vram_8_size",     320*240 },
    { "vram2_start",    0xD52C00 }, { "vram_16_size",  2*320*240 },
    { "ram_start",      0xD00000 }, { "ram_size",       0x040000 },
    { "textShadow",     0xD006C0 },
    { "cmdShadow",      0xD0232D },
    { "pixelShadow",    0xD031F6 },
    { "pixelShadow2",   0xD052C6 },
    { "cmdPixelShadow", 0xD07396 },
    { "plotSScreen",    0xD09466 },
    { "saveSScreen",    0xD0EA1F },
    { "UserMem",        0xD1A881 },
    { "CursorImage",    0xE30800 }
};

/* TODO */
struct coord2d { uint8_t y; uint8_t x; };
static const std::unordered_map<std::string, coord2d> valid_keys = {
    { "2nd",    { 3 , 4 } },
    { "alpha",  { 3 , 4 } },
    { "mode",   { 3 , 4 } },
    { "del",    { 3 , 4 } },
    { "clear",  { 3 , 4 } },
    { "enter",  { 3 , 4 } }
    /* ... */
};

// Some equates
#define CE_keyExtend  0xD0058E
#define CE_SendKPress 0x02015C
#define CE_JForceCmd  0x020164
// I'm not sure we need that one, actually
#define CE_JForceCmdNoChar 0x020160

void sendTokenKeyPress(uint8_t val, uint8_t ext, bool clearFirst)
{
    cemucore::emulationPaused = true;
    cemucore::mem_poke_byte(CE_keyExtend, ext);
    cemucore::cpu.registers.A = val;
    cemucore::cpu.registers.PC = clearFirst ? CE_JForceCmd : CE_SendKPress;
    cemucore::emulationPaused = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void sendLetterKeyPress(char letter)
{
    uint8_t val;
    if (letter != '@') { // @ is actually theta (replaced earlier)
        val = (uint8_t)(0x9A + letter - 'A');
    } else {
        val = 0xCC; // theta
    }
    sendTokenKeyPress(val, 0, false);
}

typedef std::function<void(const std::string&)> seq_cmd_func_t;
typedef std::function<void(void)> seq_cmd_action_func_t;

static const std::unordered_map<std::string, seq_cmd_action_func_t> valid_actions = {
    {
        "launch", [] {
            sendTokenKeyPress(0x09, 0, true); // Back to the Home Screen, then Clear.
            if (config.target.isASM) {
                sendTokenKeyPress(0xFC, 0x9C, false); // Insert Asm(
            }
            sendTokenKeyPress(0xDA, 0, false); // Insert prgm
            for (const char& c : config.target.name) {
                sendLetterKeyPress(c); // type program name
            }
            sendTokenKeyPress(0x05, 0, false); // Enter
        }
    },
    {
        "reset", [] {
            cemucore::cpuEvents |= EVENT_RESET;
        }
    },
    {
        "useClassic", [] {
            sendTokenKeyPress(0x09, 0, true);    // Back to the Home Screen, then Clear.
            sendTokenKeyPress(0xFB, 0xD3, true); // CLASSIC token
            sendTokenKeyPress(0x05, 0, false);   // Enter
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
            std::cerr << "[Error] 'hash' command not implemented yet " << std::endl;
        }
    },
    {
        "key", [](const std::string& which_key) {
            std::cerr << "[Error] 'key' command not implemented yet " << std::endl;
        }
    }
};


/****** Utility functions ******/

inline bool file_exists(const std::string& name)
{
    std::ifstream f(name.c_str());
    return f.good();
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

bool loadConfig(const json11::Json& configJson)
{
    json11::Json tmp, tmp2;

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
                            std::string crc_tmp = tmpHashCRC.string_value();
                            if (std::regex_match(crc_tmp, std::regex("^[0-9a-fA-F]+$"))) {
                                hash_param.expected_CRCs.push_back(crc_tmp);
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

    return true;
}

int main(int argc, char* argv[])
{
    // Used if the coreThread has been started (need to exit properly ; uses gotos)
    int retVal = 0;

    if (argc != 2)
    {
        std::cerr << "[Error] Needs one argument: path to the test config JSON file" << std::endl;
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
    json11::Json configJson = json11::Json::parse(jsonContents, jsonError);
    if (jsonError.empty())
    {
        std::cout << "[OK] JSON parsed" << std::endl;
    } else {
        std::cerr << "[Error] JSON parse error: " << jsonError << std::endl;
        return -1;
    }

    if (loadConfig(configJson))
    {
        std::cout << "[OK] Test config loaded and verified" << std::endl;
    } else {
        std::cerr << "[Error] -> See the test config file format and make sure values are correct." << std::endl;
        return -1;
    }

    /* Someone with better multithreading coding experience should probaly re-do this stuff correctly,
     * i.e. actually wait until the core is ready to do stuff, instead of blinding doing sleeps, etc.
     * Things like std::condition_variable should help, IIRC */
    std::thread coreThread;
    if (cemucore::emu_start(config.rom.c_str(), NULL))
    {
        coreThread = std::thread(&cemucore::emu_loop, true);
    } else {
        std::cerr << "[Error] Couldn't start emulation!" << std::endl;
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    /* Send files */
    for (const auto& file : config.transfer_files)
    {
        std::cout << "- Sending file " << file << "... " << std::endl;
        if (!cemucore::sendVariableLink(file.c_str()))
        {
            std::cerr << "[Error] File couldn't be sent" << std::endl;
            retVal = -1;
            goto cleanExit;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    /* Follow sequence */
    for (const auto& command : config.sequence)
    {
        std::cout << "Launching command " << command.first << " | " << command.second << std::endl;
        valid_seq_commands.at(command.first)(command.second);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

cleanExit:
    cemucore::exiting = true; // exit outer emu loop
    cemucore::cpu.next = 0; // exit inner emu loop

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    cemucore::emu_cleanup();

    coreThread.join();

    return retVal;
}