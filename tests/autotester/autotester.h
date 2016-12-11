/*
 * Autotester header
 * (C) Adrien 'Adriweb' Bertrand
 * Part of the CEmu project
 * License: GPLv3
 */

#ifndef AUTOTESTER_H
#define AUTOTESTER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#include "../../core/defines.h"

namespace cemucore
{
    extern "C" {
        #include "../../core/emu.h"
        #include "../../core/link.h"
        #include "../../core/asic.h"
        #include "../../core/cpu.h"
        #include "../../core/mem.h"
        #include "../../core/keypad.h"
        #include "../../core/lcd.h"
        #include "../../core/extras.h"
    }
}

namespace autotester
{
    struct hash_params_t {
        std::string description;
        uint32_t start; /* Actually a pointer, for the CE */
        uint32_t size;
        int32_t timeout_ms;
        std::vector<uint32_t> expected_CRCs;
    };

    struct config_t {
        std::string rom;
        std::vector<std::string> transfer_files;
        unsigned int delay_after_key  =  0; /* delay in ms after each "key" action (after release) */
        unsigned int delay_after_step = 80; /* delay in ms after each sequence step */
        struct {
            std::string name;
            bool isASM;
        } target;
        std::vector<std::pair<std::string, std::string>> sequence;
        std::unordered_map<std::string, hash_params_t> hashes;
    };

    /*
     * Constants usable in the "start" and "size" parameters of the JSON config for the hash params
     * See http://wikiti.brandonw.net/index.php?title=Category:84PCE:RAM:By_Address
     */
    static const std::unordered_map<std::string, unsigned int> hash_consts = {
        { "vram_start",     0xD40000 },  { "vram_16_size",    2*LCD_SIZE },
        { "vram2_start",    0xD52C00 },  { "vram_8_size",       LCD_SIZE },
        { "ram_start",      0xD00000 },  { "ram_size",           0x40000 },
        { "textShadow",     0xD006C0 },  { "textShadow_size",        260 },
        { "cmdShadow",      0xD0232D },  { "cmdShadow_size",         260 },
        { "pixelShadow",    0xD031F6 },  { "pixelShadow_size",      8400 },
        { "pixelShadow2",   0xD052C6 },  { "pixelShadow2_size",     8400 },
        { "cmdPixelShadow", 0xD07396 },  { "cmdPixelShadow_size",   8400 },
        { "plotSScreen",    0xD09466 },  { "plotSScreen_size",     21945 },
        { "saveSScreen",    0xD0EA1F },  { "saveSScreen_size",     21945 },
        { "userMem",        0xD1A881 },
        { "lcdPalette",     0xE30200 },  { "lcdPalette_size",       512 },
        { "cursorImage",    0xE30800 },  { "cursorImage_size",     1024 }
    };

    void sendCSC(uint8_t csc);
    void sendKey(uint16_t key);
    void sendLetterKeyPress(char letter);

    extern const std::function<void(const std::string&)> pressKeyFromName;

    bool launchCommand(const std::pair<std::string, std::string>& command);

    bool loadJSONConfig(const std::string& jsonContents);

    std::vector<std::string> globVector(const std::string& pattern);

    bool sendFilesForTest();

    bool doTestSequence();

    /* The global config variable */
    extern config_t config;

    extern bool debugMode;
    extern bool ignoreROMfield;
    extern bool configLoaded;

    /* Will be incremented in case of matching CRC */
    extern unsigned int hashesPassed;
    /* Will be incremented in case of non-matching CRC, and used as the return value */
    extern unsigned int hashesFailed;
    /* Will be incremented at each `hash` command */
    extern unsigned int hashesTested;
}

#endif

