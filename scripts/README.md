# CEmu Lua scripts

This directory is the versioned, shareable home for CEmu Lua scripts. The examples below are embedded in CEmu and copied into the active configuration directory's `scripts` folder on first use. User copies are never overwritten, and newly installed examples are not enabled for autoload automatically.

## Simple examples

- `hello_cemu.lua`: print basic CPU and device state and update the status bar.
- `memory_dump.lua`: format a read-only 64-byte RAM dump.
- `bulk_memory.lua`: read, checksum, and search a memory range with the bulk APIs.
- `variable_inspector.lua`: list VAT entries and inspect one variable's raw storage.
- `key_sequence.lua`: send individual key actions and a timed sequence.
- `screenshot.lua`: save a timestamped screenshot to the desktop.
- `event_logger.lua`: log the supported non-frame events.

## Inspection and advanced examples

- `disassembly_walk.lua`: decode instructions starting at the current PC.
- `debugger_snapshot.lua`: report registers, debugger configuration, equates, and the next instruction.
- `peripheral_inspector.lua`: summarize every core peripheral range and the LCD state.
- `coprocessor_inspector.lua`: report the Python ARM bootloader and synchronized Cortex-M0+ state.
- `lcd_diagnostics.lua`: report detailed PL111, ST7789, gamma, timing, and backlight state.
- `framebuffer_inspection.lua`: inspect framebuffer metadata, pixels, regions, hashes, and the next frame.
- `filtered_events.lua`: install a filtered, one-shot keypad event handler.
- `virtual_time.lua`: schedule cancellable one-shot and repeating callbacks in emulated time.
- `lifecycle_cleanup.lua`: own event/timer resources and release them explicitly or on state teardown.
- `transfer_monitor.lua`: report transfer status and events without starting a transfer.
- `conditional_breakpoint.lua`: stop at a breakpoint only when a Lua condition matches.
- `memory_watchpoint.lua`: log writes to a RAM range; run it again to remove the hook.
- `register_watch.lua`: mirror a CPU Register Watch row with a removable callback.
- `peripheral_monitor.lua`: mirror a Port Monitor row; run it again to remove the monitor.
- `basic_debugger.lua`: enable TI-BASIC source debugging and report source-level events.
- `tas_demo.lua`: run deterministic key input and capture a screenshot while safely restoring throttling.

The callback, timer, transfer-monitor, breakpoint, watchpoint, peripheral-monitor, TI-BASIC, key-input, and TAS examples retain handlers or change emulator/debugger state. Run them deliberately; long-running examples either remove themselves after one match or toggle themselves off when run a second time in the same Scripts-tab state.

See [`docs/lua-scripting.md`](../docs/lua-scripting.md) for the API, event, autoload, command-line, conditional-breakpoint, and TAS workflow documentation.
