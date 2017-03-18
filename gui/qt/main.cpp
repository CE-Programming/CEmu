#include <QtWidgets/QApplication>

#include "mainwindow.h"
#include "keypad/qtkeypadbridge.h"
#include "utils.h"

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
    // DPI scaling fix must be applied at the very beginning before QApplication init
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("cemu-dev"));
    QCoreApplication::setApplicationName(QStringLiteral("CEmu"));

    execPath = QCoreApplication::applicationFilePath();

    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    // Setup QCommandParser with Our command line parameters
    QCommandLineParser parser;
    parser.setApplicationDescription("CEmu emulator");
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
                QCoreApplication::translate("main", "Send <File>"),
                QCoreApplication::translate("main", "File"));
    parser.addOption(sendFiles);

    // Force file to archive on send
    QCommandLineOption sendArchFiles(QStringList() << "a" << "send-arch",
                QCoreApplication::translate("main", "Force send <File> to archive"),
                QCoreApplication::translate("main", "File"));
    parser.addOption(sendArchFiles);

    // Force file to archive on send
    QCommandLineOption sendRAMFiles(QStringList() << "m" << "send-ram",
                QCoreApplication::translate("main", "Force send <File> to ram"),
                QCoreApplication::translate("main", "File"));
    parser.addOption(sendRAMFiles);

    // Loads Rom file
    QCommandLineOption loadRomFile(QStringList() << "r" << "rom",
                QCoreApplication::translate("main", "Load <RomFile>"),
                QCoreApplication::translate("main", "RomFile"));
    parser.addOption(loadRomFile);

    // Loads a json into the autotester and runs on application startup
    QCommandLineOption loadTestFile(QStringList() << "t" << "auto-test",
                QCoreApplication::translate("main", "Run <Testfile>"),
                QCoreApplication::translate("main", "TestFile"));
    parser.addOption(loadTestFile);

    // Suppresses the output of an autotester file
    QCommandLineOption suppressTestDialog(QStringList() << "no-test-dialog",
                QCoreApplication::translate("main", "Hides test complete dialog"));
    parser.addOption(suppressTestDialog);

    // Loads a CEmu settings file on start
    QCommandLineOption settingsFile(QStringList() << "g" << "settings",
                QCoreApplication::translate("main", "Load <SettingsFile> as the setup (ignored in IPC)"),
                QCoreApplication::translate("main", "SettingsFile"));
    parser.addOption(settingsFile);

    // Loads a CEmu image file on start
    QCommandLineOption imageFile(QStringList() << "i" << "image",
                QCoreApplication::translate("main", "Load <Image> into emulator"),
                QCoreApplication::translate("main", "Image"));
    parser.addOption(imageFile);

    // Loads a CEmu debugging file on start
    QCommandLineOption debugFile(QStringList() << "d" << "debug-info",
                QCoreApplication::translate("main", "Load <DebugInfo> as the setup"),
                QCoreApplication::translate("main", "DebugInfo"));
    parser.addOption(debugFile);

    QCommandLineOption procID(QStringList() << "c" << "id",
                QCoreApplication::translate("main", "Send commands to <id> if it exists, otherwise creates it"),
                QCoreApplication::translate("main", "id"));
    parser.addOption(procID);

    QCommandLineOption deforceReset(QStringList() << "no-reset",
                QCoreApplication::translate("main", "Does not reset when sending"));
    parser.addOption(deforceReset);

    QCommandLineOption forceRomReload(QStringList() << "reload-rom",
                QCoreApplication::translate("main", "Forces a rom reload"));
    parser.addOption(forceRomReload);

    QCommandLineOption emuSpeed(QStringList() << "speed",
                QCoreApplication::translate("main", "Set emulation speed percentage (value 0-500; step 10)"),
                QCoreApplication::translate("main", "speed"));
    parser.addOption(emuSpeed);

    // IPC hooks (can only use on an already running process)

    parser.process(app);

    // Take command line args and move to CEmuOpts struct
    CEmuOpts opts;
    opts.restoreOnOpen      = !parser.isSet(stateOption);
    opts.useUnthrottled     = parser.isSet(unthrottledOption);
    opts.suppressTestDialog = parser.isSet(suppressTestDialog);
    opts.deforceReset       = parser.isSet(deforceReset);
    opts.forceReloadRom     = parser.isSet(forceRomReload);
    opts.romFile            = parser.value(loadRomFile);
    opts.settingsFile       = parser.value(settingsFile);
    opts.imageFile          = parser.value(imageFile);
    opts.debugFile          = parser.value(debugFile);
    opts.sendFiles          = parser.values(sendFiles);
    opts.sendArchFiles      = parser.values(sendArchFiles);
    opts.sendRAMFiles       = parser.values(sendRAMFiles);
    if (parser.isSet(emuSpeed)) {
        opts.speed          = parser.value(emuSpeed).toInt();
        if (opts.speed < 0)   { opts.speed = 0; }
        if (opts.speed > 500) { opts.speed = 500; }
    } else {
        opts.speed = -1;
    }
    if (parser.isSet(loadTestFile)) {
        opts.autotesterFile = QDir::currentPath() + QDir::separator() + parser.value(loadTestFile);
    }

    // get application pid and tie it to the id
    opts.pidString = QString::number(QCoreApplication::applicationPid());
    opts.idString = parser.value(procID);
    if (opts.idString.isEmpty()) {
        opts.idString = "Calculator";
    }

    configPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/CEmu");

    MainWindow EmuWin(opts);
    if (!EmuWin.IsInitialized()) {
        return 0;
    }

    app.installEventFilter(keypadBridge);

    EmuWin.show();
    return app.exec();
}
