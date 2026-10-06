#!/usr/bin/env python3
"""Check DWARF 5, nested C unwinding, locals, and scalar returns on a CE."""
import argparse
from pathlib import Path

from test_gdb import run


def gdb_script(fixture, port):
    return rf'''
set pagination off
set confirm off
set remotetimeout 5
file {fixture / 'GDBNEST.obj'}
target remote 127.0.0.1:{port}
break *_main
continue
delete breakpoints
break inner
break src/main.c:7
continue
if value != 0x2468b3
  echo FAIL C argument value\n
  quit 1
end
continue
if nested_value != 0x373f28
  echo FAIL C global value\n
  quit 1
end
set $caller_ix = (unsigned long)*(unsigned char *)$ix | ((unsigned long)*(unsigned char *)($ix + 1) << 8) | ((unsigned long)*(unsigned char *)($ix + 2) << 16)
set $caller_pc = (unsigned long)*(unsigned char *)($ix + 3) | ((unsigned long)*(unsigned char *)($ix + 4) << 8) | ((unsigned long)*(unsigned char *)($ix + 5) << 16)
set $main_pc = (unsigned long)*(unsigned char *)($caller_ix + 3) | ((unsigned long)*(unsigned char *)($caller_ix + 4) << 8) | ((unsigned long)*(unsigned char *)($caller_ix + 5) << 16)
backtrace 3
frame 1
if $pc != $caller_pc || $ix != $caller_ix
  echo FAIL C outer frame\n
  quit 1
end
if saved != 0x2468b3 || value != 0x2468ac
  echo FAIL C caller locals while inside inner\n
  quit 1
end
frame 2
if $pc != $main_pc
  echo FAIL C main frame\n
  quit 1
end
frame 0
delete breakpoints
finish
if $ != 0x373f2b || $hl != 0x373f2b
  echo FAIL C inner return value\n
  quit 1
end
if saved != 0x2468b3 || value != 0x2468ac
  echo FAIL C caller locals before argument cleanup\n
  quit 1
end
finish
if $ != 0x5ba7de || $hl != 0x5ba7de
  echo FAIL C outer return value\n
  quit 1
end
break wide_return
continue
if value != 0x12345678
  echo FAIL 32-bit argument\n
  quit 1
end
finish
if $ != 0x9b9f9b97 || $hl != 0x9f9b97 || ((unsigned long)$de & 0xff) != 0x9b
  echo FAIL 32-bit return\n
  quit 1
end
continue
return (unsigned long)0x89abcdef
if $hl != 0xabcdef || ((unsigned long)$de & 0xff) != 0x89
  echo FAIL forced 32-bit return\n
  quit 1
end
delete breakpoints
echo PASS real GDB: nested C arguments, global and caller locals during calls, three-frame backtrace, finish, 24/32-bit and forced return values\n
detach
'''


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--emulator", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--gdb", default="z80-none-elf-gdb")
    parser.add_argument("--fixture", default=str(Path(__file__).parent / "fixture_nested/bin"))
    parser.add_argument("--screenshot", required=True)
    run(parser.parse_args(), program="GDBNEST", script_factory=gdb_script)
