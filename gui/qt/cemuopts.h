#ifndef CEMUOPTS_H
#define CEMUOPTS_H

#include <QtCore/QString>
#include <QtCore/QStringList>

struct CEmuOpts {
    int speed;
    int fullscreen;
    bool restoreOnOpen;
    bool useUnthrottled;
    bool suppressTestDialog;
    bool deforceReset;
    bool forceReloadRom;
    bool useSettings;
    QString romFile;
    QString autotesterFile;
    QString settingsFile;
    QString imageFile;
    QString launchPrgm;
    QString debugFile;
    QString idString;
    QString pidString;
    QStringList sendFiles;
    QStringList sendArchFiles;
    QStringList sendRAMFiles;
};

#endif
