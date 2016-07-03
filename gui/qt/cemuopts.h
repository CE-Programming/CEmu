#ifndef CEMUOPTS_H
#define CEMUOPTS_H
#include <QString>
struct CEmuOpts{
    bool restoreOnOpen;
    bool suppressTestDialog;
    QString RomFile;
    QString AutotesterFile;
};

#endif // CEMUOPTS_H
