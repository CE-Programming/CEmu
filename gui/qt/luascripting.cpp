#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSignalBlocker>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMessageBox>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "lcddebugwidget.h"
#include "sendinghandler.h"
#include "tibasicutils.h"
#include "utils.h"
#include "vartablemodel.h"

#include "../../core/asic.h"
#include "../../core/backlight.h"
#include "../../core/cpu.h"
#include "../../core/lcd.h"
#include "../../core/link.h"
#include "../../core/mem.h"
#include "../../core/panel.h"
#include "../../core/port.h"
#include "../../core/schedule.h"
#include "../../core/vat.h"
#include "../../core/debug/debug.h"

namespace {

constexpr auto LuaEventBootstrap = R"lua(
cemu = cemu or {}
cemu._handlers = {}
cemu._nextHandler = 0
cemu._eventSequence = 0
cemu._unloadHandlers = {}
cemu._nextUnloadHandler = 0
cemu._cleaningUp = false

function cemu.on(event, callback, options)
    assert(type(event) == "string", "event name must be a string")
    assert(type(callback) == "function", "event callback must be a function")
    assert(options == nil or type(options) == "table", "event options must be a table")
    cemu._nextHandler = cemu._nextHandler + 1
    local handlers = cemu._handlers[event]
    if not handlers then
        handlers = {}
        cemu._handlers[event] = handlers
    end
    handlers[cemu._nextHandler] = { callback = callback, options = options or {} }
    return cemu._nextHandler
end

function cemu.off(event, id)
    local handlers = cemu._handlers[event]
    if handlers then handlers[id] = nil end
end

function cemu.onUnload(callback)
    assert(type(callback) == "function", "unload callback must be a function")
    cemu._nextUnloadHandler = cemu._nextUnloadHandler + 1
    cemu._unloadHandlers[cemu._nextUnloadHandler] = callback
    return cemu._nextUnloadHandler
end

function cemu.offUnload(id)
    cemu._unloadHandlers[id] = nil
end

function cemu._cleanup(reason)
    if cemu._cleaningUp then return false end
    cemu._cleaningUp = true

    for _, handlers in pairs(cemu._handlers) do
        for id in pairs(handlers) do handlers[id] = nil end
    end
    cemu._handlers = {}
    if emu and emu.cancelAll then emu.cancelAll() end

    local ids = {}
    for id in pairs(cemu._unloadHandlers) do ids[#ids + 1] = id end
    table.sort(ids, function(left, right) return left > right end)
    local callbacks = cemu._unloadHandlers
    cemu._unloadHandlers = {}
    for _, id in ipairs(ids) do
        local ok, result = pcall(callbacks[id], { reason = reason or "cleanup" })
        if not ok then cErr("cleanup: " .. tostring(result)) end
    end

    cemu._cleaningUp = false
    return true
end

function cemu.cleanup(reason)
    return cemu._cleanup(reason or "manual")
end

function cemu._emit(event, payload)
    cemu._eventSequence = cemu._eventSequence + 1
    payload.sequence = cemu._eventSequence
    local handlers = cemu._handlers[event]
    if not handlers then return true end

    local ids = {}
    for id in pairs(handlers) do ids[#ids + 1] = id end
    table.sort(ids)

    local keep = true
    for _, id in ipairs(ids) do
        local handler = handlers[id]
        if handler then
            local options = handler.options
            local filters = options.where or options
            local matches = true
            for key, expected in pairs(filters) do
                if key ~= "once" and key ~= "predicate" and key ~= "where" and payload[key] ~= expected then
                    matches = false
                    break
                end
            end
            if matches and options.predicate then
                local predicateOk, predicateResult = pcall(options.predicate, payload)
                if not predicateOk then
                    cErr("event '" .. event .. "' predicate: " .. tostring(predicateResult))
                    matches = false
                else
                    matches = not not predicateResult
                end
            end
            if matches then
                if options.once then handlers[id] = nil end
                local ok, result = pcall(handler.callback, payload)
            if not ok then
                cErr("event '" .. event .. "': " .. tostring(result))
            elseif result == false then
                keep = false
            end
            end
        end
    end
    return keep
end
)lua";

QString scriptError(const sol::protected_function_result &result) {
    const sol::error error = result;
    return QString::fromStdString(error.what());
}

sol::table gammaTable(const sol::this_state &thisState, const panel_gamma_t &gamma) {
    sol::state_view lua(thisState);
    return lua.create_table_with(
        "v0", static_cast<unsigned int>(gamma.V0), "v1", static_cast<unsigned int>(gamma.V1),
        "v2", static_cast<unsigned int>(gamma.V2), "v4", static_cast<unsigned int>(gamma.V4),
        "v6", static_cast<unsigned int>(gamma.V6), "v13", static_cast<unsigned int>(gamma.V13),
        "v20", static_cast<unsigned int>(gamma.V20), "v27", static_cast<unsigned int>(gamma.V27),
        "v36", static_cast<unsigned int>(gamma.V36), "v43", static_cast<unsigned int>(gamma.V43),
        "v50", static_cast<unsigned int>(gamma.V50), "v57", static_cast<unsigned int>(gamma.V57),
        "v59", static_cast<unsigned int>(gamma.V59), "v61", static_cast<unsigned int>(gamma.V61),
        "v62", static_cast<unsigned int>(gamma.V62), "v63", static_cast<unsigned int>(gamma.V63),
        "j0", static_cast<unsigned int>(gamma.J0), "j1", static_cast<unsigned int>(gamma.J1));
}

sol::table stringListTable(const sol::this_state &thisState, const QStringList &values) {
    sol::state_view lua(thisState);
    sol::table result = lua.create_table(static_cast<int>(values.size()), 0);
    for (int index = 0; index < values.size(); ++index) {
        result[index + 1] = values[index].toStdString();
    }
    return result;
}

constexpr std::array<const char *, 16> PeripheralNames = {
    "control", "flash", "sha256", "usb", "lcd", "interrupts", "watchdog", "timers",
    "rtc", "protected", "keypad", "backlight", "misc", "spi", "uart", "reserved"
};

constexpr std::array<const char *, DBG_REG_COUNT> RegisterNames = {
    "a", "f", "b", "c", "d", "e", "h", "l", "ixh", "ixl", "iyh", "iyl",
    "a_", "f_", "b_", "c_", "d_", "e_", "h_", "l_", "af", "bc", "de", "hl",
    "ix", "iy", "af_", "bc_", "de_", "hl_", "sps", "spl", "pc", "i", "r", "mbase"
};

int registerId(std::string name) {
    std::ranges::transform(name, name.begin(), [](unsigned char c){ return std::tolower(c); });
    const auto found = std::ranges::find_if(RegisterNames, [&name](const char *r) { return name == r; });
    return found == RegisterNames.cend() ? -1 : static_cast<int>(found - RegisterNames.cbegin());
}

void validateMemoryRange(uint32_t address, uint32_t length) {
    constexpr uint32_t AddressSpaceSize = 0x1000000;
    if (address >= AddressSpaceSize || length > AddressSpaceSize - address) {
        throw sol::error("memory range extends past address 0xffffff");
    }
}

std::vector<uint8_t> luaByteVector(const sol::object &value) {
    if (value.is<std::string>()) {
        const std::string bytes = value.as<std::string>();
        return {bytes.begin(), bytes.end()};
    }
    if (!value.is<sol::table>()) throw sol::error("memory data must be a string or byte table");
    const sol::table table = value.as<sol::table>();
    std::vector<uint8_t> bytes;
    bytes.reserve(table.size());
    for (size_t index = 1; index <= table.size(); ++index) {
        const unsigned int byte = table.get<unsigned int>(index);
        if (byte > 0xFF) throw sol::error("memory byte table contains a value above 255");
        bytes.push_back(static_cast<uint8_t>(byte));
    }
    return bytes;
}

uint32_t memoryCrc32(uint32_t address, uint32_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t offset = 0; offset < length; ++offset) {
        crc ^= mem_peek_byte(address + offset);
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

std::string normalizedVariableTypeName(std::string name) {
    std::string normalized;
    for (const unsigned char character : name) {
        if (std::isalnum(character)) normalized.push_back(static_cast<char>(std::tolower(character)));
    }
    return normalized;
}

std::optional<calc_var_type_t> variableTypeFilter(const sol::optional<sol::object> &value) {
    if (!value || !value->valid() || value->get_type() == sol::type::lua_nil) return std::nullopt;
    if (value->is<unsigned int>()) {
        const unsigned int type = value->as<unsigned int>();
        if (type >= std::size(calc_var_type_names)) throw sol::error("variable type must be between 0 and 63");
        return static_cast<calc_var_type_t>(type);
    }
    if (!value->is<std::string>()) throw sol::error("variable type must be a numeric ID or type name");
    const std::string wanted = normalizedVariableTypeName(value->as<std::string>());
    for (size_t type = 0; type < std::size(calc_var_type_names); ++type) {
        if (wanted == normalizedVariableTypeName(calc_var_type_names[type])) {
            return static_cast<calc_var_type_t>(type);
        }
    }
    throw sol::error("unknown calculator variable type");
}

bool variableTypeMatches(const calc_var_t &var, const std::optional<calc_var_type_t> &filter) {
    return !filter || calc_var_normalized_type(var.type) == calc_var_normalized_type(*filter);
}

std::string variableName(const calc_var_t &var) {
    return calc_var_name_to_utf8(var.name, var.namelen, var.named);
}

bool findVariable(const std::string &name, const std::optional<calc_var_type_t> &filter, calc_var_t &result) {
    vat_search_init(&result);
    while (vat_search_next(&result)) {
        if (variableTypeMatches(result, filter) && variableName(result) == name) return true;
    }
    return false;
}

sol::table variableInfo(const sol::this_state &thisState, const calc_var_t &var) {
    sol::state_view lua(thisState);
    const calc_var_type_t normalizedType = calc_var_normalized_type(var.type);
    return lua.create_table_with(
        "name", variableName(var),
        "rawName", std::string(reinterpret_cast<const char *>(var.name), var.namelen),
        "type", calc_var_type_names[var.type],
        "typeId", static_cast<unsigned int>(var.type),
        "normalizedType", calc_var_type_names[normalizedType],
        "normalizedTypeId", static_cast<unsigned int>(normalizedType),
        "size", var.size,
        "address", var.address,
        "vatAddress", var.originalVat,
        "version", var.version,
        "archived", var.archived,
        "named", var.named,
        "tokenized", calc_var_is_tokenized(&var),
        "python", calc_var_is_python_appvar(&var),
        "internal", calc_var_is_internal(&var));
}

} // namespace

void MainWindow::initLuaThings(sol::state &lua, bool isREPL) {
    if (isREPL ? m_replLuaInitialized : m_edLuaInitialized) runLuaCleanup(lua, "reset");
    clearLuaTimers(&lua);
    lua = sol::state{};

    lua.set_panic([](lua_State *state) {
        const char *message = lua_tostring(state, -1);
        fprintf(stderr, "[Lua Panic] %s\n", message ? message : "unknown panic");
        return -1;
    });

    if (m_luaUnsafe) {
        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::coroutine,
                           sol::lib::io, sol::lib::os, sol::lib::math,
                           sol::lib::string, sol::lib::table, sol::lib::debug,
                           sol::lib::utf8);
    } else {
        lua.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::math,
                           sol::lib::string, sol::lib::table, sol::lib::utf8);
        lua.script("require, loadfile, dofile, package, io, os, debug = nil, nil, nil, nil, nil, nil, nil");
    }

