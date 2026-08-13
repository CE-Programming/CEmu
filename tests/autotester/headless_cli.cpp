/*
 * Persistent headless CEmu controller
 *
 * This intentionally uses a line-oriented stdin/stdout protocol so callers can
 * keep one core instance alive without pulling in Qt or another IPC library.
 */

#include <cerrno>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "autotester.h"

namespace cemucore
{
    extern "C"
    {
        #include "../../core/coproc.h"
        #include "../../core/usb/usb.h"

        void gui_console_clear() {}
        void gui_debug_close(void) {}
        void gui_debug_open(int reason, uint32_t data) {
            std::fprintf(stderr, "[CEmu debug open] reason=%d, data=0x%X\n", reason, data);
        }
        void gui_console_printf(const char *format, ...) {
            va_list ap;
            va_start(ap, format);
            std::vfprintf(stderr, format, ap);
            va_end(ap);
        }
        void gui_console_err_printf(const char *format, ...) {
            va_list ap;
            va_start(ap, format);
            std::vfprintf(stderr, format, ap);
            va_end(ap);
        }
        asic_rev_t gui_handle_reset(const boot_ver_t *, asic_rev_t loaded_rev,
                                    asic_rev_t, emu_device_t, bool *) {
            return loaded_rev;
        }
    }
}

namespace
{
struct options_t {
    std::string rom;
    std::string image;
    std::string arm_rom;
    uint32_t run_rate = 1000;
};

bool parseUnsigned(const std::string& text, uint32_t& result)
{
    if (text.empty()) {
        return false;
    }
    char *end = nullptr;
    errno = 0;
    const unsigned long value = std::strtoul(text.c_str(), &end, 10);
    if (errno || !end || *end || value > (std::numeric_limits<uint32_t>::max)()) {
        return false;
    }
    result = static_cast<uint32_t>(value);
    return true;
}

bool fileExists(const std::string& path)
{
    return !path.empty() && std::ifstream(path, std::ios::binary).good();
}

void put16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
{
    data[offset] = static_cast<uint8_t>(value);
    data[offset + 1] = static_cast<uint8_t>(value >> 8);
}

void put32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
{
    data[offset] = static_cast<uint8_t>(value);
    data[offset + 1] = static_cast<uint8_t>(value >> 8);
    data[offset + 2] = static_cast<uint8_t>(value >> 16);
    data[offset + 3] = static_cast<uint8_t>(value >> 24);
}

bool writeScreenshot(const std::string& path)
{
    const uint32_t row_size = (LCD_WIDTH * 3u + 3u) & ~3u;
    const uint32_t pixel_size = row_size * LCD_HEIGHT;
    std::vector<uint32_t> frame(LCD_SIZE);
    std::vector<uint8_t> bitmap(54u + pixel_size, 0);

    cemucore::emu_lcd_drawframe(frame.data());

    bitmap[0] = 'B';
    bitmap[1] = 'M';
    put32(bitmap, 2, static_cast<uint32_t>(bitmap.size()));
    put32(bitmap, 10, 54);
    put32(bitmap, 14, 40);
    put32(bitmap, 18, LCD_WIDTH);
    put32(bitmap, 22, LCD_HEIGHT);
    put16(bitmap, 26, 1);
    put16(bitmap, 28, 24);
    put32(bitmap, 34, pixel_size);

    for (uint32_t out_y = 0; out_y < LCD_HEIGHT; ++out_y) {
        const uint32_t in_y = LCD_HEIGHT - 1u - out_y;
        uint8_t *output = bitmap.data() + 54u + out_y * row_size;
        for (uint32_t x = 0; x < LCD_WIDTH; ++x) {
            const uint32_t pixel = frame[in_y * LCD_WIDTH + x];
            *output++ = static_cast<uint8_t>(pixel);
            *output++ = static_cast<uint8_t>(pixel >> 8);
            *output++ = static_cast<uint8_t>(pixel >> 16);
        }
    }

    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(bitmap.data()),
               static_cast<std::streamsize>(bitmap.size()));
    return file.good();
}

uint32_t screenHash()
{
    std::vector<uint32_t> frame(LCD_SIZE);
    cemucore::emu_lcd_drawframe(frame.data());
    uint32_t hash = UINT32_C(2166136261);
    for (const uint32_t pixel : frame) {
        for (unsigned int shift = 0; shift != 32; shift += 8) {
            hash ^= static_cast<uint8_t>(pixel >> shift);
            hash *= UINT32_C(16777619);
        }
    }
    return hash;
}

struct transfer_progress_t {
    bool finished = false;
    bool failed = false;
};

