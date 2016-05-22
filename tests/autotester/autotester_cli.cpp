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
#include <unordered_map>
#include <thread>
#include <chrono>
#include <regex>

#include "autotester.h"

/* As expected by the core */
extern "C"
{
    auto lastTime = std::chrono::steady_clock::now();

    void gui_emu_sleep(void) { std::this_thread::sleep_for(std::chrono::microseconds(50)); }
    void gui_do_stuff(void) { }
    void gui_set_busy(bool) { }
    void gui_console_printf(const char*, ...) { }
    void gui_entered_send_state(bool) { }

    void throttle_timer_wait()
    {
        auto interval  = std::chrono::duration_cast<std::chrono::steady_clock::duration>
                (std::chrono::duration<int, std::ratio<1, 60 * 1000000>>(800000)); // a bit faster than normal

        auto cur_time  = std::chrono::steady_clock::now(),
             next_time = lastTime + interval;

        if (cur_time < next_time)
        {
            lastTime = next_time;
            std::this_thread::sleep_until(next_time);
        } else {
            lastTime = cur_time;
            std::this_thread::yield();
        }
    }
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

    if (autotester::loadJSONConfig(jsonContents))
    {
        std::cout << "[OK] Test config loaded and verified. " << autotester::config.hashes.size() << " unique tests found." << std::endl;
    } else {
        std::cerr << "[Error] -> See the test config file format and make sure values are correct." << std::endl;
        return -1;
    }

    /* Someone with better multithreading coding experience should probaly re-do this stuff correctly,
     * i.e. actually wait until the core is ready to do stuff, instead of blinding doing sleeps, etc.
     * Things like std::condition_variable should help, IIRC */
    std::thread coreThread;
    if (cemucore::emu_start(autotester::config.rom.c_str(), NULL))
    {
        coreThread = std::thread(&cemucore::emu_loop, true);
    } else {
        std::cerr << "[Error] Couldn't start emulation!" << std::endl;
        return -1;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Clear home screen
    autotester::sendKey(0x09);

    // Send files
    if (!autotester::sendFilesForTest())
    {
        retVal = -1;
        goto cleanExit;
    }

    // Follow the sequence
    if (!autotester::doTestSequence())
    {
        retVal = -1;
        goto cleanExit;
        // This is useless here since cleanExit is right after,
        // but in case some other things are added in between at some point...
    }

cleanExit:
    cemucore::exiting = true; // exit outer emu loop
    cemucore::cpu.next = 0; // exit inner emu loop

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    cemucore::emu_cleanup();

    coreThread.join();

    // If no JSON/program/misc. error, return the hash failure count.
    if (retVal == 0)
    {
        std::cout << "\n*** Final results: out of " << autotester::hashesTested << " tests attempted, "
                  << autotester::hashesPassed << " passed, and " << autotester::hashesFailed << " failed. ***" << std::endl;
        return autotester::hashesFailed;
    }

    return retVal;
}
