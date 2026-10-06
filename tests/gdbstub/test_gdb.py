#!/usr/bin/env python3
"""Run the CE toolchain fixture through the real GDB remote client."""
import argparse
import os
from pathlib import Path
import socket
import subprocess
import tempfile


def run(args):
    with socket.socket() as temporary:
        temporary.bind(("127.0.0.1", 0))
        port = temporary.getsockname()[1]
    fixture = Path(args.fixture).resolve()
    with tempfile.TemporaryFile(mode="w+") as log:
        emulator = subprocess.Popen(
            [args.emulator, "--rom", args.rom], stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=log, text=True,
            env=dict(os.environ, CEMU_GDB_PORT=str(port)))
        def command(value):
            emulator.stdin.write(value + "\n")
            emulator.stdin.flush()
            result = emulator.stdout.readline().strip()
            assert result.startswith("OK"), result
        try:
            assert emulator.stdout.readline().startswith("CEMU_HEADLESS_READY")
            command("run 6000")
            command("key enter")
            command("key clear")
            command("send-file ram " + str(fixture / "GDBTEST.8xp"))
            # Let GDB attach and install main's breakpoint before the OS launches it.
            emulator.stdin.write("run-realtime 1000\nlaunch-asm GDBTEST\nrun-realtime 1000\n")
            emulator.stdin.flush()
            script = rf'''
set pagination off
set confirm off
set remotetimeout 5
file {fixture / 'GDBTEST.obj'}
target remote 127.0.0.1:{port}
break *_main
continue
if $pc != &_main
  echo FAIL main breakpoint\n
  quit 1
end
delete breakpoints
break *_probe_write
continue
if $hl != 0x123456
  echo FAIL HL\n
  quit 1
end
watch watched_value
continue
if $pc != &_probe_read
  echo FAIL write watchpoint PC\n
  quit 1
end
if watched_value != 0x123456
  echo FAIL write watchpoint value\n
  quit 1
end
delete breakpoints
rwatch watched_value
continue
if $pc != &_probe_after_read
  echo FAIL read watchpoint PC\n
  quit 1
end
if $de != 0x123456
  echo FAIL DE\n
  quit 1
end
delete breakpoints
set $pc = &_probe_read
awatch watched_value
continue
if $pc != &_probe_after_read
  echo FAIL read watchpoint PC\n
  quit 1
end
delete breakpoints
set $pc = &_probe
stepi
if $pc != &_probe_write
  echo FAIL stepi did not execute LD HL\n
  quit 1
end
set $pc = &_probe_after_read
set $return_pc = (unsigned long)*(unsigned char *)$sp | ((unsigned long)*(unsigned char *)($sp + 1) << 8) | ((unsigned long)*(unsigned char *)($sp + 2) << 16)
set $return_sp = $sp + 3
stepi
if $pc != $return_pc || $sp != $return_sp
  echo FAIL stepi did not execute RET\n
  quit 1
end
echo PASS real GDB: OS launch, symbols, breakpoints, write/read/access watchpoints, LD and RET stepi\n
detach
'''
            with tempfile.NamedTemporaryFile(mode="w", suffix=".gdb", delete=False) as file:
                file.write(script)
                script_path = file.name
            try:
                result = subprocess.run([args.gdb, "-nx", "-batch", "-x", script_path],
                                        text=True, capture_output=True, timeout=30)
            except subprocess.TimeoutExpired as error:
                print((error.stdout or b"").decode(errors="replace"))
                print((error.stderr or b"").decode(errors="replace"))
                raise
            finally:
                Path(script_path).unlink()
            print(result.stdout, end="")
            print(result.stderr, end="")
            assert result.returncode == 0, result.returncode
            assert "PASS real GDB:" in result.stdout
            emulator.stdin.write("screenshot " + str(Path(args.screenshot).resolve()) + "\nquit\n")
            emulator.stdin.flush()
            emulator.communicate(timeout=15)
            assert emulator.returncode == 0
            log.seek(0)
            diagnostics = log.read()
            assert "AddressSanitizer" not in diagnostics and "runtime error:" not in diagnostics, diagnostics
        except BaseException:
            log.seek(0)
            print(log.read()[-12000:])
            raise
        finally:
            if emulator.poll() is None:
                emulator.kill()
                emulator.wait()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--emulator", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--gdb", default="z80-none-elf-gdb")
    parser.add_argument("--fixture", default=str(Path(__file__).parent / "fixture/bin"))
    parser.add_argument("--screenshot", required=True)
    run(parser.parse_args())
