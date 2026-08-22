#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSignalBlocker>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMessageBox>

#include <algorithm>
#include <string>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "sendinghandler.h"
#include "utils.h"

#include "../../core/asic.h"
#include "../../core/cpu.h"
#include "../../core/link.h"
#include "../../core/mem.h"

namespace {

constexpr auto LuaEventBootstrap = R"lua(
cemu = cemu or {}
cemu._handlers = {}
cemu._nextHandler = 0
cemu._eventSequence = 0

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
        "readByte", mem_peek_byte,
        "readShort", mem_peek_short,
        "readLong", mem_peek_long,
        "readWord", mem_peek_word,
        "writeByte", mem_poke_byte,
        "writeShort", mem_poke_short,
        "writeLong", mem_poke_long,
        "writeWord", mem_poke_word
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
        "openScriptsFolder", [this] { QDesktopServices::openUrl(QUrl::fromLocalFile(luaScriptsPath())); },
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

    const sol::protected_function_result bootstrap = lua.safe_script(LuaEventBootstrap, sol::script_pass_on_error);
    if (!bootstrap.valid()) {
        console(QStringLiteral("[Lua] Event bootstrap failed: ") + scriptError(bootstrap) + QLatin1Char('\n'), EmuThread::ConsoleErr);
    }
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
    const sol::protected_function_result result = lua.safe_script_file(path.toStdString(), sol::script_pass_on_error);
    if (!result.valid()) {
        console(QStringLiteral("[Lua] ") + path + QStringLiteral(": ") + scriptError(result) + QLatin1Char('\n'), EmuThread::ConsoleErr);
        return false;
    }
    emitLuaEvent("script-loaded", [&path](sol::table &payload) { payload["path"] = path.toStdString(); });
    return true;
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
