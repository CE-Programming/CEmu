# CEmu [![Build Status](https://travis-ci.org/CE-Programming/CEmu.svg)](https://travis-ci.org/CE-Programming/CEmu) [![Build Status](https://scan.coverity.com/projects/7576/badge.svg)](https://scan.coverity.com/projects/ce-programming-cemu) [![IRC badge](https://img.shields.io/badge/IRC%20channel-%23cemu--dev%20on%20EFNet-blue.svg)](http://chat.efnet.org/irc.cgi?adv=1&nick=cemu-user&chan=%23cemu-dev)

CEmu is a third-party TI-84 Plus CE / TI-83 Premium CE calculator emulator, especially focused on developer features.  
The core is made in C and the GUI in C++ with Qt, for performance and portability reasons. CEmu works as-is on Windows, macOS, and Linux.

CEmu has a very customizable user interface to fit the needs of different use cases.  
Here are three setup examples of CEmu running on macOS:

Minimalistic  | Bigger keypad and screen | With many debugging features
------------ | ------------- | -------------
<a href="https://i.imgur.com/yU8xOqf.png"><img src="https://i.imgur.com/wYlQPgu.png" /></a>|<a href="https://i.imgur.com/cKYRuxM.png"><img src="https://i.imgur.com/edxwq7K.png" /></a>|<a href="https://i.imgur.com/c90lBOq.png"><img src="https://i.imgur.com/7GDppPH.png" /></a>

## Features
### _Standard features_
* Built-in ROM dump wizard. CEmu does _not_ rely on downloading an OS from TI's website nor does it have a custom boot/loader, so a ROM from your own calculator is required. CEmu makes it very easy to get it. 
* Accurate and fast emulation (you can also customize the speed and even toggle throttling)
* Resizable calculator screen
* "Always-on-top" option
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
* eZ80 disassembler
* Port monitor/editor
* Memory viewer/editor
* CPU state/registers viewer/editor
* LCD state/parameters viewer/editor
* Multiple virtual LCDs with custom parameters
* Stack viewer
* OP1-6 viewer
* VAT viewer
* Variable list with preview and program launcher
* Misc. emulation (backlight, battery...)
* "Autotester" (automated unit testing, light scripting)

## Downloads
No stable binaries yet as the code keeps changing these days! When available, they'll be here, though: https://github.com/CE-Programming/CEmu/releases  
Recent Windows binaries [available here](https://oss.jfrog.org/artifactory/oss-snapshot-local/org/github/alberthdev/cemu/git/) (credits to alberthro).  
_Note that Release builds have an update checking feature, which is disabled in development builds._

## How to build
After downloading the source (you can clone the repo or just [get the zip](https://github.com/CE-Programming/CEmu/archive/master.zip)):

1. Get the [latest Qt5 SDK](https://www.qt.io/download-open-source/#section-3) for your OS *(Note: on Windows, the MinGW variation of the Qt SDK is preferred)*.  
If you're on Ubuntu, you may need to run this command afterwards, too: `sudo apt-get install git qt5-default`

2. Now you have two options:
  * Open the .pro file with Qt Creator, set it up (default project settings should be fine), and hit Build. *(Note: you can tell make to use -j4 in the project settings)*
  * In a shell, `cd` to the project's *./gui/qt/* folder and type `qmake -r CEmu.pro; make`

_Note: Debugging support is somewhat core-related but is only built conditionally (since embedded targets probably won't need it). To enable it, define `DEBUG_SUPPORT`. The Qt GUI does this in the .pro file._  
_Note 2: If you encounter a build error with something like `lto-wrapper failed`, try removing the -flto option in the .pro file and rebuild (`qmake` etc.). We're not quite sure why this is happening._

You're welcome to [report any bugs](https://github.com/CE-Programming/CEmu/issues) you may encounter, and if you want to help, tell us, or send patches / pull requests!

If you'd like to contribute code, please consider using [Artistic Style](http://astyle.sourceforge.net/) with the settings specified in the `.astylerc` file to format your code. Qt Creator can [format code with Artistic Style](http://doc.qt.io/qtcreator/creator-beautifier.html) with minimal setup.  
We also welcome more translations (for now, it's available in English, French, and Spanish). Qt Linguist is a great tool for that (here's [a tutorial](https://doc.qt.io/qt-5/linguist-manager.html)).


## TODO and WISH list
_Take a look at [the current issues](https://github.com/CE-Programming/CEmu/issues), since these things are organized there._

## License
CEmu is licensed under the [GPLv3](LICENSE).  
_Acknowledgements_: Some CEmu parts are, directly, modified, or inspired, from [z80e](https://github.com/KnightOS/z80e), [Firebird](https://github.com/nspire-emus/firebird), [QHexEdit2](https://github.com/Simsys/qhexedit2), [libtifiles](https://github.com/debrouxl/tilibs), [tivars_lib_cpp](https://github.com/adriweb/tivars_lib_cpp), [Gifsicle](https://github.com/kohler/gifsicle), and the [Silk iconset](http://www.famfamfam.com/lab/icons/silk/).  
The complete licensing information is available in the [LICENSE](LICENSE) file.
