# CEmu
An open-source third-party TI-84 Plus CE / TI-83 Premium CE calculator emulator.  
With a core made in C and GUI in C++ with Qt, it works natively on many platforms. The core is even compilable with Emscripten, so as to be able to make a web-based version.

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