    const auto appendValues = [this, isREPL](const sol::this_state &thisState, bool error) {
        lua_State *state = thisState;
        const int count = lua_gettop(state);
        QStringList values;
        values.reserve(count);
        for (int index = 1; index <= count; ++index) {
            size_t length = 0;
            const char *value = luaL_tolstring(state, index, &length);
            values.append(QString::fromUtf8(value, static_cast<qsizetype>(length)));
            lua_pop(state, 1);
        }
        const QString text = values.join(QLatin1Char('\t'));
        if (isREPL) {
            ui->REPLConsole->appendPlainText((error ? QStringLiteral("[Error] ") : QString()) + text);
        } else {
            console(QStringLiteral("[Lua] ") + text + QLatin1Char('\n'),
                    error ? EmuThread::ConsoleErr : EmuThread::ConsoleNorm);
        }
    };
    lua.set_function("cLog", [appendValues](const sol::this_state &state) { appendValues(state, false); });
    lua.set_function("cErr", [appendValues](const sol::this_state &state) { appendValues(state, true); });
    lua.script("print = function(...) local values={...}; for i=1,select('#', ...) do values[i]=tostring(values[i]) end; cLog(table.unpack(values, 1, select('#', ...))) end");

    lua["cpu"] = std::ref(cpu);

    lua.new_usertype<decltype(cpu.registers.flags)>("eZ80flags_t",
#define FLAG(f) (#f), sol::property([](decltype(cpu.registers.flags) &flags) { return static_cast<bool>(flags.f); }, \
                                    [](decltype(cpu.registers.flags) &flags, bool value) { flags.f = value; })
        FLAG(C), FLAG(N), FLAG(PV), FLAG(_3), FLAG(H), FLAG(_5), FLAG(Z), FLAG(S)
#undef FLAG
    );

    lua.new_usertype<eZ80registers_t>("eZ80registers_t",
        "flags", sol::readonly(&eZ80registers_t::flags),
#define RP(reg) (#reg), (&eZ80registers_t::reg)
        RP(AF), RP(F), RP(A), RP(BC), RP(BCS), RP(C), RP(B), RP(BCU), RP(DE),
        RP(DES), RP(E), RP(D), RP(DEU), RP(HL), RP(HLS), RP(L), RP(H), RP(HLU), RP(_HL),
        RP(IX), RP(IXS), RP(IXL), RP(IXH), RP(IXU), RP(IY), RP(IYS), RP(IYL), RP(IYH), RP(IYU),
        RP(_AF), RP(_BC), RP(_DE), RP(SPS), RP(SPL), RP(PC), RP(PCS), RP(PCL), RP(PCH),
        RP(PCU), RP(I), RP(R), RP(MBASE)
#undef RP
    );

    lua.new_usertype<eZ80cpu_t>("eZ80cpu_t",
        "registers", sol::readonly(&eZ80cpu_t::registers),
#define CPU_FLAG(f) (#f), sol::property([](eZ80cpu_t &value) { return static_cast<bool>(value.f); }, \
                                        [](eZ80cpu_t &value, bool enabled) { value.f = enabled; })
        CPU_FLAG(halted), CPU_FLAG(ADL), CPU_FLAG(MADL), CPU_FLAG(IEF1), CPU_FLAG(IEF2),
