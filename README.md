# CEmu [![Build Status](https://travis-ci.org/CE-Programming/CEmu.svg)](https://travis-ci.org/CE-Programming/CEmu) [![Build Status](https://ci.appveyor.com/api/projects/status/github/CE-Programming/CEmu?branch=master&svg=true)](https://ci.appveyor.com/project/alberthdev/cemu-q0nl8) [![Build Status](https://scan.coverity.com/projects/7576/badge.svg)](https://scan.coverity.com/projects/ce-programming-cemu) [![IRC badge](https://img.shields.io/badge/IRC%20channel-%23cemu--dev%20on%20EFNet-blue.svg)](http://chat.efnet.org/irc.cgi?adv=1&nick=cemu-user&chan=%23cemu-dev)

CEmu is a third-party TI-84 Plus CE / TI-83 Premium CE calculator emulator, focused on developer features.  
The core is programmed in C and the GUI in C++ with Qt, for performance and portability reasons. CEmu works natively on Windows, macOS, and Linux.

Here are some screenshots showcasing some of the features:

Windows | macOS | Linux
------------ | ------------- | -------------
<a href="https://i.imgur.com/0GZRIck.png"><img src="https://i.imgur.com/xZBkG65.png" /></a>|<a href="https://i.imgur.com/LznBl5u.png"><img src="https://i.imgur.com/DvZb3Zx.png" /></a>|<a href="https://i.imgur.com/26sioCw.png"><img src="https://i.imgur.com/y4ObHtQ.png" /></a>

## Features
### _Standard features_
* Built-in ROM dump wizard. CEmu does _not_ rely on downloading an OS from TI's website nor does it have a custom boot/loader: a ROM from your own calculator is required. CEmu makes it very easy to get it.
* Accurate and fast emulation (you can also customize the speed and even toggle throttling)
* Resizable calculator screen
* "Always-on-top" window option
* Screen capture (PNG, GIF)
* Screen recording (animated GIF)
* File sending/receiving _(partial, WIP)_
* Multiple keybinding presets
* CE skins (colors like the real devices)

### _Developer features_
* Custom display refresh rate
* Custom emulation speed/throttling
* Code stepping, jumping...
* R/W/X breakpoints
* eZ80 disassembler (with equates support)
* Profiler (WIP-branch)
* Port monitor/editor
* Timers simple monitor/editor
* Memory viewer/editor
* CPU state/registers viewer/editor
* LCD state/parameters viewer/editor
* Memory visualizer (as fully customizable virtual LCDs)
* Stack viewer
* OP1-6 viewer
* VAT viewer
* Variable list with preview and program launcher
* Misc. emulation (backlight, battery...)
* "Autotester" (automated unit testing, light scripting)

## Downloads
The latest development builds are available here: https://ce-programming.github.io/CEmu/download

No binaries have been released yet as the code keeps changing these days! When available, they'll be here: https://github.com/CE-Programming/CEmu/releases   

_Note that Release builds have an update checking feature, which is disabled in development builds._

## How to build
After downloading the source (you can clone the repo or just [get the zip](https://github.com/CE-Programming/CEmu/archive/master.zip)):

1. Get the [latest Qt5 SDK](https://www.qt.io/download-open-source/#section-3) for your OS.  
  * If you're on linux, you may need to force update your PATH to have Qt's `bin/` folder prepended. This can be done by editing your shell's profile (for example ~/.bashrc), and adding the line:  
  `export PATH=<path to Qt directory>/bin:$PATH`
  * You may need to run this command under linux as well: `sudo apt-get install git qt5-default`

2. Now you have two options:
  * In a shell, `cd` to the project's `/gui/qt/` folder and type `qmake -r CEmu.pro && make`
  * Open the .pro file with Qt Creator, set it up (default project settings should be fine), and hit Build. *(Note: you can tell make to use -j4 in the project settings)*

3. If you are using linux, use `sudo make install` to integrate with your desktop.

_Note: Debugging support is somewhat core-related but is only built when `DEBUG_SUPPORT` is defined. The Qt GUI does this in the .pro file._  
_Note 2: If you encounter a build error with something like `lto-wrapper failed`, try removing the -flto option in the .pro file and rebuild (`qmake` etc.). We're not quite sure why this is happening._

You're welcome to [report any bugs](https://github.com/CE-Programming/CEmu/issues) you may encounter, and if you want to help, tell us, or send patches / pull requests!

If you'd like to contribute code, please consider using [Artistic Style](http://astyle.sourceforge.net/) with the settings specified in the `.astylerc` file to format your code. Qt Creator can [format code with Artistic Style](http://doc.qt.io/qtcreator/creator-beautifier.html) with minimal setup.  
We also welcome more translations (for now, it's available in English, French, and Spanish). Qt Linguist is a great tool for that (here's [a tutorial](https://doc.qt.io/qt-5/linguist-manager.html)).

## Caveats and TODO/WISH list
_Take a look at [the current issues](https://github.com/CE-Programming/CEmu/issues), since these things are organized there._

## License
CEmu is licensed under the [GPLv3](LICENSE).  
_Acknowledgements_: Some CEmu parts are, directly, modified, or inspired, from [z80e](https://github.com/KnightOS/z80e), [Firebird](https://github.com/nspire-emus/firebird), [QHexEdit2](https://github.com/Simsys/qhexedit2), [libtifiles](https://github.com/debrouxl/tilibs), [tivars_lib_cpp](https://github.com/adriweb/tivars_lib_cpp), [Gifsicle](https://github.com/kohler/gifsicle), and the [Silk iconset](http://www.famfamfam.com/lab/icons/silk/).  
The complete licensing information is available in the [LICENSE](LICENSE) file.
