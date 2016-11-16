#include <QtWidgets/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
    // DPI scaling fix must be applied at the very beginning before QApplication init
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("cemu-dev"));
    QCoreApplication::setApplicationName(QStringLiteral("CEmu"));

    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    // Setup QCommandParser with Our command line parameters
    QCommandLineParser parser;
    parser.setApplicationDescription("CEmu CE emulator");
    parser.addHelpOption();
    parser.addVersionOption();

    // Disable emulation speed throttling
    QCommandLineOption unthrottledOption(QStringList() << "u" << "unthrottled",
                QCoreApplication::translate("main", "Disable emulation speed throttling."));
    parser.addOption(unthrottledOption);

    // Disable loading a saved state and disables saving to one on application close
    QCommandLineOption stateOption(QStringList() << "n" << "no-state",
                QCoreApplication::translate("main", "Do not load state from disk."));
    parser.addOption(stateOption);

    // Sends files on start
    QCommandLineOption sendFiles(QStringList() << "s" << "send",
                QCoreApplication::translate("main", "Send <File> on startup"),
                QCoreApplication::translate("main", "File"));
    parser.addOption(sendFiles);

    // Force file to archive on send
    QCommandLineOption sendArchFiles(QStringList() << "a" << "send-arch",
                QCoreApplication::translate("main", "Force send <File> to archive on startup"),
                QCoreApplication::translate("main", "File"));
    parser.addOption(sendArchFiles);

    // Force file to archive on send
    QCommandLineOption sendRAMFiles(QStringList() << "m" << "send-ram",
                QCoreApplication::translate("main", "Force send <File> to ram on startup"),
                QCoreApplication::translate("main", "File"));
    parser.addOption(sendRAMFiles);

    // Loads Rom file
    QCommandLineOption loadRomFile(QStringList() << "r" << "rom",
                QCoreApplication::translate("main", "Load <RomFile> on startup"),
                QCoreApplication::translate("main", "RomFile"));
    parser.addOption(loadRomFile);

    // Loads a json into the autotester and runs on application startup
    QCommandLineOption loadTestFile(QStringList() << "t" << "auto-test",
                QCoreApplication::translate("main", "Run <Testfile> on startup"),
                QCoreApplication::translate("main", "TestFile"));
    parser.addOption(loadTestFile);

    // Suppresses the output of an autotester file
    QCommandLineOption suppressTestDialog(QStringList() << "no-test-dialog",
                QCoreApplication::translate("main", "Hides test complete dialog"));
    parser.addOption(suppressTestDialog);

    // Loads a CEmu settings file on start
    QCommandLineOption settingsFile(QStringList() << "g" << "settings",
                QCoreApplication::translate("main", "Load <SettingsFile> as the setup"),
                QCoreApplication::translate("main", "SettingsFile"));
    parser.addOption(settingsFile);

    // Loads a CEmu image file on start
    QCommandLineOption imageFile(QStringList() << "i" << "image",
                QCoreApplication::translate("main", "Load <Image> on start"),
                QCoreApplication::translate("main", "Image"));
    parser.addOption(imageFile);

    // Loads a CEmu debugging file on start
    QCommandLineOption debugFile(QStringList() << "d" << "debug-info",
                QCoreApplication::translate("main", "Load <DebugInfo> as the setup"),
                QCoreApplication::translate("main", "DebugInfo"));
    parser.addOption(debugFile);

    parser.process(app);

    // Take commandline args and move to CEmuOpts struct
    CEmuOpts opts;
    opts.restoreOnOpen = !parser.isSet(stateOption);
    opts.useUnthrottled = parser.isSet(unthrottledOption);
    opts.suppressTestDialog = parser.isSet(suppressTestDialog);
    if (parser.isSet(loadTestFile)){
        opts.autotesterFile =  QDir::currentPath() + QDir::separator() + parser.value(loadTestFile);
    }
    opts.romFile = parser.value(loadRomFile);
    opts.settingsFile = parser.value(settingsFile);
    opts.imageFile = parser.value(imageFile);
    opts.debugFile = parser.value(debugFile);
    opts.sendFiles = parser.values(sendFiles);
    opts.sendArchFiles = parser.values(sendArchFiles);
    opts.sendRAMFiles = parser.values(sendRAMFiles);

    MainWindow EmuWin(opts);
    EmuWin.show();

    return app.exec();
}