#undef CPU_FLAG
        "inBlock", sol::readonly_property([](const eZ80cpu_t &value) { return static_cast<bool>(value.inBlock); }),
        "cycles", sol::readonly(&eZ80cpu_t::cycles),
        "next", sol::readonly(&eZ80cpu_t::next),
        "prefetch", sol::readonly(&eZ80cpu_t::prefetch)
    );

    sol::table memoryTable = lua.create_named_table("mem",
        "readByte", mem_peek_byte,
        "readShort", mem_peek_short,
        "readLong", mem_peek_long,
        "readWord", mem_peek_word,
        "writeByte", mem_poke_byte,
        "writeShort", mem_poke_short,
        "writeLong", mem_poke_long,
        "writeWord", mem_poke_word
    );
    memoryTable.set_function("read", [](uint32_t address, uint32_t length) {
        validateMemoryRange(address, length);
        std::string bytes(length, '\0');
        for (uint32_t offset = 0; offset < length; ++offset) {
            bytes[offset] = static_cast<char>(mem_peek_byte(address + offset));
        }
        return bytes;
    });
    memoryTable.set_function("readTable", [](const sol::this_state &thisState,
                                                uint32_t address, uint32_t length) {
        validateMemoryRange(address, length);
        sol::state_view view(thisState);
        sol::table bytes = view.create_table(static_cast<int>(length), 0);
        for (uint32_t offset = 0; offset < length; ++offset) {
            bytes[offset + 1] = mem_peek_byte(address + offset);
        }
        return bytes;
    });
    memoryTable.set_function("write", [](uint32_t address, const sol::object &data) {
        const std::vector<uint8_t> bytes = luaByteVector(data);
        validateMemoryRange(address, static_cast<uint32_t>(bytes.size()));
        for (size_t offset = 0; offset < bytes.size(); ++offset) mem_poke_byte(address + offset, bytes[offset]);
        return bytes.size();
    });
    memoryTable.set_function("fill", [](uint32_t address, uint32_t length, uint8_t value) {
        validateMemoryRange(address, length);
        for (uint32_t offset = 0; offset < length; ++offset) mem_poke_byte(address + offset, value);
    });
    memoryTable.set_function("copy", [](uint32_t destination, uint32_t source, uint32_t length) {
        validateMemoryRange(source, length);
        validateMemoryRange(destination, length);
        std::vector<uint8_t> bytes(length);
        for (uint32_t offset = 0; offset < length; ++offset) bytes[offset] = mem_peek_byte(source + offset);
        for (uint32_t offset = 0; offset < length; ++offset) mem_poke_byte(destination + offset, bytes[offset]);
    });
    memoryTable.set_function("crc32", [](uint32_t address, uint32_t length) {
        validateMemoryRange(address, length);
        return memoryCrc32(address, length);
    });
    memoryTable.set_function("search", [](const sol::this_state &thisState, uint32_t address,
                                              uint32_t length, const sol::object &patternValue,
                                              sol::optional<unsigned int> requestedLimit) {
        validateMemoryRange(address, length);
        const std::vector<uint8_t> pattern = luaByteVector(patternValue);
        if (pattern.empty()) throw sol::error("memory search pattern must not be empty");
        const unsigned int limit = requestedLimit.value_or(1024);
        sol::state_view view(thisState);
        sol::table matches = view.create_table();
        if (pattern.size() > length || limit == 0) return matches;
        unsigned int count = 0;
        const uint32_t lastOffset = length - static_cast<uint32_t>(pattern.size());
        for (uint32_t offset = 0; offset <= lastOffset && count < limit; ++offset) {
            bool equal = true;
            for (size_t index = 0; index < pattern.size(); ++index) {
                if (mem_peek_byte(address + offset + index) != pattern[index]) {
                    equal = false;
                    break;
                }
            }
            if (equal) matches[++count] = address + offset;
        }
        return matches;
    });

    sol::table variableTable = lua.create_named_table("vars");
    variableTable.set_function("list", [](const sol::this_state &thisState,
                                           sol::optional<sol::object> requestedType) {
        const std::optional<calc_var_type_t> filter = variableTypeFilter(requestedType);
        sol::state_view view(thisState);
        sol::table variables = view.create_table();
        calc_var_t var;
        vat_search_init(&var);
        unsigned int index = 0;
        while (vat_search_next(&var)) {
            if (variableTypeMatches(var, filter)) variables[++index] = variableInfo(thisState, var);
        }
        return variables;
    });
    variableTable.set_function("find", [](const sol::this_state &thisState, const std::string &name,
                                           sol::optional<sol::object> requestedType) {
        calc_var_t var;
        if (!findVariable(name, variableTypeFilter(requestedType), var)) return sol::make_object(thisState, sol::lua_nil);
        return sol::make_object(thisState, variableInfo(thisState, var));
    });
    variableTable.set_function("read", [](const sol::this_state &thisState, const std::string &name,
                                           sol::optional<sol::object> requestedType) {
        calc_var_t var;
        if (!findVariable(name, variableTypeFilter(requestedType), var)) return sol::make_object(thisState, sol::lua_nil);
        return sol::make_object(thisState,
            std::string(reinterpret_cast<const char *>(var.data), var.size));
    });
    variableTable.set_function("launch", [this](const std::string &name,
                                                  sol::optional<sol::object> requestedType) {
        calc_var_t var;
        if (!findVariable(name, variableTypeFilter(requestedType), var)) throw sol::error("calculator variable not found");
        if (!calc_var_is_prog(&var) || calc_var_is_internal(&var)) {
            throw sol::error("calculator variable is not a launchable program");
        }
        varLaunch(&var);
    });
    variableTable.set_function("types", [](const sol::this_state &thisState) {
        sol::state_view view(thisState);
        sol::table types = view.create_table(64, 0);
        for (unsigned int type = 0; type < std::size(calc_var_type_names); ++type) {
            types[type + 1] = view.create_table_with("id", type, "name", calc_var_type_names[type]);
        }
        return types;
    });

    sol::table peripherals = lua.create_named_table("peripherals");
    peripherals.set_function("peek", [](uint16_t address) { return port_peek_byte(address); });
    peripherals.set_function("poke", [](uint16_t address, uint8_t value) { port_poke_byte(address, value); });
    peripherals.set_function("read", [](uint16_t address) { return port_read_byte(address); });
    peripherals.set_function("write", [](uint16_t address, uint8_t value) { port_write_byte(address, value); });
    peripherals.set_function("describe", [](const sol::this_state &thisState, uint16_t address) {
        const unsigned int range = address >> 12;
        sol::state_view view(thisState);
        return view.create_table_with(
            "name", PeripheralNames[range], "range", range,
            "base", range << 12, "offset", address & 0xFFF, "address", address);
    });
    peripherals.set_function("snapshot", [](const sol::this_state &thisState, uint16_t address,
                                               sol::optional<unsigned int> requestedLength) {
        const unsigned int length = requestedLength.value_or(1);
        if (length > 0x10000u - address) {
            throw sol::error("peripheral snapshot extends past port 0xffff");
        }
        sol::state_view view(thisState);
        sol::table result = view.create_table(static_cast<int>(length), 0);
        for (unsigned int offset = 0; offset < length; ++offset) {
            result[offset + 1] = port_peek_byte(static_cast<uint16_t>(address + offset));
        }
        return result;
    });
    peripherals.set_function("monitor", [](uint16_t address, sol::optional<bool> read,
                                              sol::optional<bool> write, sol::optional<bool> freeze) {
        debug_ports(address, DBG_MASK_PORT_READ, read.value_or(false));
        debug_ports(address, DBG_MASK_PORT_WRITE, write.value_or(false));
        debug_ports(address, DBG_MASK_PORT_FREEZE, freeze.value_or(false));
        return debug.port[address] & (DBG_MASK_PORT_READ | DBG_MASK_PORT_WRITE | DBG_MASK_PORT_FREEZE);
    });
    peripherals.set_function("monitorState", [](const sol::this_state &thisState, uint16_t address) {
        const int mask = debug.port[address];
        sol::state_view view(thisState);
        return view.create_table_with(
            "read", static_cast<bool>(mask & DBG_MASK_PORT_READ),
            "write", static_cast<bool>(mask & DBG_MASK_PORT_WRITE),
            "freeze", static_cast<bool>(mask & DBG_MASK_PORT_FREEZE));
    });
    sol::table peripheralRanges = lua.create_table();
    for (size_t index = 0; index < PeripheralNames.size(); ++index) {
        peripheralRanges[PeripheralNames[index]] = lua.create_table_with(
            "base", static_cast<unsigned int>(index << 12), "size", 0x1000,
            "last", static_cast<unsigned int>((index << 12) | 0xFFF));
    }
    peripherals["ranges"] = peripheralRanges;

    const auto lcdControllerState = [](const sol::this_state &thisState) {
        sol::state_view view(thisState);
        sol::table timings = view.create_table(4, 0);
        for (int index = 0; index < 4; ++index) timings[index + 1] = lcd.timing[index];
        return view.create_table_with(
            "timing", timings, "control", lcd.control, "interruptMask", lcd.imsc,
            "rawInterruptStatus", lcd.ris, "upperBase", lcd.upbase, "lowerBase", lcd.lpbase,
            "upperCurrent", lcd.upcurr, "lowerCurrent", lcd.lpcurr,
            "cursorControl", lcd.crsrControl, "cursorConfig", lcd.crsrConfig,
            "pixelsPerLine", lcd.PPL, "horizontalSync", lcd.HSW,
            "horizontalFrontPorch", lcd.HFP, "horizontalBackPorch", lcd.HBP,
            "linesPerPanel", lcd.LPP, "verticalSync", lcd.VSW,
            "verticalFrontPorch", lcd.VFP, "verticalBackPorch", lcd.VBP,
            "pixelClockDivider", lcd.PCD, "clocksPerLine", lcd.CPL,
            "currentRow", lcd.curRow, "currentColumn", lcd.curCol,
            "phase", static_cast<int>(lcd.compare), "dma", static_cast<bool>(lcd.useDma));
    };
    const auto lcdPanelState = [](const sol::this_state &thisState) {
        sol::state_view view(thisState);
        panel_timing_t timing;
        panel_get_timing(&timing);
        sol::table positive = gammaTable(thisState, panel.params.PVGAMCTRL);
        sol::table negative = gammaTable(thisState, panel.params.NVGAMCTRL);
        return view.create_table_with(
            "command", panel.cmd,
            "commandParameter", panel.paramIter,
            "displayMode", panel.displayMode,
            "modeFlags", panel.mode,
            "pendingModeFlags", panel.pendingMode,
            "inverted", panel.invert,
            "tearing", panel.tear,
            "row", panel.row,
            "column", panel.col,
            "sourceRow", panel.srcRow,
            "destinationRow", panel.dstRow,
            "columnStart", panel.params.CASET.XS,
            "columnEnd", panel.params.CASET.XE,
            "rowStart", panel.params.RASET.YS,
            "rowEnd", panel.params.RASET.YE,
            "scrollTop", panel.params.VSCRDEF.TFA,
            "scrollArea", panel.params.VSCRDEF.VSA,
            "scrollBottom", panel.params.VSCRDEF.BFA,
            "scrollStart", panel.params.VSCRSADD.VSP,
            "madctlMx", static_cast<bool>(panel.params.MADCTL.MX),
            "madctlMy", static_cast<bool>(panel.params.MADCTL.MY),
            "madctlMv", static_cast<bool>(panel.params.MADCTL.MV),
            "madctlMl", static_cast<bool>(panel.params.MADCTL.ML),
            "madctlMh", static_cast<bool>(panel.params.MADCTL.MH),
            "madctlRgb", static_cast<bool>(panel.params.MADCTL.RGB),
            "mcuPixelFormat", static_cast<unsigned int>(panel.params.COLMOD.MCU),
            "rgbPixelFormat", static_cast<unsigned int>(panel.params.COLMOD.RGB),
            "horizontalBackPorch", timing.horizBackPorch,
            "horizontalActive", timing.horizActive,
            "horizontalFrontPorch", timing.horizFrontPorch,
            "verticalBackPorch", timing.vertBackPorch,
            "verticalActive", timing.vertActive,
            "verticalFrontPorch", timing.vertFrontPorch,
            "clockRate", panel.clockRate,
            "clockDivider", static_cast<unsigned int>(panel.params.FRCTRL1.DIV),
            "gammaPreset", static_cast<unsigned int>(panel.params.GAMSET.GC),
            "accurateGamma", panel.accurateGamma,
            "positiveGamma", positive,
            "negativeGamma", negative);
    };
    sol::table lcdTable = lua.create_named_table("lcd");
    lcdTable.set_function("controllerState", lcdControllerState);
    lcdTable.set_function("panelState", lcdPanelState);
    lcdTable.set_function("state", [lcdControllerState, lcdPanelState](const sol::this_state &thisState) {
        sol::state_view view(thisState);
        return view.create_table_with("controller", lcdControllerState(thisState),
                                      "panel", lcdPanelState(thisState),
                                      "backlight", view.create_table_with(
                                          "brightness", backlight.brightness,
                                          "factor", backlight.factor)
                                      );
    });
    lcdTable.set_function("panelCommand", [](uint8_t command, const sol::table &parameters) {
        if (!guiDebug) throw sol::error("lcd.panelCommand requires paused emulation");
        const size_t size = parameters.size();
        std::vector<uint8_t> bytes;
        bytes.reserve(size);
        for (size_t index = 1; index <= size; ++index) {
            const unsigned int value = parameters.get<unsigned int>(index);
            if (value > 0xFF) throw sol::error("LCD panel command parameter must be a byte");
            bytes.push_back(static_cast<uint8_t>(value));
        }
        return panel_debug_write_command(command, bytes.data(), bytes.size());
    });
    lcdTable.set_function("refreshDebugPane", [this] { m_lcdDebug->populate(); });
    lcdTable.set_function("applyDebugPane", [this] {
        if (!guiDebug) throw sol::error("lcd.applyDebugPane requires paused emulation");
        m_lcdDebug->sync();
        lcd_update();
    });
    lcdTable.set_function("showDebugPane", [this] {
        ui->tabDebug->setCurrentWidget(m_lcdDebug);
        show();
        raise();
        activateWindow();
    });
    lcdTable.set_function("setDma", [this](bool enabled) { setLcdDma(enabled); });
    lcdTable.set_function("setGamma", [this](bool enabled) { setLcdGamma(enabled); });
    lcdTable.set_function("setResponse", [this](bool enabled) { setLcdResponse(enabled); });
    lcdTable.set_function("setScale", [this](int percent) { setLcdScale(std::clamp(percent, 10, 500)); });
    lcdTable.set_function("setUpscale", [this](int mode) { setLcdUpscale(std::clamp(mode, 0, 2)); });
    lcdTable.set_function("setSkin", [this](bool enabled) { setSkinToggle(enabled); });

    lua.create_named_table("keys",
        "press", [this](const std::string &key) { sendEmuKeySequence(QString::fromStdString(key)); },
        "sequence", [this](const std::string &sequence) { sendEmuKeySequence(QString::fromStdString(sequence)); },
        "down", [this](const std::string &key) { sendEmuKeySequence(QStringLiteral("down:") + QString::fromStdString(key)); },
        "up", [this](const std::string &key) { sendEmuKeySequence(QStringLiteral("up:") + QString::fromStdString(key)); },
        "hold", [this](const std::string &key, unsigned int milliseconds) {
            sendEmuKeySequence(QStringLiteral("hold:%1:%2").arg(QString::fromStdString(key)).arg(milliseconds));
        }
    );

    lua.create_named_table("gui",
        "screenshot", sol::overload(
            [this]() { return ui->lcd->getImage().save(
                QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) +
                QStringLiteral("/CEmu_screenshot_") + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss")) +
                QStringLiteral(".png"), "PNG", 0); },
            [this](const std::string &path) { return ui->lcd->getImage().save(QString::fromStdString(path), "PNG", 0); }),
        "refresh", [] { QApplication::processEvents(); },
        "messageBox", [this](const std::string &title, const std::string &message) {
            QMessageBox::information(this, QString::fromStdString(title), QString::fromStdString(message));
        },
        "status", [this](const std::string &message) { showStatusMsg(QString::fromStdString(message)); },
        "setKeypadColor", [this](unsigned int color) { setKeypadColor(color); },
        "setFullscreen", [this](int mode) { setFullscreen(std::clamp(mode, 0, 2)); },
        "openScriptsFolder", [this] { QDesktopServices::openUrl(QUrl::fromLocalFile(luaScriptsPath())); },
        "quit", [] { QTimer::singleShot(0, qApp, &QCoreApplication::quit); }
    );

    sol::table emuTable = lua.create_named_table("emu",
        "reset", [this] { resetEmu(); },
        "reloadROM", [this] { emuLoad(EMU_DATA_ROM); },
        "throttle", [this](bool enabled) { setThrottle(enabled ? Qt::Checked : Qt::Unchecked); },
        "setSpeed", [this](int speed) { setEmuSpeed(std::clamp(speed, 0, 500)); },
        "wait", [](unsigned int milliseconds) { guiDelay(static_cast<int>(milliseconds)); },
        "saveState", [this](const std::string &path) { stateToPath(QString::fromStdString(path)); },
        "loadState", [this](const std::string &path) { stateFromPath(QString::fromStdString(path)); },
        "sendFile", [](const std::string &path) {
            sendingHandler->sendFiles({QString::fromStdString(path)}, LINK_FILE);
        },
        "deviceType", [] { return static_cast<int>(get_device_type()); }
    );
    const auto millisecondsToTicks = [](double milliseconds, bool allowZero) {
        if (!std::isfinite(milliseconds) || milliseconds < 0.0 || (!allowZero && milliseconds == 0.0)) {
            throw sol::error(allowZero ? "delay must be a finite non-negative number"
                                       : "interval must be a finite positive number");
        }
        constexpr double TicksPerMillisecond = 48000.0;
        if (milliseconds > static_cast<double>((std::numeric_limits<uint64_t>::max)()) / TicksPerMillisecond) {
            throw sol::error("timer delay is too large");
        }
        return static_cast<uint64_t>(std::ceil(milliseconds * TicksPerMillisecond));
    };
    emuTable.set_function("time", [] {
        return static_cast<double>(sched_total_time(CLOCK_48M)) / 48000.0;
    });
    emuTable.set_function("cycles", [] { return sched_total_cycles(); });
    emuTable.set_function("after", [this, &lua, millisecondsToTicks](double milliseconds,
                                                                     sol::protected_function callback) {
        return addLuaTimer(lua, std::move(callback), millisecondsToTicks(milliseconds, true), 0, false);
    });
    emuTable.set_function("afterCycles", [this, &lua](uint64_t cycles, sol::protected_function callback) {
        return addLuaTimer(lua, std::move(callback), cycles, 0, true);
    });
    emuTable.set_function("every", [this, &lua, millisecondsToTicks](double milliseconds,
                                                                     sol::protected_function callback) {
        const uint64_t interval = millisecondsToTicks(milliseconds, false);
        return addLuaTimer(lua, std::move(callback), interval, interval, false);
    });
    emuTable.set_function("everyCycles", [this, &lua](uint64_t cycles, sol::protected_function callback) {
        if (cycles == 0) throw sol::error("cycle interval must be positive");
        return addLuaTimer(lua, std::move(callback), cycles, cycles, true);
    });
    emuTable.set_function("cancel", [this](uint64_t id) { return cancelLuaTimer(id); });
    emuTable.set_function("cancelAll", [this, &lua] { clearLuaTimers(&lua); });

    sol::table debugTable = lua.create_named_table("dbg");
    debugTable.set_function("stop", [this] { if (!guiDebug) debugToggle(); });
    debugTable.set_function("resume", [this] { if (guiDebug) debugToggle(); });
    debugTable.set_function("stepIn", [this] { stepIn(); });
    debugTable.set_function("stepOver", [this] { stepOver(); });
    debugTable.set_function("stepNext", [this] { stepNext(); });
    debugTable.set_function("stepOut", [this] { stepOut(); });
    debugTable.set_function("stepUntilReturn", [this] { stepUntilRet(); });
    debugTable.set_function("addBreakpoint", sol::overload(
        [this](uint32_t address) { return breakAdd(QStringLiteral("Lua"), address, true, false, false); },
        [this](uint32_t address, const std::string &label) {
            return breakAdd(QString::fromStdString(label), address, true, false, false);
        }));
    debugTable.set_function("removeBreakpoint", [this](uint32_t address) { breakRemove(address); });
    debugTable.set_function("addWatchpoint", sol::overload(
        [this](uint32_t low, uint32_t high, bool read, bool write) {
            const int mask = (read ? DBG_MASK_READ : 0) | (write ? DBG_MASK_WRITE : 0);
            return watchAdd(QStringLiteral("Lua"), low, high, mask, false, false);
        },
        [this](uint32_t low, uint32_t high, bool read, bool write, const std::string &label) {
            const int mask = (read ? DBG_MASK_READ : 0) | (write ? DBG_MASK_WRITE : 0);
            return watchAdd(QString::fromStdString(label), low, high, mask, false, false);
        }));
    debugTable.set_function("removeWatchpoint", [this](uint32_t address) { watchRemove(address); });
    debugTable.set_function("watchRegister", [](const std::string &name, bool read, bool write) {
        const int id = registerId(name);
        if (id < 0) throw sol::error("unknown CPU register name");
        debug_reg_watch(static_cast<unsigned int>(id), DBG_MASK_READ, read);
        debug_reg_watch(static_cast<unsigned int>(id), DBG_MASK_WRITE, write);
    });
    debugTable.set_function("registerWatchState", [](const sol::this_state &thisState, const std::string &name) {
        const int id = registerId(name);
        if (id < 0) throw sol::error("unknown CPU register name");
        const int mask = debug_reg_get_mask(static_cast<unsigned int>(id));
        sol::state_view view(thisState);
        return view.create_table_with("read", static_cast<bool>(mask & DBG_MASK_READ),
                                      "write", static_cast<bool>(mask & DBG_MASK_WRITE));
    });
    debugTable.set_function("breakpoints", [this](const sol::this_state &thisState) {
        sol::state_view view(thisState);
        sol::table result = view.create_table();
        unsigned int index = 0;
        for (int row = 0; row < m_breakpoints->rowCount(); ++row) {
            const QString address = m_breakpoints->item(row, BREAK_ADDR_COL)->text();
            if (address == DEBUG_UNSET_ADDR) continue;
            result[++index] = view.create_table_with(
                "address", static_cast<uint32_t>(hex2int(address)),
                "label", m_breakpoints->item(row, BREAK_NAME_COL)->text().toStdString(),
                "enabled", static_cast<QAbstractButton *>(m_breakpoints->cellWidget(row, BREAK_ENABLE_COL))->isChecked());
        }
        return result;
    });
    debugTable.set_function("watchpoints", [this](const sol::this_state &thisState) {
        sol::state_view view(thisState);
        sol::table result = view.create_table();
        unsigned int index = 0;
        for (int row = 0; row < m_watchpoints->rowCount(); ++row) {
            const QString low = m_watchpoints->item(row, WATCH_LOW_COL)->text();
            const QString high = m_watchpoints->item(row, WATCH_HIGH_COL)->text();
            if (low == DEBUG_UNSET_ADDR || high == DEBUG_UNSET_ADDR) continue;
            result[++index] = view.create_table_with(
                "low", static_cast<uint32_t>(hex2int(low)),
                "high", static_cast<uint32_t>(hex2int(high)),
                "label", m_watchpoints->item(row, WATCH_NAME_COL)->text().toStdString(),
                "read", static_cast<QAbstractButton *>(m_watchpoints->cellWidget(row, WATCH_READ_COL))->isChecked(),
                "write", static_cast<QAbstractButton *>(m_watchpoints->cellWidget(row, WATCH_WRITE_COL))->isChecked());
        }
        return result;
    });
    debugTable.set_function("peripheralMonitors", [this](const sol::this_state &thisState) {
        sol::state_view view(thisState);
        sol::table result = view.create_table();
        unsigned int index = 0;
        for (int row = 0; row < m_ports->rowCount(); ++row) {
            const QString address = m_ports->item(row, PORT_ADDR_COL)->text();
            if (address == DEBUG_UNSET_PORT) continue;
            result[++index] = view.create_table_with(
                "address", static_cast<uint16_t>(hex2int(address)),
                "value", static_cast<uint8_t>(hex2int(m_ports->item(row, PORT_VALUE_COL)->text())),
                "read", static_cast<QAbstractButton *>(m_ports->cellWidget(row, PORT_READ_COL))->isChecked(),
                "write", static_cast<QAbstractButton *>(m_ports->cellWidget(row, PORT_WRITE_COL))->isChecked(),
                "freeze", static_cast<QAbstractButton *>(m_ports->cellWidget(row, PORT_FREEZE_COL))->isChecked());
        }
        return result;
    });
    debugTable.set_function("registerWatches", [](const sol::this_state &thisState) {
        sol::state_view view(thisState);
        sol::table result = view.create_table();
        unsigned int index = 0;
        for (unsigned int id = 0; id < RegisterNames.size(); ++id) {
            const int mask = debug_reg_get_mask(id);
            if (mask == DBG_MASK_NONE) continue;
            result[++index] = view.create_table_with(
                "name", RegisterNames[id],
                "id", id,
                "read", static_cast<bool>(mask & DBG_MASK_READ),
                "write", static_cast<bool>(mask & DBG_MASK_WRITE));
        }
        return result;
    });
    debugTable.set_function("registerSnapshot", [](const sol::this_state &thisState) {
        sol::state_view view(thisState);
        return view.create_table_with(
            "af", cpu.registers.AF, "bc", cpu.registers.BC, "de", cpu.registers.DE,
            "hl", cpu.registers.HL, "ix", cpu.registers.IX, "iy", cpu.registers.IY,
            "af_", cpu.registers._AF, "bc_", cpu.registers._BC, "de_", cpu.registers._DE,
            "hl_", cpu.registers._HL, "sps", cpu.registers.SPS, "spl", cpu.registers.SPL,
            "pc", cpu.registers.PC, "i", cpu.registers.I, "r", cpu.registers.R,
            "mbase", cpu.registers.MBASE, "adl", static_cast<bool>(cpu.ADL),
            "madl", static_cast<bool>(cpu.MADL), "halted", static_cast<bool>(cpu.halted),
            "ief1", static_cast<bool>(cpu.IEF1), "ief2", static_cast<bool>(cpu.IEF2));
    });
    debugTable.set_function("equates", [](const sol::this_state &thisState) {
        sol::state_view view(thisState);
        sol::table result = view.create_table(static_cast<int>(disasm.reverse.size()), 0);
        unsigned int index = 0;
        for (const auto &[name, address] : disasm.reverse) {
            result[++index] = view.create_table_with("name", name, "address", address);
        }
        return result;
    });
    debugTable.set_function("resolveSymbol", [](const sol::this_state &thisState, std::string name) {
        std::ranges::transform(name, name.begin(), [](unsigned char c) { return std::toupper(c); });
        const auto found = disasm.reverse.find(name);
        return found == disasm.reverse.end() ? sol::make_object(thisState, sol::lua_nil)
                                             : sol::make_object(thisState, found->second);
    });
    debugTable.set_function("symbolsAt", [](const sol::this_state &thisState, uint32_t address) {
        sol::state_view view(thisState);
        sol::table result = view.create_table();
        const auto [begin, end] = disasm.map.equal_range(address & 0xFFFFFF);
        unsigned int index = 0;
        for (auto symbol = begin; symbol != end; ++symbol) result[++index] = symbol->second;
        return result;
    });
    debugTable.set_function("symbolAt", [](const sol::this_state &thisState, uint32_t address) {
        const auto found = disasm.map.find(address & 0xFFFFFF);
        return found == disasm.map.end() ? sol::make_object(thisState, sol::lua_nil)
                                         : sol::make_object(thisState, found->second);
    });
    debugTable.set_function("loadEquates", [this](const std::string &path) {
        const QString file = QString::fromStdString(path);
        if (!m_equateFiles.contains(file)) m_equateFiles.append(file);
        equatesAddFile(file);
    });
    debugTable.set_function("clearBreakpoints", [this] {
        while (m_breakpoints->rowCount()) breakRemoveRow(m_breakpoints->rowCount() - 1);
    });
    debugTable.set_function("clearWatchpoints", [this] {
        while (m_watchpoints->rowCount()) watchRemoveRow(m_watchpoints->rowCount() - 1);
    });
    debugTable.set_function("gotoDisasm", [this](uint32_t address) { gotoDisasmAddr(address); });
    debugTable.set_function("disasm", [](const sol::this_state &thisState, uint32_t address, sol::optional<bool> useCpuMode) {
        const int32_t savedBase = disasm.base;
        const int32_t savedNext = disasm.next;
        const bool savedAdl = disasm.adl;
        const auto savedInstr = disasm.instr;
        const auto savedHighlight = disasm.highlight;
        std::string *savedCur = disasm.cur;

        const bool cpuMode = useCpuMode.value_or(true);
        disasm.base = static_cast<int32_t>(address & 0xFFFFFF);
        disasm.adl = cpuMode ? static_cast<bool>(cpu.ADL) : savedAdl;
        disasmGet(cpuMode);

        sol::state_view view(thisState);
        sol::table instruction = view.create_table();
        instruction["address"] = static_cast<uint32_t>(disasm.base);
        instruction["next"] = static_cast<uint32_t>(disasm.next);
        instruction["size"] = disasm.instr.size;
        instruction["bytes"] = disasm.instr.data;
        instruction["opcode"] = disasm.instr.opcode;
        instruction["operands"] = disasm.instr.operands;
        instruction["text"] = disasm.instr.opcode +
            (disasm.instr.operands.empty() ? std::string() : std::string(" ") + disasm.instr.operands);

        disasm.base = savedBase;
        disasm.next = savedNext;
        disasm.adl = savedAdl;
        disasm.instr = savedInstr;
        disasm.highlight = savedHighlight;
        disasm.cur = savedCur;
        return instruction;
    });

    sol::table basicTable = lua.create_named_table("basic");
    basicTable.set_function("enable", [this](bool enabled) { debugBasic(enabled); });
    basicTable.set_function("enabled", [] { return guiDebugBasic; });
    basicTable.set_function("showDebugger", [this] {
        if (!guiDebugBasic) debugBasic(true);
        ui->tabDebug->setCurrentWidget(ui->tab_tibasic_debugger);
        show();
        raise();
        activateWindow();
    });
    basicTable.set_function("state", [this](const sol::this_state &thisState) {
        const uint32_t begin = mem_peek_long(DBG_BASIC_BEGPC);
        const uint32_t current = mem_peek_long(DBG_BASIC_CURPC);
        const uint32_t end = mem_peek_long(DBG_BASIC_ENDPC);
        int sourceLine = 0;
        int byteOffset = -1;
        int index = m_basicCodeIndex;
        if (current >= begin && current <= end && index >= 0 && index < m_basicPrgmsTokensMap.size()) {
            byteOffset = static_cast<int>(current - begin);
            if (byteOffset < m_basicPrgmsTokensMap[index].size()) {
                sourceLine = m_basicPrgmsTokensMap[index][byteOffset].sourceLine + 1;
            }
        }
        sol::state_view view(thisState);
        return view.create_table_with(
            "enabled", guiDebugBasic, "paused", guiDebug,
            "program", debugBasicGetPrgmName().toStdString(),
            "begin", begin, "current", current, "end", end,
            "byteOffset", byteOffset, "line", sourceLine,
            "showFetches", m_basicShowFetches, "showTemporaryParser", m_basicShowTempParser,
            "liveExecution", m_basicShowLiveExecution, "highlight", m_basicShowHighlighted);
    });
    basicTable.set_function("source", [this](sol::optional<bool> temporary) {
        const int index = temporary.value_or(false) ? 0 : m_basicCodeIndex;
        return index >= 0 && index < m_basicPrgmsOriginalCode.size()
            ? m_basicPrgmsOriginalCode[index].toStdString() : std::string();
    });
    basicTable.set_function("step", [this] { debugBasicStep(); });
    basicTable.set_function("stepNext", [this] { debugBasicStepNext(); });
    basicTable.set_function("resume", [this] { if (guiDebug) debugToggle(); });
    basicTable.set_function("setHighlight", [this](bool enabled) { debugBasicToggleHighlight(enabled); });
    basicTable.set_function("setShowFetches", [this](bool enabled) { debugBasicToggleShowFetch(enabled); });
    basicTable.set_function("setShowTemporaryParser", [this](bool enabled) { debugBasicToggleShowTempParse(enabled); });
    basicTable.set_function("setLiveExecution", [this](bool enabled) { debugBasicToggleLiveExecution(enabled); });
    basicTable.set_function("setSourceBreakpoint", [this](unsigned int line, bool enabled,
                                                            sol::optional<bool> temporary) {
        if (line == 0) throw sol::error("TI-BASIC source lines are 1-based");
        const int index = temporary.value_or(false) ? 0 : m_basicCodeIndex;
        BasicEditor *editor = index == 0 ? ui->basicTempEdit : ui->basicEdit;
        debugBasicToggleBreakpoint(editor, index, static_cast<int>(line - 1), enabled);
    });
    basicTable.set_function("sourceBreakpoints", [this](const sol::this_state &thisState,
                                                          sol::optional<bool> temporary) {
        const int index = temporary.value_or(false) ? 0 : m_basicCodeIndex;
        sol::state_view view(thisState);
        sol::table result = view.create_table();
        if (index < 0 || index >= m_basicPrgmsIds.size()) return result;
        QList<int> lines = m_basicSourceBreakpoints.value(m_basicPrgmsIds[index]).values();
        std::sort(lines.begin(), lines.end());
        for (int item = 0; item < lines.size(); ++item) result[item + 1] = lines[item] + 1;
        return result;
    });
    basicTable.set_function("watchVariable", [this](const std::string &name, bool enabled) {
        m_varTableModel->refresh();
        const QString wanted = QString::fromStdString(name);
        for (int row = 0; row < m_varTableModel->rowCount(); ++row) {
            const QModelIndex index = m_varTableModel->index(row, VarTableModel::VAR_NAME_COL);
            if (index.data(Qt::DisplayRole).toString().compare(wanted, Qt::CaseInsensitive) == 0) {
                m_varTableModel->setWatched(index, enabled);
                return true;
            }
        }
        return false;
    });
    basicTable.set_function("watchedVariables", [this](const sol::this_state &thisState) {
        m_varTableModel->refresh();
        QStringList watched;
        for (int row = 0; row < m_varTableModel->rowCount(); ++row) {
            const QModelIndex index = m_varTableModel->index(row, VarTableModel::VAR_NAME_COL);
            if (m_varTableModel->isWatched(index)) watched.append(index.data(Qt::DisplayRole).toString());
        }
        return stringListTable(thisState, watched);
    });
    basicTable.set_function("prepareSource", [](const std::string &source) { return ti_basic_prepare_source(source); });
    basicTable.set_function("deindentSource", [](const std::string &source) { return ti_basic_deindent_source(source); });

    lua.create_named_table("autotester",
        "loadJSON", [this](const std::string &path) { return autotesterOpen(QString::fromStdString(path)); },
        "reloadJSON", [this] { autotesterReload(); },
        "launchTest", [this] { autotesterLaunch(); }
    );

    const sol::protected_function_result bootstrap = lua.safe_script(LuaEventBootstrap, sol::script_pass_on_error);
    if (!bootstrap.valid()) {
        console(QStringLiteral("[Lua] Event bootstrap failed: ") + scriptError(bootstrap) + QLatin1Char('\n'), EmuThread::ConsoleErr);
    }
    sol::table cemuTable = lua["cemu"];
    cemuTable.set_function("stopScript", [this, &lua] {
        runLuaCleanup(lua, "stopped");
        throw sol::error("script stopped");
    });
    cemuTable.set_function("reloadScript", [this, &lua, isREPL](sol::optional<std::string> requestedPath) {
        if (isREPL) throw sol::error("cemu.reloadScript is only available in the Scripts/editor runtime");
        const QString path = requestedPath
            ? QFileInfo(QString::fromStdString(*requestedPath)).absoluteFilePath()
            : m_lastLuaScriptPath;
        if (path.isEmpty()) throw sol::error("no script path is available to reload");
        if (!QFileInfo::exists(path)) throw sol::error("script to reload does not exist");
        runLuaCleanup(lua, "reload");
        QTimer::singleShot(0, this, [this, path] {
            initLuaThings(ed_lua, false);
            m_luaAutoloadRan = true;
            executeLuaFile(ed_lua, path);
        });
        return true;
    });
    lua.script("dbg.disasmPC = function() return dbg.disasm(cpu.registers.PC, true) end");
    if (isREPL) {
        lua.script("R, F = cpu.registers, cpu.registers.flags");
        m_replLuaInitialized = true;
    } else {
        m_edLuaInitialized = true;
    }
}

