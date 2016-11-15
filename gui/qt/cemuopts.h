#ifndef CEMUOPTS_H
#define CEMUOPTS_H

#include <QtCore/QString>

struct CEmuOpts{
    bool restoreOnOpen;
    bool useUnthrottled;
    bool suppressTestDialog;
    QString romFile;
    QString autotesterFile;
    QStringList sendFiles;
    QStringList sendArchFiles;
    QStringList sendRAMFiles;
};

#endif // CEMUOPTS_H
