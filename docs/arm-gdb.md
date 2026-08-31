# ARM coprocessor GDB server

CEmu can expose the SAMD21E18A coprocessor in Python-enabled calculator models
to a GDB client through the GDB Remote Serial Protocol. This is available when
the Qt build is compiled with `COPROC_DEBUG_SUPPORT`, which depends on CEmu's
general `DEBUG_SUPPORT`. Configuring CMake with
`-DCOPROC_DEBUG_SUPPORT=OFF` removes the server, command-line option, and
built-in debugger while leaving normal ARM emulation enabled.

Start the Qt build with a local TCP port:

```sh
CEmu --arm-gdb-port 3333
```

The server listens only on `127.0.0.1`. Connect with an ARM GDB build, loading
an ELF file from the same coprocessor firmware when symbols or source-level
debugging are needed:

```gdb
arm-none-eabi-gdb path/to/firmware.elf
(gdb) target remote 127.0.0.1:3333
```

Without a matching ELF file, registers, memory, instruction stepping, and
address breakpoints remain available, but GDB cannot show firmware symbols or
source lines.

The Qt build also includes a lightweight client under **Debug > ARM
Coprocessor Debugger**. If no port was selected on the command line, opening
the window starts the same local RSP server on an ephemeral port. The window
provides CPU register editing, run/halt/step/reset controls, virtual
breakpoints, a memory editor, and a searchable view of the modeled SAMD21
peripheral registers. It deliberately communicates through RSP rather than
accessing the ARM core directly, so it exercises the same debugger interface
as an external GDB client.

Only one client can be connected at a time. Detach an external GDB session
before opening the built-in debugger, or close CEmu to disconnect the built-in
client before attaching externally.

The server supports the Cortex-M register set (`r0` through `r15` and `xpsr`),
memory reads and writes, continue, single-step, Ctrl-C interrupt, and virtual
software breakpoints. Virtual breakpoints do not modify the emulated flash and
up to 64 may be active at once.

Stopping the remote target halts only the ARM coprocessor. The calculator's
eZ80 core and UI continue running, which matches the two-processor hardware but
can let the calculator-side Python app time out while the coprocessor is paused.
Detach GDB or continue the target to resume normal coprocessor execution.

The following monitor commands are available:

```gdb
(gdb) monitor help
(gdb) monitor info
(gdb) monitor reset
```

`monitor info` reports the emulated device, detected ARM bootloader identity,
and ARM cycle count. `monitor reset` resets and halts the coprocessor.