void MainWindow::setupLuaUi() {
    m_luaUnsafe = m_config->value(SETTING_LUA_UNSAFE, false).toBool();
    {
        const QSignalBlocker blocker(ui->checkLuaUnsafe);
        ui->checkLuaUnsafe->setChecked(m_luaUnsafe);
    }

    connect(ui->buttonRefreshLuaScripts, &QPushButton::clicked, this, &MainWindow::refreshLuaScripts);
    connect(ui->buttonOpenLuaScriptsFolder, &QPushButton::clicked, this, [this] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(luaScriptsPath()));
    });
    connect(ui->buttonRunSelectedLuaScript, &QPushButton::clicked, this, [this] {
        if (QListWidgetItem *item = ui->luaScriptList->currentItem()) {
            if (!m_edLuaInitialized) initLuaThings(ed_lua, false);
            executeLuaFile(ed_lua, item->data(Qt::UserRole).toString());
        }
    });
    connect(ui->luaScriptList, &QListWidget::itemSelectionChanged, this, [this] {
        ui->buttonRunSelectedLuaScript->setEnabled(ui->luaScriptList->currentItem() != nullptr);
    });
    connect(ui->luaScriptList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        loadLuaScript(item->data(Qt::UserRole).toString());
        ui->LuaTabs->setCurrentWidget(ui->LuaEditorTab);
    });
    connect(ui->luaScriptList, &QListWidget::itemChanged, this, [this] {
        if (m_refreshingLuaScripts) return;
        QStringList autoload;
        for (int row = 0; row < ui->luaScriptList->count(); ++row) {
            QListWidgetItem *item = ui->luaScriptList->item(row);
            if (item->checkState() == Qt::Checked) autoload.append(item->text());
        }
        m_config->setValue(SETTING_LUA_AUTOLOAD, autoload);
    });
    connect(ui->checkLuaUnsafe, &QCheckBox::toggled, this, &MainWindow::setLuaUnsafe);

    if (opts.useSettings) installLuaExamples();
    refreshLuaScripts();
}

