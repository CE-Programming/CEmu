# CEmu [![Build Status](https://travis-ci.org/CE-Programming/CEmu.svg)](https://travis-ci.org/CE-Programming/CEmu) [![Build Status](https://scan.coverity.com/projects/7576/badge.svg)](https://scan.coverity.com/projects/ce-programming-cemu)

CEmu is a third-party TI-84 Plus CE / TI-83 Premium CE calculator emulator, especially focused on developer features.  

It has a very customizable user interface to fit the needs of different use cases.  
Here are three setup examples of CEmu running on Mac OS X:

Minimalistic  | Bigger keypad and screen | With many debugging features
------------ | ------------- | -------------
<a href="https://i.imgur.com/yU8xOqf.png"><img src="https://i.imgur.com/wYlQPgu.png" /></a>|<a href="https://i.imgur.com/cKYRuxM.png"><img src="https://i.imgur.com/edxwq7K.png" /></a>|<a href="https://i.imgur.com/c90lBOq.png"><img src="https://i.imgur.com/7GDppPH.png" /></a>

With a core made in C and GUI in C++ with Qt, it works natively on many platforms.  
In fact, the core has even been succesfully [tested](https://github.com/MateoConLechuga/CEmu/tree/master/gui) _(proof-of-concepts)_ on web-browsers (JavaScript via Emscripten), the TI-Nspire CX (Ndless SDK), and the Apple Watch.

## Features
### _Standard features_
* Built-in ROM dumper wizard. CEmu does _not_ rely on downloading an OS from TI's website nor does it have a custom boot/loader, so a ROM from your own calculator is required. CEmu makes it very easy to get it. 
* Accurate and fast emulation (you can also customize the speed and even toggle throttling)
* Resizable calculator screen
* "Always-on-top" option
* Screen capture (PNG, GIF)
* Screen recording (animated GIF)
* File sending/receiving _(partial, WIP)_

### _Developer features_
* Custom display refresh rate
* Custom emulation speed/throttling
* Code stepping, jumping...
* R/W/X breakpoints
* eZ80 disassembler
* Port monitor/editor
* Memory viewer/editor
* CPU state/registers viewer/editor
* LCD state/config. viewer/editor
* Stack viewer
* OP1-6 viewer
* VAT viewer
* Misc. emulation (backlight, battery...)

## Downloads
No stable binaries yet as the code keeps changing these days! When available, they'll be here, though: https://github.com/MateoConLechuga/CEmu/releases  
Nightly win32 binaries available here (hosted by pimathbrainiac): http://pimathbrainiac.me/CEmu/  
_Note that Release builds have an update checking feature, which is disabled in development builds._

## How to build
After downloading the source (you can clone the repo or just [get the zip](https://github.com/MateoConLechuga/CEmu/archive/master.zip)):

1. Get the [latest Qt5 SDK](https://www.qt.io/download-open-source/#section-3) for your OS (and read on before downloading)
  * On Windows, the MinGW variation of the Qt SDK is preferred (v5.6+, too).  
    However, if you are building with Visual Studio, you must use Visual Studio 2015 or newer. You also must download a Qt build that is compatible with Visual Studio 2015 or newer.
    * If you don't have Visual Studio 2015 installed, we recommend
      installing [Visual Studio 2015 Community](https://go.microsoft.com/fwlink/?LinkId=691978&clcid=0x409).
    * Qt v5.6 is the only version of Qt (at the moment) that supports
      VS2015.
      * You can download the beta version of Qt v5.6
        [here](http://download.qt.io/development_releases/qt/5.6/5.6.0-beta/).
      * Direct download links for Qt v5.6 (VS2015):
        [MSVC 2015 x86](http://download.qt.io/development_releases/qt/5.6/5.6.0-beta/qt-opensource-windows-x86-msvc2015-5.6.0-beta.exe)
        and [MSVC 2015 x64](http://download.qt.io/development_releases/qt/5.6/5.6.0-beta/qt-opensource-windows-x86-msvc2015_64-5.6.0-beta.exe)
    * Note that the latest Qt v5.6 beta has a bug where the version of
      MSVC is incorrect. (It is displayed as 2013 or older.) This is
      simply a display bug. To ensure that you are using MSVC 2015,
      check the actual compiler version and ensure that it is 14.0
      or newer.
    
2. Now you have two options:
  * Open the .pro file with Qt Creator, set it up (default project settings should be fine), and hit Build
  * In a shell, cd to the project folder and type `qmake -r CEmu.pro; make`

_Note: Debugging support is somewhat core-related but is only built conditionally (since embedded targets probably won't need it). To enable it, define `DEBUG_SUPPORT`. The Qt GUI does this in the .pro file._

You're welcome to [report any bugs](https://github.com/MateoConLechuga/CEmu/issues) you may encounter, and if you want to help, tell us, or send patches / pull requests! If you'd like to contribute code, please consider using [Artistic Style](http://astyle.sourceforge.net/) with the settings specified in the `.astylerc` file to format your code. Qt Creator can [format code with Artistic Style](http://doc.qt.io/qtcreator/creator-beautifier.html) with minimal setup.


## TODO list
_(Take a look at [the current issues](https://github.com/CE-Programming/CEmu/issues))._

## WISH list
_(Take a look at [the issues](https://github.com/CE-Programming/CEmu/issues), too, since not everything here has been migrated there yet)_
* Make a web-based version of CEmu, like there’s a web-based version of z80e for trying out KnightOS. _Compiling the CEmu core to JavaScript (and later WebAssembly), using Emscripten, is already known to work_
* Think about CEmu's core’s integration on third-party projects, like TI-Planet's Project Builder - for instance, in C projects, in order to directly test the program, and eventually have live source-level debugging!
* Look at [this gdb-z80](https://github.com/legumbre/gdb-z80) project (code from 2011...) ; try to see if it can be updated for eZ80, and used with a CEmu GDB stub. Mainlining such code is highly preferable.
* More translations (for now, it's available in English, French, and Spanish). Qt Linguist is a great tool for that (here's [a tutorial](https://doc.qt.io/qt-5.5/linguist-manager.html)). We welcome Pull Requests :)
* ...

## License
CEmu is licensed under the [GPLv3](LICENSE).  
_Acknowledgements_: Some CEmu parts are, directly, modified, or inspired, from [z80e](https://github.com/KnightOS/z80e), [Firebird](https://github.com/nspire-emus/firebird), [QHexEdit2](https://github.com/Simsys/qhexedit2), [tivars_lib_cpp](https://github.com/adriweb/tivars_lib_cpp), and the [Silk iconset](http://www.famfamfam.com/lab/icons/silk/). The complete licensing information is available in the [LICENSE](LICENSE) file.
