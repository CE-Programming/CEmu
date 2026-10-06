# GDB remote debugging

Build CEmu with `DEBUG_SUPPORT` (enabled in the Qt frontend), then set
`CEMU_GDB_PORT` before starting it:

```sh
CEMU_GDB_PORT=1234 ./CEmu
```

The server is disabled when the variable is unset. It listens on IPv4 loopback
only and accepts one debugger at a time. Use the CE toolchain's GDB and the
uncompressed program's ELF object, built with debug information:

```sh
make debug
z80-none-elf-gdb bin/PROGRAM.obj
```

```text
(gdb) target remote 127.0.0.1:1234
(gdb) break main
(gdb) continue
```

Install the `.8xp` on the emulated calculator and launch it normally. A pre-5.5
OS can run it directly with `Asm(prgmPROGRAM`. The ELF's linked addresses must
match where the OS loads the program. Breakpoints can be installed before the
program is loaded; CEmu does not replace the target's instruction bytes.

The server supports register and memory inspection/modification, execution
breakpoints, write/read/access watchpoints, `continue`, `stepi`, Ctrl-C, and
`detach`. GDB resumes the emulator when detaching or disconnecting. Its
breakpoints and watchpoints are removed without removing the GUI debugger's
breakpoints, watchpoints, or hit counters. Overlapping GDB watchpoints and
repeated insertion/removal are supported, with up to 256 distinct entries.

Data watchpoints stop after the accessing instruction has finished. Repeating
block instructions stop after the matching iteration. A watchpoint stop reports
the beginning of the watched range, even when a later byte triggered it.
Debugger memory reads use CEmu's non-destructive peek interface.

The target description is `ez80-adl`: GDB uses ADL disassembly and stack unwinding.
It does not automatically switch its architecture when the CPU enters Z80 mode.
The register packet uses GDB's 14-register eZ80 layout, with three bytes per
register. `af` contains AF with zero padding; `sp` is SPL and `sps` is the
independent short-mode stack pointer. MBASE is not packed into AF. Signal resume
packets are accepted, but signals are not delivered to the emulated calculator.
The network server is unavailable in Emscripten builds.

Older CE toolchain GDB builds, including `17.0.50.20251110-git`, misorder the
return-address bytes when software-stepping a 24-bit `RET`. For example, stack
bytes `57 a9 d1` produce a breakpoint at `0xA957D1` instead of `0xD1A957`.
Use a GDB build with the `z80_software_single_step` return-address fix for the
complete integration test below. Native RSP `s`/`vCont;s` also cover `RET`
independently of GDB's software stepper.

## Regression tests

The state and GDB/GUI watchpoint overlap tests require neither a ROM nor the CE
toolchain:

```sh
cmake -S tests/gdbstub -B /tmp/cemu-gdb-tests -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/cemu-gdb-tests -j8
ctest --test-dir /tmp/cemu-gdb-tests --output-on-failure
```

The RSP/CPU test requires a supplied CE ROM. It executes a small deterministic
ADL instruction sequence in RAM and tests negotiation, validation, register and
memory writes, stepping, all watchpoint types, block instructions, interrupts,
detach, and reconnect:

```sh
python3 tests/gdbstub/test_rsp.py \
  --emulator /tmp/cemu-gdb-tests/cemu-gdb-headless --rom /path/to/CE.rom
```

For the complete toolchain/OS/GDB path, build the standalone fixture and run it
on an OS that permits ASM execution. The fixture requires no shared libraries:

```sh
make -C tests/gdbstub/fixture debug
python3 tests/gdbstub/test_gdb.py \
  --emulator /tmp/cemu-gdb-tests/cemu-gdb-headless --rom /path/to/CE.rom \
  --gdb z80-none-elf-gdb --screenshot /tmp/cemu-gdb-result.bmp
```

The test transfers and launches `GDBTEST`, loads its ELF symbols, verifies
breakpoints and all three watchpoint types through real GDB, executes `stepi`
on both a linear instruction and a 24-bit `RET`, checks 24-bit CFI unwinding,
and captures the final calculator screen. The headless `launch-asm` command
reuses the autotester's OS keycode launch path.

The nested C fixture checks DWARF 5 indexed addresses and source breakpoints,
24-bit arguments and locals, a three-frame backtrace, `finish`, natural
24/32-bit returns, and a forced 32-bit return. It requires the corresponding
DWARF, prologue, and return-value fixes in `binutils-2_47-cedev` and the DWARF
register/location fixes in the Clang 22 `z80` branch:

```sh
make -C tests/gdbstub/fixture_nested debug
python3 tests/gdbstub/test_gdb_nested.py \
  --emulator /tmp/cemu-gdb-tests/cemu-gdb-headless --rom /path/to/CE.rom \
  --gdb z80-none-elf-gdb --screenshot /tmp/cemu-gdb-nested.bmp
```

Clang 22 describes stack locals relative to the actual IX/IY frame base. The
nested test checks caller arguments and locals while inside the callee and
immediately after `finish`, before the caller cleans up its pushed argument.

The mixed fixture keeps an IX caller at `-O0`, with separate leaves at `-O2`
that use an unsaved IY frame and a frameless register-held variable. It tests
caller PC/SP/IX, caller locals, IY setup instruction boundaries, the final
`RET` after IY teardown, register locations, and return values:

```sh
make -C tests/gdbstub/fixture_frames debug
python3 tests/gdbstub/test_gdb_frames.py \
  --emulator /tmp/cemu-gdb-tests/cemu-gdb-headless --rom /path/to/CE.rom \
  --gdb z80-none-elf-gdb --screenshot /tmp/cemu-gdb-frames.bmp
python3 tests/gdbstub/test_gdb_frames.py \
  --emulator /tmp/cemu-gdb-tests/cemu-gdb-headless --rom /path/to/CE.rom \
  --gdb z80-none-elf-gdb --screenshot /tmp/cemu-gdb-frames-fallback.bmp \
  --without-cfi --objcopy z80-none-elf-objcopy
```

Clang emits CFI for GAS debug builds; the updated assembler and linker retain
16/24-bit address sizes, and GDB uses these rules before its prologue fallback.
The second run removes `.debug_frame` and `.eh_frame` from a temporary ELF copy
to exercise that fallback. IX is preserved by the compiler ABI; IY is
caller-clobbered, so an IY frame carries the caller IX unchanged and switches
its CFA to SP before popping IY. Optimized variables can still be unavailable:
this fixture checks the optimized IY local in raw memory and the frameless
variable through its register location.

Enable ASan/UBSan with `-DGDBSTUB_SANITIZERS=ON`. ROM initialization currently
triggers an unrelated signed-shift report in `core/flash.c:flash_set_mask`.
For the ROM tests, the checked-in suppression excludes only that function:

```sh
export UBSAN_OPTIONS="suppressions=$PWD/tests/gdbstub/ubsan.supp"
```

The regression scripts use local TCP sockets, which require loopback networking
permission in a sandbox. No ROM is included in the repository.
