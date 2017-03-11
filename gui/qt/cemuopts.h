#ifndef CEMUOPTS_H
#define CEMUOPTS_H

#include <QtCore/QString>

struct CEmuOpts {
    int speed;
    bool restoreOnOpen;
    bool useUnthrottled;
    bool suppressTestDialog;
    bool deforceReset;
    bool forceReloadRom;
    QString romFile;
    QString autotesterFile;
    QString settingsFile;
    QString imageFile;
    QString debugFile;
    QString idString;
    QString pidString;
    QStringList sendFiles;
    QStringList sendArchFiles;
    QStringList sendRAMFiles;
};

#endif
