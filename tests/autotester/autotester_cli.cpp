/*
 * Autotester CLI
 * (C) Adrien 'Adriweb' Bertrand
 * Part of the CEmu project
 * License: GPLv3
 */

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
  #include <direct.h>
  #define chdir _chdir
  #define getcwd _getcwd
#else
  #include <unistd.h>
#endif

#include "autotester.h"

/* As expected by the core */
namespace cemucore
{
    extern "C"
    {
        void gui_console_clear() {}
        void gui_debug_close(void) {}
        void gui_debug_open(int reason, uint32_t data) {
            printf("[CEmu debug open] reason=%d, data=0x%X\n", reason, data);
        }
        void gui_console_printf(const char *format, ...) { (void)format; }
        void gui_console_err_printf(const char *format, ...) {
            static char message[512];
            va_list ap;
            va_start(ap, format);
            if (vsnprintf(message, sizeof(message), format, ap) >= 0) {
                std::cerr << message;
            }
            va_end(ap);
        }
        asic_rev_t gui_handle_reset(const boot_ver_t* boot_ver, asic_rev_t loaded_rev, asic_rev_t default_rev, emu_device_t device, bool* python) {
            (void)boot_ver;
            (void)loaded_rev;
            (void)default_rev;
            (void)device;
            (void)python;
            return ASIC_REV_AUTO;
        }
    }
}

int main(int argc, char* argv[])
{
    // Used if the coreThread has been started (need to exit properly ; uses gotos)
    int retVal = 0;

    if (argc < 2)
    {
        std::cerr << "[Error] Needs a path argument, the test config JSON file" << std::endl;
        return -1;
    }

    if (strcmp(argv[1], "-d") == 0)
    {
        autotester::debugMode = true;
        argv++;
    } else {
        autotester::debugMode = false;
    }

    const std::string jsonPath(argv[1]);
    std::string jsonContents;
    std::ifstream ifs(jsonPath);
    if (ifs.good())
    {
        std::getline(ifs, jsonContents, '\0');
        if (!ifs.eof()) {
            std::cerr << "[Error] Couldn't read JSON file" << std::endl;
            return -1;
        }
    } else {
        std::cerr << "[Error] Couldn't open JSON file at provided path" << std::endl;
        return -1;
    }

    char* oldCWD = getcwd(nullptr, 0);
    if (oldCWD != nullptr && oldCWD[0] != '\0')
    {
        autotester::oldCWD = std::string{oldCWD} + '/';
    }

    // Go to the json file's dir to allow relative paths from there
    if (chdir(jsonPath.substr(0, jsonPath.find_last_of("/\\")).c_str())) {
        std::cerr << "[Error] Couldn't change directory path" << std::endl;
        return -1;
    }

    if (autotester::loadJSONConfig(jsonContents))
    {
        std::cout << jsonPath << " loaded and verified. " << autotester::config.hashes.size() << " unique tests found." << std::endl;
    } else {
        std::cerr << "[Error] See the test config file format and make sure values are correct" << std::endl;
        return -1;
    }

    if (cemucore::EMU_STATE_VALID != cemucore::emu_load(cemucore::EMU_DATA_ROM, autotester::config.rom.c_str()))
    {
        std::cerr << "[Error] Couldn't start emulation!" << std::endl;
        return -1;
    }

    cemucore::emu_set_run_rate(1000);
    cemucore::emu_run(10000);

    // Clear home screen
    autotester::sendKey(0x09);
    cemucore::emu_run(300);

    // Transfer things if needed
    if (!autotester::config.transfer_files.empty())
    {
        if (!autotester::sendFilesForTest())
        {
            std::cerr << "[Error] Error while in sendFilesForTest!" << std::endl;
            retVal = -1;
            goto cleanExit;
        }
    }

    cemucore::emu_run(500);

    // Follow the sequence
    if (!autotester::doTestSequence())
    {
        std::cerr << "[Error] Error while in doTestSequence!" << std::endl;
        retVal = -1;
        goto cleanExit;
        // This is useless here since cleanExit is right after,
        // but in case some other things are added in between at some point...
    }

cleanExit:
    if (oldCWD)
    {
        chdir(oldCWD);
        free(oldCWD);
    }
    cemucore::emu_exit();
    cemucore::asic_free();

    // If no JSON/program/misc. error, return the hash failure count.
    if (retVal == 0)
    {
        const char* status = autotester::hashesFailed == 0 ? "[Autotest passed]" : "[Autotest failed]";
        std::cout << status << " Out of " << autotester::hashesTested << " tests attempted, "
                  << autotester::hashesPassed << " passed, and " << autotester::hashesFailed << " failed.\n" << std::endl;

        return static_cast<int>(autotester::hashesFailed);
    }

    return retVal;
}