bool transferProgress(void *context, int value, int total)
{
    transfer_progress_t *progress = static_cast<transfer_progress_t *>(context);
    if (total <= 0) {
        progress->failed = true;
        progress->finished = true;
    } else if (value == total) {
        progress->finished = true;
    }
    return false;
}

bool sendFile(const std::string& path, int location)
{
    const char *file = path.c_str();
    transfer_progress_t progress;
    if (cemucore::emu_send_variables(&file, 1, location, transferProgress, &progress) !=
        cemucore::LINK_GOOD) {
        return false;
    }

    static const uint32_t timeout_ms = 60000;
    for (uint32_t elapsed = 0; !progress.finished && elapsed < timeout_ms; elapsed += 10) {
        cemucore::emu_run(10);
    }
    if (!progress.finished) {
        return false;
    }
    cemucore::emu_run(100);
    return !progress.failed;
}

void printUsage(const char *program)
{
    std::cerr << "Usage: " << program
              << " (--rom <file> | --image <file>) [--arm-rom <file>]"
                 " [--run-rate <ticks-per-second>]\n";
}

bool parseOptions(int argc, char **argv, options_t& options)
{
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            std::exit(EXIT_SUCCESS);
        }
        if (i + 1 >= argc) {
            std::cerr << "Missing value after " << arg << '\n';
            return false;
        }
        const std::string value(argv[++i]);
        if (arg == "--rom") {
            options.rom = value;
        } else if (arg == "--image") {
            options.image = value;
        } else if (arg == "--arm-rom") {
            options.arm_rom = value;
        } else if (arg == "--run-rate") {
            if (!parseUnsigned(value, options.run_rate) || !options.run_rate) {
                std::cerr << "Invalid run rate: " << value << '\n';
                return false;
            }
        } else {
            std::cerr << "Unknown option: " << arg << '\n';
            return false;
        }
    }

    if (options.rom.empty() == options.image.empty()) {
        std::cerr << "Specify exactly one of --rom or --image\n";
        return false;
    }
    if ((!options.rom.empty() && !fileExists(options.rom)) ||
        (!options.image.empty() && !fileExists(options.image)) ||
        (!options.arm_rom.empty() && !fileExists(options.arm_rom))) {
        std::cerr << "One or more input files do not exist\n";
        return false;
    }
    return true;
}

void respond(const std::string& message)
{
    std::cout << message << std::endl;
}