QString MainWindow::luaScriptsPath() const {
    return QDir(QFileInfo(m_pathConfig).absolutePath()).filePath(QStringLiteral("scripts"));
}

void MainWindow::installLuaExamples() {
    QDir destination(luaScriptsPath());
    if (!destination.mkpath(QStringLiteral("."))) {
        console(QStringLiteral("[Lua] Could not create scripts folder: ") + destination.path() + QLatin1Char('\n'), EmuThread::ConsoleErr);
        return;
    }

    const QDir examples(QStringLiteral(":/lua/examples"));
    for (const QString &name : examples.entryList({QStringLiteral("*.lua")}, QDir::Files, QDir::Name)) {
        const QString target = destination.filePath(name);
        if (QFileInfo::exists(target)) continue;
        if (!QFile::copy(examples.filePath(name), target)) {
            console(QStringLiteral("[Lua] Could not install example script: ") + name + QLatin1Char('\n'), EmuThread::ConsoleErr);
            continue;
        }
        if (!QFile::setPermissions(target, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                            QFileDevice::ReadGroup | QFileDevice::ReadOther)) {
            console(QStringLiteral("[Lua] Could not make example script writable: ") + name + QLatin1Char('\n'), EmuThread::ConsoleErr);
        }
    }
}

void MainWindow::refreshLuaScripts() {
    const QStringList autoload = m_config->value(SETTING_LUA_AUTOLOAD).toStringList();
    const QDir directory(luaScriptsPath());
    m_refreshingLuaScripts = true;
    ui->luaScriptList->clear();
    for (const QFileInfo &file : directory.entryInfoList({QStringLiteral("*.lua")}, QDir::Files, QDir::Name)) {
        auto *item = new QListWidgetItem(file.fileName(), ui->luaScriptList);
        item->setData(Qt::UserRole, file.absoluteFilePath());
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(autoload.contains(file.fileName()) ? Qt::Checked : Qt::Unchecked);
        item->setToolTip(file.absoluteFilePath());
    }
    m_refreshingLuaScripts = false;
    ui->buttonRunSelectedLuaScript->setEnabled(false);
}

