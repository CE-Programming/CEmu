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
    lua["cpu"] = &cpu;

    // TODO: find out what to do for bitfields (like the flags)
    /*
    lua.new_usertype<eZ80flags_t>("eZ80flags_t",

    );
    */

    lua.new_usertype<eZ80registers_t>("eZ80registers_t",
        // "flags", &eZ80registers_t::flags,
#define RP(reg) (#reg), (&eZ80registers_t::reg)
        RP(af), RP(AF), RP(F), RP(A), RP(bc), RP(BC), RP(BCS), RP(C), RP(B), RP(BCU), RP(de), RP(DE),
        RP(DES), RP(E), RP(D), RP(DEU), RP(hl), RP(HL), RP(HLS), RP(L), RP(H), RP(HLU), RP(_HL), RP(ix),
        RP(IX), RP(IXS), RP(IXL), RP(IXH), RP(IXU), RP(iy), RP(IY), RP(IYS), RP(IYL), RP(IYH), RP(IYU),
        RP(_AF), RP(_BC), RP(_DE), RP(SPS), RP(SPSU), RP(SPL), RP(pc), RP(PC), RP(PCS), RP(PCL), RP(PCH),
        RP(PCU), RP(rawPC), RP(I), RP(R), RP(MBASE)
#undef RP
    );

    lua.new_usertype<eZ80cpu_t>("eZ80cpu_t",
        "registers", &eZ80cpu_t::registers,

        "halted", &eZ80cpu_t::halted,
        "ADL", &eZ80cpu_t::ADL, "MADL", &eZ80cpu_t::MADL,
        "IEF1", &eZ80cpu_t::IEF1, "IEF2",&eZ80cpu_t::IEF2,

        "inBlock",  sol::readonly(&eZ80cpu_t::inBlock),
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
