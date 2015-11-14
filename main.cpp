#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <climits>

#include "mainwindow.h"
#include "romselection.h"

int main(int argc, char *argv[])
{
    QApplication z(argc, argv);

    RomSelection a;
    a.exec();

    MainWindow EmuWin;
    EmuWin.show();

    return z.exec();
}
