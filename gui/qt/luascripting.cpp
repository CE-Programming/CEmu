#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkAccessManager>
#include <QtWidgets/QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "../../core/cpu.h"

void MainWindow::initLuaThings() {

    lua.set_panic( [](lua_State* L) {
        const char* message = lua_tostring(L, -1);
        if (message) {
            fprintf(stderr, "[Lua Panic] %s\n", message);
            lua_pop(L, 1);
        }
        return -1;
    });

    // TODO: How to secure things?
    lua.open_libraries();

    // Logging helpers (Note: print() goes to stdout)
    lua.set_function("cLog", [&](const sol::this_state& s) {
        lua_State* L = s;
        int nargs = lua_gettop(L);
        consoleAppend("[Lua] ");
        for (int i=1; i <= nargs; ++i) {
            consoleAppend(lua_tostring(L, i));
            if (i != nargs) { consoleAppend("\t"); }
        }
        consoleAppend("\n");
    });
    lua.set_function("cErr", [&](const sol::this_state& s) {
        lua_State* L = s;
        int nargs = lua_gettop(L);
        consoleAppend("[Lua] ", Qt::red);
        for (int i=1; i <= nargs; ++i) {
            consoleAppend(lua_tostring(L, i), Qt::red);
            if (i != nargs) { consoleAppend("\t"); }
        }
        consoleAppend("\n");
    });

    lua.set_function("mem_peek_byte",  [](uint32_t addr) { return mem_peek_byte(addr); });
    lua.set_function("mem_peek_short", [](uint32_t addr) { return mem_peek_short(addr); });
    lua.set_function("mem_peek_long",  [](uint32_t addr) { return mem_peek_long(addr); });
    lua.set_function("mem_peek_word",  [](uint32_t addr, bool mode) { return mem_peek_word(addr, mode); });
    lua.set_function("mem_poke_byte",  [](uint32_t addr, uint8_t val) { return mem_poke_byte(addr, val); });

    // Bind core stuff
    lua["cpu"] = std::cref(cpu);

    lua.new_usertype<decltype(cpu.registers.flags)>("eZ80flags_t",
#define FLAG(f) (#f), (sol::property([](decltype(cpu.registers.flags)& flags) -> bool { return flags.f; }, \
                                     [](decltype(cpu.registers.flags)& flags, bool val) -> void { flags.f = val; }))
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

#define FLAG(f) (#f), (sol::property([](eZ80cpu_t &cpu) -> bool { return cpu.f; }, [](eZ80cpu_t &cpu, bool val) -> void { cpu.f = val; }))
        FLAG(halted),
        FLAG(ADL), FLAG(MADL),
        FLAG(IEF1), FLAG(IEF2),
#undef FLAG

        "inBlock",  sol::property([](eZ80cpu_t &cpu) -> bool { return cpu.inBlock; }, [](eZ80cpu_t &, bool) -> void { /* RO */ }),
        "cycles",   sol::readonly(&eZ80cpu_t::cycles),
        "next",     sol::readonly(&eZ80cpu_t::next),
        "prefetch", sol::readonly(&eZ80cpu_t::prefetch)
    );

    // TODO: bind devices and other stuff.
}

void MainWindow::loadLuaScript() {
    // TODO
}

void MainWindow::saveLuaScript() {
    // TODO
}

void MainWindow::runLuaScript() {
    // TODO: maybe have a separate thread for Lua (because of infinite loops...)
    const std::string& code = ui->luaScriptEditor->toPlainText().toStdString();
    const sol::protected_function_result& stringresult = lua.do_string(code.c_str());
    if (!stringresult.valid())
    {
        const sol::error& err = stringresult;
        consoleErrStr("[Lua-Error] " + QString::fromStdString(err.what()) + "\n");
    }
}