void MainWindow::setLuaUnsafe(bool enabled) {
    if (m_luaUnsafe == enabled) return;
    if (m_edLuaInitialized) runLuaCleanup(ed_lua, "reset");
    if (m_replLuaInitialized) runLuaCleanup(repl_lua, "reset");
    m_luaUnsafe = enabled;
    m_config->setValue(SETTING_LUA_UNSAFE, enabled);
    m_edLuaInitialized = false;
    m_replLuaInitialized = false;
    m_luaAutoloadRan = false;
    m_luaStartupEmitted = false;
    initLuaThings(repl_lua, true);
    runLuaStartupScripts({});
    console(QStringLiteral("[Lua] Unsafe libraries %1; Lua states were restarted.\n").arg(enabled ? QStringLiteral("enabled") : QStringLiteral("disabled")),
            EmuThread::ConsoleNorm);
}

bool MainWindow::executeLuaFile(sol::state &lua, const QString &path) {
    if (!QFileInfo::exists(path)) {
        console(QStringLiteral("[Lua] Script does not exist: ") + path + QLatin1Char('\n'), EmuThread::ConsoleErr);
        return false;
    }
    const QString absolutePath = QFileInfo(path).absoluteFilePath();
    if (&lua == &ed_lua) m_lastLuaScriptPath = absolutePath;
    sol::table cemu = lua["cemu"];
    cemu["scriptPath"] = absolutePath.toStdString();
    const sol::protected_function_result result = lua.safe_script_file(absolutePath.toStdString(), sol::script_pass_on_error);
    if (!result.valid()) {
        console(QStringLiteral("[Lua] ") + path + QStringLiteral(": ") + scriptError(result) + QLatin1Char('\n'), EmuThread::ConsoleErr);
        return false;
    }
    emitLuaEvent("script-loaded", [&absolutePath](sol::table &payload) { payload["path"] = absolutePath.toStdString(); });
    return true;
}

