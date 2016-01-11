#include <QtWidgets/QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("cemu-dev"));
    QCoreApplication::setApplicationName(QStringLiteral("CEmu"));
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    MainWindow EmuWin;
    EmuWin.show();

    return app.exec();
}
