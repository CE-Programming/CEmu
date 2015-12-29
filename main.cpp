#include <QtWidgets/QApplication>
#include <QtQml/QtQml>

#include "mainwindow.h"
#include "qmlbridge.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("cemu-dev");
    QCoreApplication::setApplicationName("CEmu");
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    // Register QMLBridge for Keypad<->Emu communication
    qmlRegisterSingletonType<QMLBridge>("CE.emu", 1, 0, "Emu", qmlBridgeFactory);

    MainWindow EmuWin;
    EmuWin.show();

    return app.exec();
}
