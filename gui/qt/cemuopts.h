#ifndef CEMUOPTS_H
#define CEMUOPTS_H

#include <QtCore/QString>

struct CEmuOpts{
    bool restoreOnOpen;
    bool useUnthrottled;
    bool suppressTestDialog;
    QString RomFile;
    QString AutotesterFile;
};

#endif // CEMUOPTS_H
