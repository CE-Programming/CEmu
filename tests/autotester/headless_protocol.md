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
