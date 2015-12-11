#include <QtWidgets/QApplication>

#include "mainwindow.h"
#include "romselection.h"
#include "settings.h"

int main(int argc, char *argv[])
{
    QApplication z(argc, argv);

    MainWindow EmuWin;
    EmuWin.show();

    return z.exec();
}