void MainWindow::runLuaCleanup(sol::state &lua, const std::string &reason) {
    sol::object cemuObject = lua["cemu"];
    if (!cemuObject.is<sol::table>()) return;
    sol::protected_function cleanup = cemuObject.as<sol::table>()["_cleanup"];
    if (!cleanup.valid()) return;
    const sol::protected_function_result result = cleanup(reason);
    if (!result.valid()) {
        console(QStringLiteral("[Lua] Cleanup failed: ") + scriptError(result) + QLatin1Char('\n'), EmuThread::ConsoleErr);
    }
}

void MainWindow::runLuaStartupScripts(const QStringList &cliScripts) {
    if (!m_edLuaInitialized) initLuaThings(ed_lua, false);

    if (!m_luaAutoloadRan) {
        const QStringList autoload = m_config->value(SETTING_LUA_AUTOLOAD).toStringList();
        for (const QString &name : autoload) executeLuaFile(ed_lua, QDir(luaScriptsPath()).filePath(name));
        m_luaAutoloadRan = true;
    }
    for (const QString &path : cliScripts) executeLuaFile(ed_lua, path);

    if (!m_luaStartupEmitted) {
        m_luaStartupEmitted = true;
        emitLuaEvent("startup", [this](sol::table &payload) {
            payload["scriptsPath"] = luaScriptsPath().toStdString();
            payload["unsafe"] = m_luaUnsafe;
        });
    }
}

