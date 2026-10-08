/*
 * Persistent headless CEmu controller
 *
 * This intentionally uses a line-oriented stdin/stdout protocol so callers can
 * keep one core instance alive without pulling in Qt or another IPC library.
 */

#include <cctype>
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
        #include "../../core/flash.h"
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
std::FILE *line_trace;
std::vector<uint32_t> line_trace_buffer;

void lineTraceHook(uint32_t line)
{
    line_trace_buffer.push_back(line);
    if (line_trace_buffer.size() == 65536) {
        std::fwrite(line_trace_buffer.data(), sizeof(uint32_t), line_trace_buffer.size(), line_trace);
        line_trace_buffer.clear();
    }
}

void lineTraceStop()
{
    if (line_trace) {
        std::fwrite(line_trace_buffer.data(), sizeof(uint32_t), line_trace_buffer.size(), line_trace);
        std::fclose(line_trace);
        line_trace = nullptr;
    }
    line_trace_buffer.clear();
    cemucore::flash_line_hook = nullptr;
}

struct options_t {
    std::string rom;
    std::string image;
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

bool parseAddress(const std::string& text, uint32_t& result)
{
    if (text.empty() || !std::isxdigit(static_cast<unsigned char>(text.front()))) {
        return false;
    }
    char *end = nullptr;
    errno = 0;
    const unsigned long value = std::strtoul(text.c_str(), &end, 16);
    if (errno || !end || *end || value > 0xFFFFFFu) {
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
              << " (--rom <file> | --image <file>)"
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
        (!options.image.empty() && !fileExists(options.image))) {
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
                "usb <VID:PID|bus#address|disconnect>; reset; status; "
                "peek <hex-address> [count]; poke <hex-address> <hex-bytes>; "
                "keydown <name>; keyup <name>; regs; stats; lcd-dma <0|1>; "
                "line-profile <on|off|save <path>>; line-trace <on <path>|off>; "
                "flash-cycles <n>; quit");
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

    if (command == "peek") {
        std::string address_text;
        std::string count_text;
        uint32_t address;
        uint32_t count = 1;
        if (!(input >> address_text) || !parseAddress(address_text, address) ||
            ((input >> count_text) && (!parseUnsigned(count_text, count) || count == 0 || count > 4096))) {
            respond("ERR usage: peek <hex-address> [count 1-4096]");
        } else {
            std::string bytes;
            char hex[3];
            for (uint32_t i = 0; i < count; ++i) {
                std::snprintf(hex, sizeof hex, "%02X", cemucore::mem_peek_byte((address + i) & 0xFFFFFFu));
                bytes += hex;
            }
            respond("OK peek " + bytes);
        }
        return true;
    }
    if (command == "poke") {
        std::string address_text;
        std::string bytes;
        uint32_t address;
        if (!(input >> address_text >> bytes) || !parseAddress(address_text, address) || bytes.size() % 2 ||
            bytes.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
            respond("ERR usage: poke <hex-address> <hex-bytes>");
        } else {
            for (size_t i = 0; i < bytes.size(); i += 2) {
                cemucore::mem_poke_byte((address + static_cast<uint32_t>(i / 2)) & 0xFFFFFFu,
                                        static_cast<uint8_t>(std::strtoul(bytes.substr(i, 2).c_str(), nullptr, 16)));
            }
            respond("OK poke " + std::to_string(bytes.size() / 2));
        }
        return true;
    }
    if (command == "keydown" || command == "keyup") {
        std::string name;
        autotester::key_coord_t coord{};
        input >> name;
        if (!autotester::keyCoordForName(name, coord)) {
            respond("ERR unknown key " + name);
        } else {
            cemucore::emu_keypad_event(coord.y, coord.x, command == "keydown");
            respond("OK " + command + " " + name);
        }
        return true;
    }
    if (command == "regs") {
        const auto& r = cemucore::cpu.registers;
        char text[192];
        std::snprintf(text, sizeof text,
                      "OK regs pc=%06X sp=%06X af=%04X bc=%06X de=%06X hl=%06X ix=%06X iy=%06X adl=%d halted=%d",
                      static_cast<unsigned>(r.PC), static_cast<unsigned>(r.SPL), static_cast<unsigned>(r.AF),
                      static_cast<unsigned>(r.BC), static_cast<unsigned>(r.DE), static_cast<unsigned>(r.HL),
                      static_cast<unsigned>(r.IX), static_cast<unsigned>(r.IY),
                      static_cast<int>(cemucore::cpu.ADL), static_cast<int>(cemucore::cpu.halted));
        respond(text);
        return true;
    }
    if (command == "stats") {
        char text[192];
        std::snprintf(text, sizeof text,
                      "OK stats cycles=%llu halted=%llu dma=%llu flash-reads=%lu flash-misses=%lu flash-delay=%lld",
                      static_cast<unsigned long long>(cemucore::cpu.baseCycles + cemucore::cpu.cycles),
                      static_cast<unsigned long long>(cemucore::cpu.haltCycles),
                      static_cast<unsigned long long>(cemucore::cpu.dmaCycles),
                      static_cast<unsigned long>(cemucore::cpu.flashTotalAccesses),
                      static_cast<unsigned long>(cemucore::cpu.flashCacheMisses),
                      static_cast<long long>(cemucore::cpu.flashDelayCycles));
        respond(text);
        return true;
    }
    if (command == "lcd-dma") {
        std::string value;
        if (!(input >> value) || (value != "0" && value != "1")) {
            respond("ERR usage: lcd-dma <0|1>");
        } else {
            cemucore::emu_set_lcd_dma(value == "1");
            respond("OK lcd-dma " + value);
        }
        return true;
    }

    if (command == "line-profile") {
        std::string what;
        input >> what;
        const size_t counters = 2 * FLASH_PROFILE_LINES;
        if (what == "on") {
            if (!cemucore::flash_line_profile) {
                cemucore::flash_line_profile = static_cast<uint32_t *>(std::calloc(counters, sizeof(uint32_t)));
            } else {
                std::memset(cemucore::flash_line_profile, 0, counters * sizeof(uint32_t));
            }
            respond(cemucore::flash_line_profile ? "OK line-profile on" : "ERR out of memory");
        } else if (what == "save" && cemucore::flash_line_profile) {
            std::string path;
            std::getline(input >> std::ws, path);
            std::FILE *file = path.empty() ? nullptr : std::fopen(path.c_str(), "wb");
            if (!file) {
                respond("ERR cannot write " + path);
            } else {
                const bool ok = std::fwrite(cemucore::flash_line_profile, sizeof(uint32_t), counters, file) == counters;
                std::fclose(file);
                respond(ok ? "OK line-profile save" : "ERR cannot write " + path);
            }
        } else if (what == "off") {
            std::free(cemucore::flash_line_profile);
            cemucore::flash_line_profile = nullptr;
            respond("OK line-profile off");
        } else {
            respond("ERR usage: line-profile <on|off|save <path>> (save needs on)");
        }
        return true;
    }
    if (command == "line-trace") {
        std::string what;
        input >> what;
        lineTraceStop();
        if (what == "on") {
            std::string path;
            std::getline(input >> std::ws, path);
            line_trace = path.empty() ? nullptr : std::fopen(path.c_str(), "wb");
            if (!line_trace) {
                respond("ERR cannot write " + path);
            } else {
                cemucore::flash_line_hook = lineTraceHook;
                respond("OK line-trace on");
            }
        } else if (what == "off") {
            respond("OK line-trace off");
        } else {
            respond("ERR usage: line-trace <on <path>|off>");
        }
        return true;
    }
    if (command == "flash-cycles") {
        std::string value;
        uint32_t cycles;
        if (!(input >> value) || !parseUnsigned(value, cycles) || cycles > 1000) {
            respond("ERR usage: flash-cycles <n> (0: the revision M cache)");
        } else {
            cemucore::flash_fixed_cycles = cycles;
            respond("OK flash-cycles " + value);
        }
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
    if (!cemucore::emu_set_run_rate(options.run_rate)) {
        std::cerr << "Failed to set run rate\n";
        return EXIT_FAILURE;
    }

    respond("CEMU_HEADLESS_READY width=" + std::to_string(LCD_WIDTH) +
            " height=" + std::to_string(LCD_HEIGHT) +
            " python=" + std::to_string(cemucore::get_asic_python()));

    std::string line;
    while (std::getline(std::cin, line) && runCommand(line)) {}

    lineTraceStop();
    cemucore::emu_exit();
    cemucore::asic_free();
    return EXIT_SUCCESS;
}
