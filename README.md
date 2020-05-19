# CEmu [![Build Status](https://api.travis-ci.org/CE-Programming/CEmu.svg?branch=master)](https://travis-ci.org/CE-Programming/CEmu) [![Build Status](https://ci.appveyor.com/api/projects/status/github/CE-Programming/CEmu?branch=master&svg=true)](https://ci.appveyor.com/project/alberthdev/cemu-q0nl8) [![Build Status](https://scan.coverity.com/projects/7576/badge.svg)](https://scan.coverity.com/projects/ce-programming-cemu) [![IRC badge](https://img.shields.io/badge/IRC%20channel-%23cemu--dev%20on%20EFNet-blue.svg)](http://chat.efnet.org/irc.cgi?adv=1&nick=cemu-user&chan=%23cemu-dev)

CEmu is a third-party TI-84 Plus CE / TI-83 Premium CE calculator emulator, focused on developer features.  
CEmu works natively on Windows, macOS, and Linux. For performance and portability, the core is programmed in C and its customizable GUI in C++ with Qt.

<a href="https://i.imgur.com/7QsJd5L.png"><img src="https://i.imgur.com/mTC6yXp.png" /></a>

_Note: CEmu is not a TI product nor is it TI-endorsed/affiliated. If you need an official TI CE emulator, TI-SmartView™ CE is for you._

## Downloads
Development builds are available here: https://ce-programming.github.io/CEmu/download

Official stable releases are available here: https://github.com/CE-Programming/CEmu/releases

_Note that Release builds have an update checking feature, which is disabled in development builds._

## Features
### _Standard features_
* Built-in ROM dump wizard. CEmu does _not_ rely on downloading an OS from TI's website nor does it have a custom boot/loader: a ROM from your own calculator is required. CEmu makes it very easy to get it.
* Accurate and fast emulation (you can also customize the speed and even toggle throttling)
* Resizable calculator screen
* "Always-on-top" window option
* Screen capture, copy, and drag'n'drop (PNG)
* Screen recording (animated PNG)
* USB emulation for transfers (including for Apps and OSes)
* Customizable keybindings with multiple presets
* Keypress recording and history
* Full screen modes (F11 key)
* Emulation states for efficient saving / restoring
* CE skins (colors like the real devices)
* Available in English, French, Spanish, and Dutch

### _Developer features_
* Main options available via CLI arguments
* IPC features when launching several CEmu processes
* Import/Export RAM, ROM, images...
* Custom display refresh rate, FPS indicator
* Custom emulation speed/throttling
* Rich text console for easier logging/debugging
* Code stepping, jumping...
* R/W/X breakpoints, watchpoints
* eZ80 disassembler (with equates support)
* Port monitor/editor
* General Timer monitor/editor
* Memory viewer/editor
* CPU state/registers viewer/editor
* LCD state/parameters viewer/editor
* Memory visualizer (as fully customizable virtual LCDs)
* Stack viewer
* OP1-7 viewer
* FP and OP stacks viewer/editor
* Variable Allocation Table (VAT) viewer
* Variable list with preview and program launcher
* TI-Basic program viewer with syntax-highlight and reformatting
* Recent files list with ability to quickly resend
* Cycle counter for benchmarking/profiling
* Emulation of DMA and SPI (for optimal accuracy)
* Misc. emulation (backlight, battery...)
* Pre-I HW Rev. emulation toggle (IM 2 compatibility)
* "Autotester" (automated unit testing, light scripting)

## How to build
You can find information for building CEmu yourself from the source on [this](https://github.com/CE-Programming/CEmu/wiki/Building-CEmu) wiki page. If you encounter any problems, feel free to open an [issue](https://github.com/CE-Programming/CEmu/issues)!

## Contributing

You're welcome to [report any bugs](https://github.com/CE-Programming/CEmu/issues) you may encounter, in addition to any [feature requests](https://github.com/CE-Programming/CEmu/issues) you may have. If you want to help, [tell us](http://chat.efnet.org/irc.cgi?adv=1&nick=cemu-user&chan=%23cemu-dev), or send patches / pull requests!

If you'd like to contribute code, please consider using [Artistic Style](http://astyle.sourceforge.net/) with the settings specified in the `.astylerc` file to format your code. Qt Creator can [format code with Artistic Style](http://doc.qt.io/qtcreator/creator-beautifier.html) with minimal setup.  
We also welcome more translations. Qt Linguist is a great tool for that (here's [a tutorial](https://doc.qt.io/qt-5/linguist-manager.html)).

## License
CEmu is licensed under the [GPLv3](LICENSE).  
_Acknowledgements_: Some CEmu parts are, directly, modified, or inspired, from [z80e](https://github.com/KnightOS/z80e), [Firebird](https://github.com/nspire-emus/firebird), [libtifiles](https://github.com/debrouxl/tilibs), [tivars_lib_cpp](https://github.com/adriweb/tivars_lib_cpp), [KDMacTouchBar](https://github.com/KDAB/KDMacTouchBar), and FatCow's ["Farm-Fresh Web Icons"](http://www.fatcow.com/free-icons).  
The complete licensing information is available in the [LICENSE](LICENSE) file.
