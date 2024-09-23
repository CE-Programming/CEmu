# zdis [![Build Status](https://travis-ci.org/CE-Programming/zdis.svg)](https://travis-ci.org/CE-Programming/zdis)

zdis is a tiny but powerful eZ80 disassembler, programmed in C.
It's used in [CEmu](https://github.com/CE-Programming/CEmu/blob/2384d30bffe3d341cf08c9fafea327753b26f5b5/gui/qt/debugger/disasm.cpp), for instance.

You need a compiler that supports C11 in order to build zdis (otherwise, just remove the `static_assert` usages).
Also add the -DDEBUG_SUPPORT or similar as a compile option as needed.

See [zdis.h](https://github.com/CE-Programming/zdis/blob/master/zdis.h) for documentation and [test.c](https://github.com/CE-Programming/zdis/blob/master/test.c) for an example of usage.
