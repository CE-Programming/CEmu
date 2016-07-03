#ifndef CEMUOPTS_H
#define CEMUOPTS_H
#include "QString"
struct CEMUOpts{
    bool restoreOnOpen;
    bool suppressTestDialog;
    QString RomFile;
    QString TestFile;
};

#endif // CEMUOPTS_H
