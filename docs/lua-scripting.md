# Lua scripting

CEmu embeds Lua 5.4 or newer (currently Lua 5.5.1 in vcpkg) through Sol2 3.5. Scripts can be run from the editor, the REPL, the Scripts tab, an autoload list, or one or more `--lua-script <path>` command-line options. The Scripts tab uses a `scripts` directory beside the active CEmu configuration file. Bundled examples are copied there only when the corresponding file does not already exist.

## Safety and lifecycle

Lua is inactive by default, so normal emulation does not create a Lua state. It is enabled for the current session when a checked autoload script or `--lua-script` option is present at startup, or when the user runs a script, enters a REPL command, resets the REPL state, or checks **Enable Lua scripting for this session**. An explicit `--lua-script` sent to an existing instance through IPC also enables it; unrelated IPC commands leave the Lua runtime untouched.

Unchecking the session option runs registered cleanup callbacks, cancels virtual timers, and destroys both the Scripts/editor and REPL states. Checked autoload entries remain configured for the next startup. Event delivery is subscription-driven: when neither Lua state has a handler for an event, CEmu does not allocate its payload or enter the Lua dispatcher.

The default Lua state exposes the base, coroutine, math, string, table, and UTF-8 libraries. `require`, `loadfile`, `dofile`, `package`, `io`, `os`, and the standard `debug` module are unavailable. The Scripts tab can enable them for trusted scripts; changing this setting restarts any active Lua states and reloads checked autoload scripts. CEmu's debugger bindings always use the separate `dbg` table, so enabling Lua's standard `debug` library does not replace them.

Checked scripts are loaded in filename order after the emulator is initialized. Repeated `--lua-script` arguments run afterward, in command-line order, in the same persistent runtime. Running the editor explicitly starts a fresh runtime. The REPL has a separate state so experiments cannot overwrite autoload-script globals.

`cemu.onUnload(callback)` registers state-scoped cleanup in last-in, first-out order; remove it with `cemu.offUnload(id)`. `cemu.cleanup()` runs cleanup callbacks and cancels every event handler and virtual timer in that state. CEmu invokes the same cleanup automatically before resetting a Lua state. `cemu.stopScript()` additionally aborts the current Lua invocation at that cooperative call point.

`cemu.reloadScript([path])` queues a clean Scripts/editor-state restart and executes the requested file, or the most recently executed file when omitted. The current absolute file is available as `cemu.scriptPath`. Reload is deliberately queued until the current callback returns; it is unavailable in the REPL, which has its own explicit reset button.

## Events and conditional breakpoints

Register a callback with `cemu.on(name, callback[, options])` and remove it with `cemu.off(name, id)`. Every callback receives a table containing `event`, monotonically increasing per-state `sequence`, emulated `time` in milliseconds, total `cycles`, `pc`, and `paused`, in addition to the event-specific fields below. Supported events are:

- `startup`: `scriptsPath`, `unsafe`
- `loaded`: `type`, `deviceType`
- `reset`: `phase`
- `key`: `row`, `column`, `pressed`, `repeat`
- `frame`: `fps`
- `breakpoint`: `address`, `label`, `pc`
- `watchpoint`: `address`, `label`, `write`, `pc`
- `register-read`, `register-write`: `id`, `name`, `write`, `pc`
- `peripheral-read`, `peripheral-write`: `address`, `range`, `value`, `write`, `pc`
- `basic-breakpoint`, `basic-step`: `program`, `line`, `byteOffset`, `pc`
- `basic-variable-change`: `program`, `variables`, `pc`
- `script-loaded`: `path`

Handlers run in registration order. Errors are reported to the Lua console and do not stop other handlers. For events that normally stop in the debugger (`breakpoint`, `watchpoint`, register/peripheral monitors, and TI-BASIC debug events), returning `false` suppresses the stop and resumes emulation. This makes a conditional breakpoint a normal breakpoint plus a callback:

```lua
local address = 0xD1A881
dbg.addBreakpoint(address, "conditional")
cemu.on("breakpoint", function(event)
    if event.address ~= address then return true end
    return mem.readByte(0xD00000) == 1
end)
```

