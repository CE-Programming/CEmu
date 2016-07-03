#include <QtWidgets/QApplication>
#include <QtQml/QtQml>
#include "mainwindow.h"
#include "qmlbridge.h"
#include "cemuopts.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Setup QCommandParser with Our command line parameters
    QCommandLineParser parser;
    parser.setApplicationDescription("Cemu CE emulator");
    parser.addHelpOption();
    parser.addVersionOption();
    //Disable Loading a Saved State and disables Saving to one on application Close
    QCommandLineOption stateOption(QStringList() << "s" << "no-state",
                QCoreApplication::translate("main", "Do Not load state from memory."));
    parser.addOption(stateOption);
    // "Loads Rom file
    //#TODO# "Not functional, No interface to load a rom from a file."
    QCommandLineOption loadRomFile(QStringList() << "r" << "rom",
                QCoreApplication::translate("main", "Load <RomFile> on sartup !BROKEN!"),
                QCoreApplication::translate("main", "RomFile"));
    parser.addOption(loadRomFile);
    //Loads a json into the autotester and runs on application startup
    QCommandLineOption loadTestFile(QStringList() << "t" << "auto-test",
                QCoreApplication::translate("main", "run <Testfile> on startup"),
                QCoreApplication::translate("main", "TestFile"));
    parser.addOption(loadTestFile);
    QCommandLineOption suppressTestDialog(QStringList() << "suppress-test-dialog",
                QCoreApplication::translate("main", "Hides test complete dialog"));
    parser.addOption(suppressTestDialog);
    parser.process(app);
    //Take commandline args and move to CEMUOpts Struct
    CEmuOpts opts;
    opts.restoreOnOpen = parser.isSet(stateOption)?false:true;
    opts.suppressTestDialog = parser.isSet(suppressTestDialog);
    if (parser.isSet(loadTestFile)){
        opts.AutotesterFile =  QDir::currentPath() + "/" +parser.value(loadTestFile);
    }
    opts.RomFile = parser.value(loadRomFile);

    QCoreApplication::setOrganizationName(QStringLiteral("cemu-dev"));
    QCoreApplication::setApplicationName(QStringLiteral("CEmu"));
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    // Register QMLBridge for Keypad<->Emu communication
    qmlRegisterSingletonType<QMLBridge>("CE.emu", 1, 0, "Emu", qmlBridgeFactory);

    MainWindow EmuWin(opts);
    EmuWin.show();

    return app.exec();
}
