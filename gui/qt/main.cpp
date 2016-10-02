#include <QtWidgets/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Setup QCommandParser with Our command line parameters
    QCommandLineParser parser;
    parser.setApplicationDescription("CEmu CE emulator");
    parser.addHelpOption();
    parser.addVersionOption();

    // Disable emulation speed throttling
    QCommandLineOption unthrottledOption(QStringList() << "u" << "unthrottled",
                QCoreApplication::translate("main", "Disable emulation speed throttling."));
    parser.addOption(unthrottledOption);

    // Disable loading a saved state and disables saving to one on application Close
    QCommandLineOption stateOption(QStringList() << "s" << "no-state",
                QCoreApplication::translate("main", "Do not load state from disk."));
    parser.addOption(stateOption);

    // Loads Rom file
    QCommandLineOption loadRomFile(QStringList() << "r" << "rom",
                QCoreApplication::translate("main", "Load <RomFile> on startup"),
                QCoreApplication::translate("main", "RomFile"));
    parser.addOption(loadRomFile);

    // Loads a json into the autotester and runs on application startup
    QCommandLineOption loadTestFile(QStringList() << "t" << "auto-test",
                QCoreApplication::translate("main", "run <Testfile> on startup"),
                                    QCoreApplication::translate("main", "TestFile"));
                        parser.addOption(loadTestFile);
                        QCommandLineOption suppressTestDialog(QStringList() << "suppress-test-dialog",
                QCoreApplication::translate("main", "Hides test complete dialog"));
    parser.addOption(suppressTestDialog);

    parser.process(app);

    // Take commandline args and move to CEmuOpts struct
    CEmuOpts opts;
    opts.restoreOnOpen = parser.isSet(stateOption) ? false : true;
    opts.useUnthrottled = parser.isSet(unthrottledOption);
    opts.suppressTestDialog = parser.isSet(suppressTestDialog);
    if (parser.isSet(loadTestFile)){
        opts.AutotesterFile =  QDir::currentPath() + "/" +parser.value(loadTestFile);
    }
    opts.RomFile = parser.value(loadRomFile);

    QCoreApplication::setOrganizationName(QStringLiteral("cemu-dev"));
    QCoreApplication::setApplicationName(QStringLiteral("CEmu"));
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    MainWindow EmuWin(opts);
    EmuWin.show();

    return app.exec();
}
