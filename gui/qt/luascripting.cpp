#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkAccessManager>
#include <QtWidgets/QMessageBox>
#include <QtCore/QFile>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "../../core/cpu.h"

void MainWindow::initLuaThings(sol::state& lua, bool isREPL) {

    lua = sol::state();

    lua.set_panic( [](lua_State* L) {
        const char* message = lua_tostring(L, -1);
        if (message) {
            fprintf(stderr, "[Lua Panic] %s\n", message);
            lua_pop(L, 1);
        }
        return -1;
    });

    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::coroutine, sol::lib::math, sol::lib::string, sol::lib::table, sol::lib::utf8);
    lua.script("require, loadfile, dofile = nil, nil, nil"); // More?
    // TODO: io.*, os.*, debug.*, require(), etc. => opt-in via settings.

    // Logging helpers (Note: print() goes to stdout)
    if (isREPL) {
        lua.set_function("cLog", [&](const sol::this_state& s) {
            lua_State* L = s;
            int nargs = lua_gettop(L);
            for (int i=1; i <= nargs; ++i) {
                ui->REPLConsole->appendPlainText(lua_tostring(L, i));
                if (i != nargs) { ui->REPLConsole->appendPlainText("\t"); }
            }
        });
        lua.set_function("cErr", [&](const sol::this_state& s) {
            lua_State* L = s;
            int nargs = lua_gettop(L);
            ui->REPLConsole->appendPlainText("[Error] ");
            for (int i=1; i <= nargs; ++i) {
                ui->REPLConsole->appendPlainText(lua_tostring(L, i));
                if (i != nargs) { ui->REPLConsole->appendPlainText("\t"); }
            }
        });
        lua.script("print = function(...) local a={...}; for _,v in pairs(a) do cLog(tostring(v)) end end");
    } else {
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
    }

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

    if (isREPL) {
        lua.script("R, F = cpu.registers, cpu.registers.flags");
    }
}

void MainWindow::loadLuaScript() {
    QFileDialog dialog(this);

    dialog.setDirectory(QDir::homePath());
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(QStringLiteral("Lua script (*.lua)"));
    if (!dialog.exec()) {
        return;
    }

    QFile file(dialog.selectedFiles().at(0));
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("File loading error"), tr("Error. Could not load that file."));
        return;
    }
    ui->luaScriptEditor->document()->setPlainText(file.readAll());
    file.close();
}

void MainWindow::saveLuaScript() {
    QFileDialog dialog(this);

    dialog.setDirectory(QDir::homePath());
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(QStringLiteral("Lua script (*.lua)"));
    if (!dialog.exec()) {
        return;
    }

    QFile file(dialog.selectedFiles().at(0));
    if(!file.open(QIODevice::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("File writing error"), tr("Error. Could not write to that file."));
        return;
    }
    QTextStream outStream(&file);
    outStream << ui->luaScriptEditor->document()->toPlainText();
    file.close();
}

void MainWindow::runLuaScript() {
    // Reset Lua engine and bindings
    this->initLuaThings(ed_lua, false);
    // TODO: maybe have a separate thread for Lua (because of infinite loops...)
    const std::string& code = ui->luaScriptEditor->toPlainText().toStdString();
    const sol::protected_function_result& stringresult = ed_lua.do_string(code.c_str());
    if (!stringresult.valid())
    {
        const sol::error& err = stringresult;
        consoleAppend("[Lua-Error] " + QString::fromStdString(err.what()) + "\n", Qt::darkRed);
    }
}

void MainWindow::LuaREPLeval() {
    // TODO: maybe have a separate thread for Lua (because of infinite loops...)
    std::string code = ui->REPLInput->text().toStdString();
    if (code.empty()) {
        return;
    }
    ui->REPLConsole->appendPlainText(QString::fromStdString("▶ " + code));
    if (code.substr(0, 2) == "==") {
        if (code.length() == 2) {
            return;
        }
        code = "print(string.format('hex: %X', " + code.substr(2) + "))";
    } else if (code.substr(0, 1) == "=") {
        if (code.length() == 1) {
            return;
        }
        code = "print(" + code.substr(1) + ")";
    }
    const sol::protected_function_result& stringresult = repl_lua.do_string(code);
    if (!stringresult.valid())
    {
        const sol::error& err = stringresult;
        ui->REPLConsole->appendPlainText("[Lua-Error] " + QString::fromStdString(err.what()));
    }
}
