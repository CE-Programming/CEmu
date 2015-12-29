# CEmu
An open-source third-party TI-84 Plus CE / TI-83 Premium CE calculator emulator.  
With a core made in C and GUI in C++ with Qt, it works natively on many platforms. The core is even compilable with Emscripten, so as to be able to make a web-based version.

## Downloads
No binaries yet! When available, they'll be here, though: https://github.com/MateoConLechuga/CEmu/releases

## How to build
After downloading the source (you can clone the repo or just get the zip):

1. Get the [latest Qt5 SDK](https://www.qt.io/download-open-source/#section-3) for your OS.
2. Now you have two options:
  * Open the .pro file with Qt Creator, set it up (default project settings should be fine), and hit Build
  * In a shell, cd to the project folder and type `qmake -r CEmu.pro; make`


## TODO list
### Core
* Implement protected port range (0x9XXX)
* Implement unknown port range (0xDXXX)
* Implement timers
* Implement proper interrupt handling
* Implement USB
* ...

### Overall features
* Variable transfer (Calc <-> Computer)
* Disassembly view
* Breakpoints
* Step / Step over instructions in debugger
* ...

### GUI
* Window for changing keyboard equates
* HD Icon
* Debugger-related improvements
* Plan for 83PCE / 84+CE differences (2 keypad setups, "skins", etc.)
* More translations
* ...

## In the future...
* Provide more translations (for now, it's available in English and French). _If you want to help, tell us, or send patches / pull requests!_
* Make a web-based version of CEmu, like there’s a web-based version of z80e for trying out KnightOS. _Compiling the CEmu core to JavaScript (and later WebAssembly), using Emscripten, is already known to work_
* Think about CEmu's core’s integration on third-party projects, like TI-Planet's Project Builder - for instance, in C projects, in order to directly test the program, and eventually have live source-level debugging!
* Look at [url=https://github.com/legumbre/gdb-z80]this gdb-z80[/url] project (code from 2011...) ; try to see if it can be updated for eZ80, and used with a CEmu GDB stub. Mainlining such code is highly preferable.
* ...