Event options filter before invoking the callback. Any scalar option other than `once` and `predicate` must equal the same payload field; alternatively put those comparisons in a `where` table. `once = true` removes the handler on its first match, and `predicate` receives the payload for more complex filtering:

```lua
cemu.on("peripheral-write", function(event)
    print(string.format("USB write %04x = %02x", event.address, event.value))
end, {
    once = true,
    predicate = function(event) return event.address >= 0x3000 and event.address < 0x4000 end,
})
```

## Bindings

- `cpu`, `cpu.registers`, and `cpu.registers.flags` expose the current eZ80 state. `R` and `F` are REPL shortcuts.
- `mem.readByte/readShort/readLong/readWord(address)` and matching `write*` functions access calculator memory.
- `mem.read(address, length)` returns a binary string, while `mem.readTable(address, length)` returns a 1-based byte table. `mem.write(address, data)` accepts either representation.
- `mem.fill(address, length, byte)`, `mem.copy(destination, source, length)`, `mem.crc32(address, length)`, and `mem.search(address, length, pattern[, maxMatches])` support efficient bulk inspection and editing. Searches return a 1-based table of addresses and default to at most 1024 matches.
- `vars.list([type])` returns VAT metadata snapshots; `vars.find(name[, type])` returns one snapshot or `nil`. A type may be its numeric ID or a case-insensitive type name from `vars.types()`.
- `vars.read(name[, type])` returns the variable's raw calculator storage as a binary string, including the normal two-byte data-length prefix where the OS format has one. `vars.launch(name[, type])` launches a non-internal program through the same keypad path as the variable pane.
- `tivars.create(type, name[, content[, options]])` constructs a host-side calculator variable with `tivars_lib_cpp`; unlike `vars`, it does not require the variable to exist in the calculator VAT. A type may be a numeric ID or case-insensitive type name. The model defaults to the active CEmu model and can be overridden with `options.model`.
- A created `tivars.Variable` has mutable `name` and `archived` properties plus read-only `type`, `typeId`, `model`, and `extension`. `setContent` and `content` convert between readable text and calculator data, while `setRawContent`, `rawContent`, `bytes`, and `save(path)` provide binary content, a complete serialized calculator file, and filesystem output. Binary values are Lua strings; raw setters also accept 1-based byte tables.
- `variable:send([location])` serializes and transfers a temporary file, removing it after completion or failure. The default is RAM or Archive according to `variable.archived`; an explicit location uses the same `"ram"`, `"archive"`, or `"auto"` values as `link.send`. `tivars.currentModel()`, `tivars.models()`, and `tivars.types([model])` expose creation metadata; type entries include a `writable` flag for readable-content conversion support.
- Creation options accept `archived`, `model`, `raw`, `prepareBasic`, and a `conversion` table of handler-specific integer or boolean options. Archive storage is rejected for legacy models that do not support it. TI-BASIC Program and ProtectedProgram source uses `basic.prepareSource` normalization by default; set `prepareBasic = false` to pass source directly to `tivars_lib_cpp`, or `raw = true` to treat the initial content as calculator bytes.
- `peripherals.peek(address)` and `peripherals.poke(address, value)` access any byte in the complete 16-bit peripheral port space without emulating bus timing. `peripherals.read/write` perform a real CPU-style port access, including timing, device side effects, and monitors. `peripherals.snapshot(address[, length])` returns a 1-based byte array, and `peripherals.describe(address)` identifies its named range and offset.
- `peripherals.ranges` names every 4 KiB APB range: `control`, `flash`, `sha256`, `usb`, `lcd`, `interrupts`, `watchdog`, `timers`, `rtc`, `protected`, `keypad`, `backlight`, `misc`, `spi`, `uart`, and `reserved`. Each entry has `base`, `last`, and `size`.
- `peripherals.monitor(address[, read[, write[, freeze]]])` configures the equivalent of a Port Monitor row and returns its mask. `peripherals.monitorState(address)` returns the three flags. A frozen port ignores normal writes.
- `keys.press(name)`, `keys.down(name)`, `keys.up(name)`, `keys.hold(name, ms)`, and `keys.sequence(sequence)` drive calculator input. The sequence grammar accepts comma-separated key names plus `press:name`, `down:name`, `up:name`, `hold:name:ms`, and `delay:ms`.
- `gui.screenshot([path])`, `gui.refresh()`, `gui.messageBox(title, message)`, `gui.status(message)`, `gui.setKeypadColor(color)`, `gui.setFullscreen(mode)`, `gui.openScriptsFolder()`, and `gui.quit()` control common UI operations.
- `emu.reset()`, `emu.reloadROM()`, `emu.throttle(enabled)`, `emu.setSpeed(percent)`, `emu.wait(ms)`, `emu.saveState(path)`, `emu.loadState(path)`, `emu.sendFile(path)`, and `emu.deviceType()` control emulator operations.
- `emu.time()` returns elapsed emulated milliseconds and `emu.cycles()` returns total CPU cycles. `emu.after`, `emu.afterCycles`, `emu.every`, and `emu.everyCycles` schedule callbacks against those virtual clocks and return IDs accepted by `emu.cancel`; `emu.cancelAll()` removes timers owned by the current Lua state. Returning `false` from a repeating callback cancels it.
- `link.send(path[, location])` queues a calculator file transfer and returns whether it started. Location is `"ram"`, `"archive"`, or `"auto"` (also available as `link.RAM`, `link.ARCHIVE`, and `link.AUTO`). `link.cancel()`, `link.busy()`, and `link.status()` provide cancellation and a snapshot of the current or last transfer.
- Transfers emit `transfer-start`, `transfer-progress`, and either `transfer-complete` or `transfer-error`. Payloads report file paths, location, progress, and completion/cancellation status as applicable; these events also receive the standard emulator event metadata.
- `dbg.stop/resume/stepIn/stepOver/stepNext/stepOut/stepUntilReturn`, `dbg.addBreakpoint(address[, label])`, `dbg.removeBreakpoint(address)`, and `dbg.gotoDisasm(address)` control the debugger.
- `dbg.addWatchpoint(low, high, read, write[, label])` and `dbg.removeWatchpoint(low)` mirror memory watchpoints. `dbg.watchRegister(name, read, write)` and `dbg.registerWatchState(name)` mirror CPU-register watches; names are the lowercase debugger names such as `a`, `af`, `hl`, `pc`, and `mbase` (alternate registers use a trailing underscore).
- `dbg.breakpoints()`, `dbg.watchpoints()`, `dbg.peripheralMonitors()`, and `dbg.registerWatches()` return coherent configuration snapshots. `dbg.registerSnapshot()` captures the principal CPU registers and mode flags in one table. `dbg.clearBreakpoints()` and `dbg.clearWatchpoints()` remove their respective configurations.
- `dbg.equates()` enumerates symbols, `dbg.resolveSymbol(name)` resolves a name, and `dbg.symbolAt(address)`/`dbg.symbolsAt(address)` perform the reverse lookup. `dbg.loadEquates(path)` loads a supported debugger equate file and retains it in the debugger configuration.
- `dbg.disasm(address[, useCpuMode])` uses CEmu's zdis decoder and returns `address`, `next`, `size`, `bytes`, `opcode`, `operands`, and `text`. `dbg.disasmPC()` decodes the current PC.
- `autotester.loadJSON(path)`, `autotester.reloadJSON()`, and `autotester.launchTest()` bridge existing deterministic tests.

