# CEmu
An open-source third-party TI-84 Plus CE / TI-83 Premium CE calculator emulator.  
With a core made in C and GUI in C++ with Qt, it works natively on many platforms.

## TODO list
### Core
* Implement protected port range (0x9XXX)
* Implement unknown port range (0xDXXX)
* Implement timers
* Implement proper interrupt handling
* Implement USB
* ...

### Overall features
* GIF output
* Variable transfer (Calc->Computer)
* ROM dump tool
* Disassembly view
* Breakpoints
* Step / Step over instructions in debugger
* Port monitor
* ...

### GUI
* Window for changing keyboard equates
* Fix icon things (mainwindow one -> use in aboutwindow, etc.)
* Add brightness scale
* Debugger GUI improvements
* Debugger<->Emu interface
* Plan for 83PCE / 84+CE differences (2 keypad setups, etc.)
* ...
