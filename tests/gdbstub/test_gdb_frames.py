#!/usr/bin/env python3
"""Check compiler-generated IX, IY and frameless unwinding with and without CFI."""
import argparse
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

from test_gdb import run


def function_size(path, name):
    """Read the linked ELF symbol's size to find this fixture's final RET."""
    data = path.read_bytes()
    assert data[:6] == b"\x7fELF\x01\x01", "expected little-endian ELF32"
    header = struct.unpack_from("<16sHHIIIIIHHHHHH", data)
    sections = [struct.unpack_from("<10I", data, header[6] + i * header[11])
                for i in range(header[12])]
    for section in sections:
        if section[1] != 2:  # SHT_SYMTAB
            continue
        strings = sections[section[6]]
        names = data[strings[4]:strings[4] + strings[5]]
        for offset in range(section[4], section[4] + section[5], section[9]):
            symbol = struct.unpack_from("<IIIBBH", data, offset)
            end = names.find(b"\0", symbol[0])
            if names[symbol[0]:end].decode() == name:
                code = sections[symbol[5]]
                last = code[4] + symbol[1] - code[3] + symbol[2] - 1
                assert symbol[2] > 0 and data[last] == 0xc9, "expected final RET"
                return symbol[2]
    raise AssertionError(f"missing function {name}")


def gdb_script(fixture, port):
    iy_size = function_size(fixture / "GDBFRAME.obj", "_iy_leaf")
    checks = r'''
frame 1
if $pc != $outer_pc || $ix != $outer_ix || $sp != $entry_sp + 3
  echo FAIL IY caller registers\n
  quit 1
end
if saved != 0x2468b3 || value != 0x2468ac
  echo FAIL IX caller locals while inside IY frame\n
  quit 1
end
frame 0
'''
    boundaries = checks + ("stepi\n" + checks) * 3
    return rf'''
set pagination off
set confirm off
set remotetimeout 5
file {fixture / 'GDBFRAME.obj'}
target remote 127.0.0.1:{port}
break *_main
continue
delete breakpoints
break *_iy_leaf
continue
if *(unsigned char *)&_iy_leaf != 0xfd || *(unsigned char *)((unsigned long)&_iy_leaf + 1) != 0x21
  echo FAIL fixture did not use IY setup\n
  quit 1
end
set $outer_ix = $ix
set $entry_sp = $sp
set $outer_pc = (unsigned long)*(unsigned char *)$sp | ((unsigned long)*(unsigned char *)($sp + 1) << 8) | ((unsigned long)*(unsigned char *)($sp + 2) << 16)
{boundaries}
break *_iy_frame_checkpoint
break *((unsigned long)&_iy_leaf + {iy_size - 1})
continue
if $iy != $entry_sp || $ix != $outer_ix
  echo FAIL IY base or preserved IX\n
  quit 1
end
if ((unsigned long)*(unsigned char *)($iy - 3) | ((unsigned long)*(unsigned char *)($iy - 2) << 8) | ((unsigned long)*(unsigned char *)($iy - 1) << 16)) != 0x373f28
  echo FAIL IY local memory\n
  quit 1
end
backtrace 3
{checks}
continue
if $sp != $entry_sp
  echo FAIL IY epilogue SP\n
  quit 1
end
{checks}
finish
if $ != 0x373f2b
  echo FAIL IY return value\n
  quit 1
end
delete breakpoints
break *_reg_frame_checkpoint
continue
if value != 0x314159
  echo FAIL register-held C variable\n
  quit 1
end
if $ix != $outer_ix
  echo FAIL frameless function changed IX\n
  quit 1
end
set $reg_sp = $sp
set $reg_pc = (unsigned long)*(unsigned char *)$sp | ((unsigned long)*(unsigned char *)($sp + 1) << 8) | ((unsigned long)*(unsigned char *)($sp + 2) << 16)
frame 1
if $pc != $reg_pc || $sp != $reg_sp + 3 || $ix != $outer_ix
  echo FAIL frameless caller registers\n
  quit 1
end
if saved != 0x2468b3 || value != 0x2468ac || first != 0x373f2b
  echo FAIL caller locals while inside frameless function\n
  quit 1
end
frame 0
finish
if $ != 0x9a8cb6
  echo FAIL frameless return value\n
  quit 1
end
finish
if $ != 0xf63494
  echo FAIL outer return value\n
  quit 1
end
delete breakpoints
echo PASS real GDB: IX/IY/frameless calls, partial IY prologues and epilogue, caller locals during calls and register-held variables\n
detach
'''


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--emulator", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--gdb", default="z80-none-elf-gdb")
    parser.add_argument("--fixture", default=str(Path(__file__).parent / "fixture_frames/bin"))
    parser.add_argument("--screenshot", required=True)
    parser.add_argument("--without-cfi", action="store_true")
    parser.add_argument("--objcopy", default="z80-none-elf-objcopy")
    args = parser.parse_args()
    if args.without_cfi:
        with tempfile.TemporaryDirectory(prefix="cemu-no-cfi-") as directory:
            source = Path(args.fixture).resolve()
            target = Path(directory)
            shutil.copy2(source / "GDBFRAME.8xp", target)
            subprocess.run([args.objcopy, "--remove-section=.debug_frame",
                            "--remove-section=.eh_frame", str(source / "GDBFRAME.obj"),
                            str(target / "GDBFRAME.obj")], check=True)
            args.fixture = directory
            run(args, program="GDBFRAME", script_factory=gdb_script)
    else:
        run(args, program="GDBFRAME", script_factory=gdb_script)
