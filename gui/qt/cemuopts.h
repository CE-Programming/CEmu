#ifndef CEMUOPTS_H
#define CEMUOPTS_H

#include <QtCore/QString>
#include <QtCore/QStringList>
#ifdef COPROC_DEBUG_SUPPORT
# include <QtCore/QtTypes>
#endif

struct CEmuOpts {
    int speed;
    int fullscreen;
#ifdef COPROC_DEBUG_SUPPORT
    quint16 armGdbPort;
#endif
    bool restoreOnOpen;
    bool useUnthrottled;
    bool suppressTestDialog;
    bool deforceReset;
    bool forceReloadRom;
    bool useSettings;
    bool reset;
    bool ipcOnly;
    QString romFile;
    QString autotesterFile;
    QString settingsFile;
    QString imageFile;
    QString launchPrgm;
    QString debugFile;
    QString screenshotFile;
    QString keySequence;
    QString usbDevice;
    QString idString;
    QString pidString;
    QStringList sendFiles;
    QStringList sendArchFiles;
    QStringList sendRAMFiles;
    QStringList luaScripts;
};

#endif
