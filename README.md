# CEmu [![Build Status](https://travis-ci.org/MateoConLechuga/CEmu.svg)](https://travis-ci.org/MateoConLechuga/CEmu)
An open-source third-party TI-84 Plus CE / TI-83 Premium CE calculator emulator.  
With a core made in C and GUI in C++ with Qt, it works natively on many platforms. The core is even compilable with Emscripten, so as to be able to make a web-based version.

Here's what it looks like on Mac OS X:  
<a href="https://i.imgur.com/0BJsPoG.png"><img src="https://i.imgur.com/L9nuir2.png" /></div></a>

## License
CEmu is licensed under the [GPLv3](LICENSE).  
_Acknowledgements_: Some CEmu parts are [inspired] from the [z80e](https://github.com/KnightOS/z80e) (MIT license [here](https://github.com/KnightOS/z80e/blob/master/LICENSE)) and [Firebird](https://github.com/nspire-emus/firebird) (GPLv3 license [here](https://github.com/nspire-emus/firebird/blob/master/LICENSE)) projects.

## Downloads
No binaries yet! When available, they'll be here, though: https://github.com/MateoConLechuga/CEmu/releases

## How to build
After downloading the source (you can clone the repo or just get the zip):

1. Get the [latest Qt5 SDK](https://www.qt.io/download-open-source/#section-3) for your OS.
2. Now you have two options:
  * Open the .pro file with Qt Creator, set it up (default project settings should be fine), and hit Build
  * In a shell, cd to the project folder and type `qmake -r CEmu.pro; make`


## TODO list
* Implement unknown port range (0xDXXX)
* Implement USB (0xDXXX appears to be a part of this as well)
* Fix watchdog timer things
* Add more flash handlers
* Bug fixes
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
* Look at [this gdb-z80](https://github.com/legumbre/gdb-z80) project (code from 2011...) ; try to see if it can be updated for eZ80, and used with a CEmu GDB stub. Mainlining such code is highly preferable.
* ...