## LCD controller and panel

`lcd.controllerState()` returns the PL111 registers plus decoded timing, DMA, position, and phase state. `lcd.panelState()` returns the ST7789 command/parser state, address and scroll windows, MADCTL and pixel formats, effective timing, mode flags, clock settings, and both complete gamma curves. `lcd.state()` combines those snapshots with backlight state.

`lcd.panelCommand(command, parameters)` sends an ST7789 command through the same parser and side-effect path as the LCD debugger pane. Emulation must be paused; parameters are a 1-based table of bytes. This provides access to the complete panel register/command set without binding the pane's Qt controls.

`lcd.refreshDebugPane()`, `lcd.applyDebugPane()`, and `lcd.showDebugPane()` bridge the useful pane operations. Applying edits also requires paused emulation. `lcd.setDma`, `lcd.setGamma`, `lcd.setResponse`, `lcd.setScale`, `lcd.setUpscale`, and `lcd.setSkin` expose scriptable screen behavior.

For visual assertions, `lcd.width` and `lcd.height` are 320 and 240. `lcd.framebuffer([format])` returns a row-major binary string and `lcd.region(x, y, width, height[, format])` returns a crop; format is `"rgba"` (the default, four bytes per pixel in R-G-B-A order) or `"rgb"`. Coordinates are zero-based.

