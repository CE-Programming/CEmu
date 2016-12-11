#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkAccessManager>
#include <QtWidgets/QMessageBox>
#include <QtCore/QFile>
#include <QtCore/QDateTime>

#include <thread>
#include <chrono>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "utils.h"
#include "sendinghandler.h"

#include "../../core/cpu.h"
#include "../../core/mem.h"
#include "../../core/link.h"


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
            console(EmuThread::ConsoleNorm, "[Lua] ");
            for (int i=1; i <= nargs; ++i) {
                console(EmuThread::ConsoleNorm, lua_tostring(L, i));
                if (i != nargs) { console(EmuThread::ConsoleNorm, "\t"); }
            }
            console(EmuThread::ConsoleNorm, "\n");
        });
        lua.set_function("cErr", [&](const sol::this_state& s) {
            lua_State* L = s;
            int nargs = lua_gettop(L);
            console(EmuThread::ConsoleErr, "[Lua] ");
            for (int i=1; i <= nargs; ++i) {
                console(EmuThread::ConsoleErr, lua_tostring(L, i));
                if (i != nargs) { console(EmuThread::ConsoleErr, "\t"); }
            }
            console(EmuThread::ConsoleErr, "\n");
        });
    }

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

    lua.create_named_table("mem",
       "readByte",   mem_peek_byte,
       "readShort",  mem_peek_short,
       "readLong",   mem_peek_long,
       "readWord",   mem_peek_word,
       "writeByte",  mem_poke_byte,
       "writeShort", mem_poke_short,
       "writeLong",  mem_poke_long,
       "writeWord",  mem_poke_word
    );

    lua.set_function("pressKey",  [&](const std::string& key) { pressKeyFromName(key); });
    lua.set_function("pressKeys", [&](const sol::variadic_args& keys) {
        for (const std::string key: keys) {
            pressKeyFromName(key);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    lua.create_named_table("gui",
       "screenshot", sol::as_function([&](const std::string& p = "") -> int {
           QString path = QString::fromStdString(p);
           if (path.length() == 0) {
               path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + QDir::separator();
           }
           if (!path.endsWith(".png")) {
               if (!path.endsWith(QDir::separator())) {
                   path += QDir::separator();
               }
               const QString now = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss");
               path += QStringLiteral("CEmu_screenshot_") + now + QStringLiteral(".png");
           }
           return !(ui->lcd->getImage().save(path, "PNG", 0));
       }),
       "refresh",    sol::as_function([] { QApplication::processEvents(); }),
       "messageBox", sol::as_function([&](const std::string& t, const std::string& msg) {
           QMessageBox::information(this, QString::fromStdString("[Lua] " + t), QString::fromStdString(msg));
       }),
       "setKeypadColor", sol::as_function([&](unsigned c){ setKeypadColor(c); })

       // todo: see mainwindow.h -> Settings
    );

    lua.create_named_table("emu",
       "reset",     sol::as_function([&] { resetEmu(); }),
       "reloadROM", sol::as_function([&] { emuLoad(EMU_DATA_ROM); }),
       "throttle",  sol::as_function([&](bool t){ setThrottle(t ? Qt::Checked : Qt::Unchecked); }),
       "setSpeed",  sol::as_function([&](int s){ s /= 10; setEmuSpeed(s<0 ? 0 : (s>50 ? 50 : s)); }), // todo: maybe fix this
       "sendFile",  sol::as_function([&](const std::string& path) {
           sendingHandler->sendFiles(QStringList(QString::fromStdString(path)), LINK_FILE);
       })

        // todo: getFile
    );

    lua.create_named_table("debug",
       "stop",      sol::as_function([&] { if (!guiDebug) { debugToggle(); } }),
       "resume",    sol::as_function([&] { if (guiDebug)  { debugToggle(); } }),
       "stepIn",    sol::as_function([&] { stepIn(); }),
       "stepOver",  sol::as_function([&] { stepOver(); }),
       "stepNext",  sol::as_function([&] { stepNext(); }),
       "stepOut",   sol::as_function([&] { stepOut(); }),
       "disasm",    sol::as_function([&](uint32_t addr) -> auto {
           /* todo with zdis compat */
           (void)addr;
       })

       // todo: equates, breakpoints, watchpoints, port monitor stuff, etc.
    );
    lua.script("debug.disasmPC = function() cLog('todo') end");

    lua.create_named_table("autotester",
       "loadJSON",   sol::as_function([&](const std::string& path) -> int { return autotesterOpen(QString::fromStdString(path)); }),
       "reloadJSON", sol::as_function([&] { autotesterReload(); }),
       "launchTest", sol::as_function([&] { autotesterLaunch(); })

      // todo: actually test this
    );

    // TODO: bind more stuff (all features that are accessible from the GUI should have a Lua equivalent)

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
    const sol::protected_function_result& stringresult = ed_lua.do_string(code);
    if (!stringresult.valid())
    {
        const sol::error& err = stringresult;
        const std::string errStr = std::string("[Lua-Error] ") + err.what() + "\n";
        console(EmuThread::ConsoleNorm, errStr.c_str());
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
