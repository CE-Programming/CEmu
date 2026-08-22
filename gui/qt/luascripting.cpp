#include <QtCore/QFileInfo>
#include <QtWidgets/QMessageBox>
#include <QtCore/QFile>
#include <QtCore/QDateTime>

#include <algorithm>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "utils.h"
#include "sendinghandler.h"

#include "../../core/asic.h"
#include "../../core/cpu.h"
#include "../../core/mem.h"
#include "../../core/link.h"

namespace {

QString scriptError(const sol::protected_function_result &result) {
    const sol::error error = result;
    return QString::fromStdString(error.what());
}

} // namespace

void MainWindow::initLuaThings(sol::state &lua, bool isREPL) {

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
        "quit", [] { QTimer::singleShot(0, qApp, &QCoreApplication::quit); }
    );

    lua.create_named_table("emu",
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

    lua.create_named_table("autotester",
        "loadJSON", [this](const std::string &path) { return autotesterOpen(QString::fromStdString(path)); },
        "reloadJSON", [this] { autotesterReload(); },
        "launchTest", [this] { autotesterLaunch(); }
    );

    lua.script("dbg.disasmPC = function() return dbg.disasm(cpu.registers.PC, true) end");
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
    initLuaThings(ed_lua, false);
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