bool MainWindow::emitLuaEventForState(sol::state &lua, bool initialized, const std::string &name,
                                      const std::function<void(sol::table &)> &populate) {
    if (!initialized) return true;
    sol::object cemuObject = lua["cemu"];
    if (!cemuObject.is<sol::table>()) return true;
    sol::table cemu = cemuObject.as<sol::table>();
    sol::protected_function dispatcher = cemu["_emit"];
    if (!dispatcher.valid()) return true;

    sol::table payload = lua.create_table();
    payload["event"] = name;
    payload["time"] = static_cast<double>(sched_total_time(CLOCK_48M)) / 48000.0;
    payload["cycles"] = sched_total_cycles();
    payload["pc"] = cpu.registers.PC;
    payload["paused"] = guiDebug;
    if (populate) populate(payload);
    const sol::protected_function_result result = dispatcher(name, payload);
    if (!result.valid()) {
        console(QStringLiteral("[Lua] Event dispatch failed: ") + scriptError(result) + QLatin1Char('\n'), EmuThread::ConsoleErr);
        return true;
    }
    return result.get_type() != sol::type::boolean || result.get<bool>();
}

bool MainWindow::emitLuaEvent(const std::string &name, const std::function<void(sol::table &)> &populate) {
    const bool editorResult = emitLuaEventForState(ed_lua, m_edLuaInitialized, name, populate);
    const bool replResult = emitLuaEventForState(repl_lua, m_replLuaInitialized, name, populate);
    return editorResult && replResult;
}

uint64_t MainWindow::addLuaTimer(sol::state &lua, sol::protected_function callback,
                                 uint64_t delay, uint64_t interval, bool cycles) {
    if (!callback.valid()) throw sol::error("timer callback must be a function");
    const uint64_t now = cycles ? sched_total_cycles() : sched_total_time(CLOCK_48M);
    if (delay > (std::numeric_limits<uint64_t>::max)() - now) throw sol::error("timer deadline is too large");
    const uint64_t id = m_nextLuaTimerId++;
    m_luaTimers.push_back({id, &lua, std::move(callback), now + delay, interval, cycles});
    if (!m_luaTimerPoll.isActive()) m_luaTimerPoll.start(1);
    return id;
}

bool MainWindow::cancelLuaTimer(uint64_t id) {
    const auto timer = std::ranges::find_if(m_luaTimers, [id](const LuaTimer &item) { return item.id == id; });
    if (timer == m_luaTimers.end()) return false;
    m_luaTimers.erase(timer);
    if (m_luaTimers.empty()) m_luaTimerPoll.stop();
    return true;
}

void MainWindow::clearLuaTimers(sol::state *lua) {
    std::erase_if(m_luaTimers, [lua](const LuaTimer &timer) { return !lua || timer.lua == lua; });
    if (m_luaTimers.empty()) m_luaTimerPoll.stop();
}

void MainWindow::processLuaTimers() {
    const uint64_t nowTime = sched_total_time(CLOCK_48M);
    const uint64_t nowCycles = sched_total_cycles();
    std::vector<uint64_t> due;
    due.reserve(m_luaTimers.size());
    for (const LuaTimer &timer : m_luaTimers) {
        if ((timer.cycles ? nowCycles : nowTime) >= timer.deadline) due.push_back(timer.id);
    }

    for (uint64_t id : due) {
        const auto found = std::ranges::find_if(m_luaTimers, [id](const LuaTimer &item) { return item.id == id; });
        if (found == m_luaTimers.end()) continue;

        sol::state *lua = found->lua;
        sol::protected_function callback = found->callback;
        const bool cycles = found->cycles;
        const uint64_t deadline = found->deadline;
        const uint64_t interval = found->interval;
        const uint64_t now = cycles ? nowCycles : nowTime;

        if (interval == 0) {
            m_luaTimers.erase(found);
        } else {
            const uint64_t elapsedIntervals = (now - deadline) / interval + 1;
            found->deadline += elapsedIntervals * interval;
        }

        sol::table payload = lua->create_table_with(
            "id", id,
            "time", static_cast<double>(nowTime) / 48000.0,
            "cycles", nowCycles);
        if (cycles) {
            payload["late"] = now - deadline;
        } else {
            payload["late"] = static_cast<double>(now - deadline) / 48000.0;
        }
        const sol::protected_function_result result = callback(payload);
        if (!result.valid()) {
            console(QStringLiteral("[Lua] Timer callback failed: ") + scriptError(result) + QLatin1Char('\n'), EmuThread::ConsoleErr);
            cancelLuaTimer(id);
        } else if (result.get_type() == sol::type::boolean && !result.get<bool>()) {
            cancelLuaTimer(id);
        }
    }

    if (m_luaTimers.empty()) m_luaTimerPoll.stop();
}

void MainWindow::loadLuaScript() {
    QFileDialog dialog(this);
    dialog.setDirectory(luaScriptsPath());
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(QStringLiteral("Lua script (*.lua)"));
    if (dialog.exec()) loadLuaScript(dialog.selectedFiles().constFirst());
}

void MainWindow::loadLuaScript(const QString &path) {
    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("File loading error"), tr("Error. Could not load that file."));
        return;
    }
    ui->luaScriptEditor->document()->setPlainText(QString::fromUtf8(file.readAll()));
    m_currentLuaScript = QFileInfo(path).absoluteFilePath();
}

void MainWindow::saveLuaScript() {
    QFileDialog dialog(this);
    dialog.setDirectory(m_currentLuaScript.isEmpty() ? luaScriptsPath() : QFileInfo(m_currentLuaScript).absolutePath());
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(QStringLiteral("Lua script (*.lua)"));
    if (!dialog.exec()) return;

    QString path = dialog.selectedFiles().constFirst();
    if (QFileInfo(path).suffix().isEmpty()) path += QStringLiteral(".lua");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("File writing error"), tr("Error. Could not write to that file."));
        return;
    }
    QTextStream(&file) << ui->luaScriptEditor->document()->toPlainText();
    m_currentLuaScript = QFileInfo(path).absoluteFilePath();
    refreshLuaScripts();
}

void MainWindow::runLuaScript() {
    initLuaThings(ed_lua, false);
    m_luaAutoloadRan = true;
    const std::string code = ui->luaScriptEditor->toPlainText().toStdString();
    const sol::protected_function_result result = ed_lua.safe_script(code, sol::script_pass_on_error);
    if (!result.valid()) {
        console(QStringLiteral("[Lua] ") + scriptError(result) + QLatin1Char('\n'), EmuThread::ConsoleErr);
    }
}

void MainWindow::LuaREPLeval() {
    std::string code = ui->REPLInput->text().toStdString();
    if (code.empty()) return;
    ui->REPLConsole->appendPlainText(QString::fromStdString("▶ " + code));
    ui->REPLInput->clear();
    if (code.starts_with("==")) {
        if (code.size() == 2) return;
        code = "print(string.format('hex: %X', " + code.substr(2) + "))";
    } else if (code.starts_with('=')) {
        if (code.size() == 1) return;
        code = "print(" + code.substr(1) + ")";
    }
    const sol::protected_function_result result = repl_lua.safe_script(code, sol::script_pass_on_error);
    if (!result.valid()) ui->REPLConsole->appendPlainText(QStringLiteral("[Lua] ") + scriptError(result));
}
