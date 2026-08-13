# CEmu headless control protocol

`cemu-headless` runs the emulator core without Qt and accepts line-oriented
commands on standard input. Each command produces exactly one `OK` or `ERR`
response on standard output. Core diagnostics are written to standard error.

Build from the repository root with:

```sh
make -C tests/autotester -j4 cemu-headless
```

For core debugging and calculator software-console output (for example,
CEdev's `dbg_printf`), initialize the `core/debug/zdis` submodule and build with:

```sh
git submodule update --init core/debug/zdis
make -C tests/autotester -j4 cemu-headless DEBUG_SUPPORT=1
```

The Makefile builds the core library automatically. `DEBUG_SUPPORT` defaults
to `0` and also applies to the `autotester` target. It controls emulator debugger
features; optimization and compiler debug symbols still follow `CFLAGS` and
`CXXFLAGS`. The headless runner enables software debug commands and sends their
console output to standard error, preserving the control protocol on standard
output.

Clean both the core and frontend before changing `DEBUG_SUPPORT`:

```sh
make -C core clean
make -C tests/autotester clean
```

Start it with either a ROM or a saved CEmu image.

You can also pass the python models' SAMD21 flash image:

```sh
./cemu-headless --image state.ce --arm-rom samd21-flash.bin
```

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
- `help` lists commands.
- `quit` exits cleanly.

For example:

```text
run 2000
key down
key enter
run 5000
screenshot /tmp/python.bmp
quit
```