bool runCommand(const std::string& line)
{
    std::istringstream input(line);
    std::string command;
    input >> command;

    if (command.empty()) {
        return true;
    }
    if (command == "quit" || command == "exit") {
        respond("OK bye");
        return false;
    }
    if (command == "help") {
        respond("OK commands: run <ms>; run-realtime <ms>; "
                "key <name> [hold-ms]; keys <sequence>; "
                "screenshot <bmp-path>; screen-hash; save-state <path>; "
                "send-file [ram|archive|auto] <path>; "
                "usb <VID:PID|bus#address|disconnect>; reset; status; quit");
        return true;
    }
    if (command == "run") {
        std::string value;
        uint32_t milliseconds;
        if (!(input >> value) || !parseUnsigned(value, milliseconds)) {
            respond("ERR usage: run <ms>");
        } else {
            cemucore::emu_run(milliseconds);
            respond("OK run " + std::to_string(milliseconds));
        }
        return true;
    }
    if (command == "run-realtime") {
        std::string value;
        uint32_t milliseconds;
        if (!(input >> value) || !parseUnsigned(value, milliseconds)) {
            respond("ERR usage: run-realtime <ms>");
        } else {
            const auto start = std::chrono::steady_clock::now();
            for (uint32_t elapsed = 0; elapsed < milliseconds; ++elapsed) {
                cemucore::emu_run(1);
                std::this_thread::sleep_until(
                    start + std::chrono::milliseconds(elapsed + 1));
            }
            respond("OK run-realtime " + std::to_string(milliseconds));
        }
        return true;
    }
    if (command == "key") {
        std::string name;
        std::string hold_text;
        uint32_t hold_ms = 80;
        autotester::key_coord_t coord{};
        input >> name;
        if (input >> hold_text) {
            if (!parseUnsigned(hold_text, hold_ms)) {
                respond("ERR invalid hold duration");
                return true;
            }
        }
        if (!autotester::keyCoordForName(name, coord)) {
            respond("ERR unknown key " + name);
        } else {
            cemucore::emu_keypad_event(coord.y, coord.x, true);
            cemucore::emu_run(hold_ms);
            cemucore::emu_keypad_event(coord.y, coord.x, false);
            respond("OK key " + name);
        }
        return true;
    }
    if (command == "keys") {
        std::string sequence;
        std::getline(input >> std::ws, sequence);
        std::string error;
        autotester::key_sequence_handlers_t handlers;
        handlers.keyEvent = [](uint8_t row, uint8_t col, bool pressed) {
            cemucore::emu_keypad_event(row, col, pressed);
        };
        handlers.delay = [](unsigned int ms) { cemucore::emu_run(ms); };
        handlers.error = [&error](const std::string& value) { error = value; };
        if (sequence.empty() || !autotester::runKeySequence(sequence, handlers)) {
            respond("ERR " + (error.empty() ? std::string("invalid key sequence") : error));
        } else {
            respond("OK keys");
        }
        return true;
    }
    if (command == "screenshot" || command == "save-state") {
        std::string path;
        std::getline(input >> std::ws, path);
        if (path.empty()) {
            respond("ERR missing output path");
        } else if (command == "screenshot" ? writeScreenshot(path) :
                   cemucore::emu_save(cemucore::EMU_DATA_IMAGE, path.c_str())) {
            respond("OK " + command + " " + path);
        } else {
            respond("ERR failed to write " + path);
        }
        return true;
    }
    if (command == "screen-hash") {
        char hash[16];
        std::snprintf(hash, sizeof(hash), "%08X", screenHash());
        respond(std::string("OK screen-hash ") + hash);
        return true;
    }
    if (command == "send-file") {
        std::string arguments;
        std::string location_text;
        std::string path;
        std::getline(input >> std::ws, arguments);
        std::istringstream options(arguments);
        options >> location_text;
        int location = cemucore::LINK_FILE;
        bool explicit_location = true;
        if (location_text == "ram") {
            location = cemucore::LINK_RAM;
        } else if (location_text == "archive") {
            location = cemucore::LINK_ARCH;
        } else if (location_text == "auto") {
            location = cemucore::LINK_FILE;
        } else {
            explicit_location = false;
            path = arguments;
        }
        if (explicit_location) {
            std::getline(options >> std::ws, path);
        }
        if (arguments.empty() || path.empty()) {
            respond("ERR usage: send-file [ram|archive|auto] <path>");
            return true;
        }
        if (!fileExists(path)) {
            respond("ERR input file does not exist");
        } else if (sendFile(path, location)) {
            respond("OK send-file " + path);
        } else {
            respond("ERR failed to send " + path);
        }
        return true;
    }
    if (command == "usb") {
        std::string selector;
        std::getline(input >> std::ws, selector);
        if (selector.empty()) {
            respond("ERR usage: usb <VID:PID|bus#address|disconnect>");
            return true;
        }

        int error;
        if (selector == "disconnect") {
            error = cemucore::usb_plug_device(0, nullptr, nullptr, nullptr);
        } else {
            const char *arguments[] = { "physical", selector.c_str() };
            error = cemucore::usb_plug_device(2, arguments, nullptr, nullptr);
        }
        if (error) {
            respond("ERR usb " + selector + " code=" + std::to_string(error));
        } else {
            respond("OK usb " + selector);
        }
        return true;
    }
    if (command == "reset") {
        cemucore::emu_reset();
        respond("OK reset");
        return true;
    }
    if (command == "status") {
        respond("OK status device=" + std::to_string(cemucore::get_device_type()) +
                " revision=" + std::to_string(cemucore::get_asic_revision()) +
                " python=" + std::to_string(cemucore::get_asic_python()) +
                " run-rate=" + std::to_string(cemucore::emu_get_run_rate()));
        return true;
    }

    respond("ERR unknown command " + command);
    return true;
}
} // namespace

int main(int argc, char **argv)
{
    options_t options;
    if (!parseOptions(argc, argv, options)) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    const cemucore::emu_data_t type = options.image.empty()
        ? cemucore::EMU_DATA_ROM : cemucore::EMU_DATA_IMAGE;
    const std::string& path = options.image.empty() ? options.rom : options.image;
    if (cemucore::emu_load(type, path.c_str()) != cemucore::EMU_STATE_VALID) {
        std::cerr << "Failed to load " << path << '\n';
        return EXIT_FAILURE;
    }
    if (!options.arm_rom.empty() && !cemucore::coproc_load(options.arm_rom.c_str())) {
        std::cerr << "Failed to load ARM ROM " << options.arm_rom << '\n';
        return EXIT_FAILURE;
    }
    if (!cemucore::emu_set_run_rate(options.run_rate)) {
        std::cerr << "Failed to set run rate\n";
        return EXIT_FAILURE;
    }

    respond("CEMU_HEADLESS_READY width=" + std::to_string(LCD_WIDTH) +
            " height=" + std::to_string(LCD_HEIGHT) +
            " python=" + std::to_string(cemucore::get_asic_python()));

    std::string line;
    while (std::getline(std::cin, line) && runCommand(line)) {}

    cemucore::emu_exit();
    cemucore::asic_free();
    return EXIT_SUCCESS;
}
