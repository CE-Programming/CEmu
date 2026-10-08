# CEmu headless control protocol

`cemu-headless` runs the emulator core without Qt and accepts line-oriented
commands on standard input. Each command produces exactly one `OK` or `ERR`
response on standard output. Core diagnostics are written to standard error.

Start it with either a ROM or a saved CEmu image.

The process prints `CEMU_HEADLESS_READY` when it is ready for commands.

Supported commands:

- `run <ms>` advances the emulator by the requested emulated milliseconds.
- `run-realtime <ms>` advances in one-millisecond steps paced against the host
  clock, allowing asynchronous physical USB reset and hotplug work to settle.
- `key <name> [hold-ms]` presses and releases one calculator key.
- `keys <sequence>` runs the same comma-separated key syntax as the autotester.
- `screenshot <path>` writes the current 320x240 LCD as a 24-bit BMP.
- `screen-hash` returns an FNV-1a hash of the RGBA8888 LCD frame.
- `save-state <path>` writes a CEmu `.ce` state.
- `send-file [ram|archive|auto] <path>` transfers a calculator file. The
  destination defaults to `auto`.
- `usb <VID:PID|bus#address|disconnect>` attaches a physical USB device to the
  emulated host controller, or disconnects the current USB peer.
- `reset` resets the emulated calculator.
- `status` reports the device, ASIC revision, Python flag, and run rate.
- `peek <hex-address> [count]` returns `count` bytes (1 to 4096, default 1) read
  from the address space without side effects, as hex: `OK peek 3F00C9`.
- `poke <hex-address> <hex-bytes>` writes bytes (flash included, ignoring its
  protection), for example `poke D0008E 01`.
- `keydown <name>` and `keyup <name>` press and release a key separately, without
  running the emulator, to inspect the calculator while a key is held.
- `regs` reports the CPU registers (PC, SPL, AF, BC, DE, HL, IX, IY), ADL and
  whether the CPU is halted. With a stack of IX frames, the caller's IX is at
  (IX) and the return address at (IX+3): enough to see where a program hangs.
- `stats` reports the cycle counters since power-on: total and halted cycles,
  cycles taken by the LCD's DMA, serial flash reads and cache misses (revision M
  and later: an 8 KB cache, about 197 cycles per miss) and the flash wait cycles.
- `lcd-dma <0|1>` emulates the LCD's DMA, which takes RAM cycles from the CPU
  while the screen refreshes (off by default here), or turns it off.
- `line-profile on` counts, for each 32-byte line of flash, how many times the
  CPU entered it (a read after a read of another line) and how many of those
  missed the serial flash cache; `line-profile save <path>` writes the counters
  (2 per line: entries then misses, 32-bit, host byte order, 524288 lines for
  the 16 MB address space); `line-profile off` stops counting.
- `line-trace on <path>` writes every line entered, in order, as 32-bit line
  numbers (address / 32, host byte order), for cache simulations and code layout
  studies; `line-trace off` closes the file.
- `flash-cycles <n>` makes every serial flash read cost n cycles without the
  cache; 10 is close to a calculator made before revision M (parallel flash with
  the OS's 9 wait states), to compare a program's speed on older hardware with a
  revision M ROM. `flash-cycles 0` restores the cache.
- `help` lists commands.
- `quit` exits cleanly.

If the core was built with `-DDEBUG_CONSOLE_SUPPORT` (for example `make
CPPFLAGS=-DDEBUG_CONSOLE_SUPPORT` in `core/`), what the calculator's programs print
to the debug console (the toolchain's `dbg_printf`) is written to standard error, as
the core's diagnostics are.

For example:

```text
run 2000
key down
key enter
run 5000
screenshot /tmp/python.bmp
quit
```
