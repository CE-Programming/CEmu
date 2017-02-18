#ifndef CEMUOPTS_H
#define CEMUOPTS_H

#include <QtCore/QString>

struct CEmuOpts {
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
    qint64 pid;
};

#endif