`lcd.pixel(x, y)` returns `0xRRGGBB`, `lcd.matches(x, y, color[, tolerance])` compares each color channel, and `lcd.frameHash()` returns an IEEE CRC-32 of the complete RGBA frame. `lcd.frameInfo()` includes dimensions, format, byte count, CRC, display/DMA state, and backlight factor. To wait without blocking or re-entering Lua, subscribe to `frame` with `{ once = true }` and inspect the framebuffer from that callback.

```lua
local controller = lcd.controllerState()
local panel = lcd.panelState()
print(string.format("DMA=%06x row=%d col=%d", controller.upperCurrent,
                    controller.currentRow, controller.currentColumn))
print("ST7789 window", panel.columnStart, panel.columnEnd, panel.rowStart, panel.rowEnd)
```

## TI-BASIC debugger

`basic.enable(enabled)`, `basic.enabled()`, and `basic.showDebugger()` control source-level debugging. `basic.state()` reports execution addresses, current program, 1-based source line, byte offset, paused state, and display options. `basic.source([temporary])` returns the cached detokenized source.

`basic.step()`, `basic.stepNext()`, and `basic.resume()` control execution. `basic.setHighlight`, `basic.setShowFetches`, `basic.setShowTemporaryParser`, and `basic.setLiveExecution` configure source debugging. Source lines passed to `basic.setSourceBreakpoint(line, enabled[, temporary])` and returned by `basic.sourceBreakpoints([temporary])` are 1-based.

`basic.watchVariable(name, enabled)` configures a value-change stop for a currently present calculator variable, and `basic.watchedVariables()` returns the names being watched. `basic.prepareSource(source)` and `basic.deindentSource(source)` expose the same normalization used by the editable TI-BASIC viewer.

## Bundled examples

The Scripts tab installs editable examples without overwriting existing copies. Start with `hello_cemu.lua`, `memory_dump.lua`, `bulk_memory.lua`, `variable_inspector.lua`, `key_sequence.lua`, and `screenshot.lua`. `create_program.lua` builds a TI-BASIC program with `tivars_lib_cpp` and sends it to calculator RAM. The event logger, disassembly, peripheral, LCD, debugger-snapshot, and framebuffer scripts demonstrate read-only inspection.

`filtered_events.lua`, `virtual_time.lua`, `lifecycle_cleanup.lua`, and `transfer_monitor.lua` demonstrate persistent callbacks and their cleanup. `conditional_breakpoint.lua`, `memory_watchpoint.lua`, `register_watch.lua`, `peripheral_monitor.lua`, `basic_debugger.lua`, and `tas_demo.lua` install debugger hooks or change emulator state and should be run deliberately.

Advanced monitor examples toggle themselves off when run a second time in the same Scripts-tab state. Scripts are installed unchecked and do not autoload until explicitly enabled in the Scripts tab.

## TAS workflow

For repeatable input, start from a known state with `emu.loadState`, disable wall-clock throttling, use `keys.sequence` with explicit holds/delays, and capture checkpoints with `gui.screenshot` or `emu.saveState`. Existing JSON autotests can still be loaded and launched from Lua. CEmu advances emulation on its worker thread, so `emu.wait(ms)` keeps the GUI event loop alive; use explicit state and input timing rather than host-side busy loops.

Virtual-time timers continue only while emulation advances. The callback receives `id`, `time`, `cycles`, and `late`; the last field is expressed in the timer's own unit. Timers are discarded when their Lua state is reset, so they cannot call functions retained from an obsolete runtime.

## CLI examples

```sh
CEmu --image known-state.ce --lua-script scripts/setup.lua --lua-script scripts/run.lua
CEmu --lua-script scripts/examples/event_logger.lua
```

When `--id` targets an already-running CEmu instance, Lua script paths are delivered through the same IPC command as the other command-line actions.
