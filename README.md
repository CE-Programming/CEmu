# CEmu
An open-source third-party TI-84 Plus CE / TI-83 Premium CE calculator emulator.  
With a core made in C and GUI in C++ with Qt, it works natively on many platforms.

## TODO list
### Core
* Implement SHA256 chip
* Implement protected port range (0x9XXX)
* Implement unknown port range (0xDXXX)
* Implement timers
* Implement proper interrupt handling
* Implement Real Time Clock
* Implement USB
* Fix watchdog timer
* Fix archiving/deleting from mem screen
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
* Switch from tabs to docks
* Better keypad GUI
* Fix ROM selection screen input
* Window for changing keyboard equates
* Fix icon things (mainwindow one -> use in aboutwindow, etc.)
* Add brightness scale
* Debugger GUI improvements
* Debugger<->Emu interface
* Fix bug where LCD data is drawn upside down on keypad redraw
* Plan for 83PCE / 84+CE differences (2 keypad setups, etc.)
* ...