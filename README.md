# CEmu [![Build Windows](https://github.com/CE-Programming/CEmu/actions/workflows/build.windows.workflow.yml/badge.svg)](https://github.com/CE-Programming/CEmu/actions/workflows/build.windows.workflow.yml) [![Build macOS](https://github.com/CE-Programming/CEmu/actions/workflows/build.mac.workflow.yml/badge.svg)](https://github.com/CE-Programming/CEmu/actions/workflows/build.mac.workflow.yml) [![Build Linux](https://github.com/CE-Programming/CEmu/actions/workflows/build.linux.workflow.yml/badge.svg)](https://github.com/CE-Programming/CEmu/actions/workflows/build.linux.workflow.yml) [![Build Status](https://scan.coverity.com/projects/7576/badge.svg)](https://scan.coverity.com/projects/ce-programming-cemu) [![Discord Chat Link](https://img.shields.io/discord/432891584451706892?logo=discord)](https://discord.gg/CyUmEx9zmQ)

CEmu is a third-party TI-84 Plus CE / TI-83 Premium CE calculator emulator, focused on developer features.  
CEmu works natively on Windows, macOS, and Linux. For performance and portability, the core is programmed in C and its customizable GUI in C++ with Qt.

<a href="https://i.imgur.com/6lMva88.png"><img src="https://i.imgur.com/6lMva88.png" /></a>

_Note: CEmu is not a TI product nor is it TI-endorsed/affiliated. If you need an official TI CE emulator, TI-SmartView™ CE is for you._

## Downloads
Official stable releases are available here: https://github.com/CE-Programming/CEmu/releases  
Development builds are available on the auto-updating ["nightly" pre-release](https://github.com/CE-Programming/CEmu/releases/tag/nightly).

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
* USB emulation for flash drive images and external devices (via libusb)
* Customizable keybindings with multiple presets
* Keypress recording and history
* Full screen modes (F11 key)
* Emulation states for efficient saving / restoring
* CE skins (colors like the real devices)
* Available in English, French, Spanish, Dutch, and Chinese

### _Developer features_
* Main options available via CLI arguments
* IPC features when launching several CEmu processes
* Choice of ASIC / HW revision emulation (A, pre-I, M+...)
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
* Advanced LCD parameters emulation (gamma, response time, ...)
* Tracking of flash cache misses and average access time
* Memory visualizer (as fully customizable virtual LCDs)
* Stack viewer
* OP1-7 viewer
* FP and OP stacks viewer/editor
* Variable Allocation Table (VAT) viewer
* Variable list with preview and program launcher
* TI-Basic program viewer with syntax-highlight and reformatting
* TI-Basic program debugger with line-by-line stepping
* Recent files list with ability to quickly resend
* Cycle counter for benchmarking/profiling
* Emulation of DMA and SPI (for optimal accuracy)
* Misc. emulation (backlight, battery...)
* "Autotester" (automated unit testing, light scripting)

## How to build

You can find information for building CEmu yourself from the source on [this wiki page](https://github.com/CE-Programming/CEmu/wiki/Building-CEmu).  
If you encounter any problems, feel free to open an [issue](https://github.com/CE-Programming/CEmu/issues)!

## Getting Help

You can join us on the `CE Programming` Discord server in the `#cemu-emulator` channel: [Discord Chat Link](https://discord.gg/CyUmEx9zmQ).\
If you prefer IRC, the chat is also bridged to the EFNet `#cemu-dev` channel (you can use a web client like [IRCCloud](https://www.irccloud.com/irc/efnet/channel/cemu-dev), if you don't want to use a local application).

Depending on how active the channel is you might not get a response.\
In that case, post your questions [here](https://github.com/CE-Programming/CEmu/issues), and we will get back to you as soon as possible.

## Contributing

You're welcome to [report any bugs](https://github.com/CE-Programming/CEmu/issues) you may encounter, in addition to any [feature requests](https://github.com/CE-Programming/CEmu/issues) you may have. If you want to help, tell us, or send patches / pull requests!

If you'd like to contribute code, please consider using [Artistic Style](http://astyle.sourceforge.net/) with the settings specified in the `.astylerc` file to format your code. Qt Creator can [format code with Artistic Style](http://doc.qt.io/qtcreator/creator-beautifier.html) with minimal setup.  
We also welcome more translations. Qt Linguist is a great tool for that (here's [a tutorial](https://doc.qt.io/qt-5/linguist-manager.html)).

## License
CEmu is licensed under the [GPLv3](LICENSE).  
_Acknowledgements_: Some CEmu parts are, directly, modified, or inspired, from [z80e](https://github.com/KnightOS/z80e), [Firebird](https://github.com/nspire-emus/firebird), [libtifiles](https://github.com/debrouxl/tilibs), [tivars_lib_cpp](https://github.com/adriweb/tivars_lib_cpp), [KDMacTouchBar](https://github.com/KDAB/KDMacTouchBar), [TinyCThread](https://github.com/gyrovorbis/tinycthread) (_gyrovorbis_'s fork), and FatCow's ["Farm-Fresh Web Icons"](http://www.fatcow.com/free-icons).  
The complete licensing information is available in the [LICENSE](LICENSE) file.
