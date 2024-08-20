#include "utils.h"
#include "mainwindow.h"
#include "keypad/qtkeypadbridge.h"

#include <QtCore/QProcess>
#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtWidgets/QApplication>
#include <QtGui/QFontDatabase>

int main(int argc, char *argv[]) {

#if QT_VERSION_MAJOR < 6 && defined(Q_OS_WIN)
    // DPI scaling fix must be applied at the very beginning before QApplication init
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("cemu-dev"));
    QCoreApplication::setApplicationName(QStringLiteral("CEmu"));
    QCoreApplication::setApplicationVersion(QStringLiteral(CEMU_VERSION " (git: " CEMU_GIT_SHA ")"));

    execPath = QCoreApplication::applicationFilePath();

#if QT_VERSION_MAJOR < 6
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    // Add special jacobly font
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/resources/custom_fonts/TICELarge.ttf"));

    // Setup QCommandParser with command line parameters
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("CEmu emulator"));
    parser.addHelpOption();
    parser.addVersionOption();

    // Disable emulation speed throttling
    QCommandLineOption unthrottledOption(QStringList() << QStringLiteral("u") << QStringLiteral("unthrottled"),
                QCoreApplication::translate("main", "Disable emulation speed throttling."));
    parser.addOption(unthrottledOption);

    // Disable loading a saved state and disables saving to one on application close
    QCommandLineOption noState(QStringList() << QStringLiteral("n") << QStringLiteral("no-state"),
                QCoreApplication::translate("main", "Do not load state from disk."));
    parser.addOption(noState);

    // Sends files on start
    QCommandLineOption sendFiles(QStringList() << QStringLiteral("s") << QStringLiteral("send"),
                QCoreApplication::translate("main", "Send <File>"),
                QCoreApplication::translate("main", "File"));
    parser.addOption(sendFiles);

    // Force file to archive on send
    QCommandLineOption sendArchFiles(QStringList() << QStringLiteral("a") << QStringLiteral("send-arch"),
                QCoreApplication::translate("main", "Force send <File> to archive"),
                QCoreApplication::translate("main", "File"));
    parser.addOption(sendArchFiles);

    // Force file to archive on send
    QCommandLineOption sendRAMFiles(QStringList() << QStringLiteral("m") << QStringLiteral("send-ram"),
                QCoreApplication::translate("main", "Force send <File> to ram"),
                QCoreApplication::translate("main", "File"));
    parser.addOption(sendRAMFiles);

    // Loads Rom file
    QCommandLineOption loadRomFile(QStringList() << QStringLiteral("r") << QStringLiteral("rom"),
                QCoreApplication::translate("main", "Load <RomFile>"),
                QCoreApplication::translate("main", "RomFile"));
    parser.addOption(loadRomFile);

    // Loads a json into the autotester and runs on application startup
    QCommandLineOption loadTestFile(QStringList() << QStringLiteral("t") << QStringLiteral("auto-test"),
                QCoreApplication::translate("main", "Run <Testfile>"),
                QCoreApplication::translate("main", "TestFile"));
    parser.addOption(loadTestFile);

    // Suppresses the output of an autotester file
    QCommandLineOption suppressTestDialog(QStringList() << QStringLiteral("no-test-dialog"),
                QCoreApplication::translate("main", "Hides test complete dialog"));
    parser.addOption(suppressTestDialog);

    // Loads a CEmu settings file on start
    QCommandLineOption settingsFile(QStringList() << QStringLiteral("g") << QStringLiteral("settings"),
                QCoreApplication::translate("main", "Load <SettingsFile> as the setup (ignored in IPC)"),
                QCoreApplication::translate("main", "SettingsFile"));
    parser.addOption(settingsFile);

    // Loads a CEmu image file on start
    QCommandLineOption imageFile(QStringList() << QStringLiteral("i") << QStringLiteral("image"),
                QCoreApplication::translate("main", "Load <Image> into emulator"),
                QCoreApplication::translate("main", "Image"));
    parser.addOption(imageFile);

    // Loads a CEmu debugging file on start
    QCommandLineOption debugFile(QStringList() << QStringLiteral("d") << QStringLiteral("debug-info"),
                QCoreApplication::translate("main", "Load <DebugInfo> as the setup"),
                QCoreApplication::translate("main", "DebugInfo"));
    parser.addOption(debugFile);

    QCommandLineOption procID(QStringList() << QStringLiteral("c") << QStringLiteral("id"),
                QCoreApplication::translate("main", "Send commands to <id> if it exists, otherwise creates it"),
                QCoreApplication::translate("main", "id"));
    parser.addOption(procID);

    QCommandLineOption screenshotFile(QStringList() << QStringLiteral("screenshot"),
                QCoreApplication::translate("main", "Saves a screenshot to <File> (only usable as a sent command)"),
                QCoreApplication::translate("main", "screenshot"));
    parser.addOption(screenshotFile);

    QCommandLineOption deforceReset(QStringList() << QStringLiteral("no-reset"),
                QCoreApplication::translate("main", "Does not reset when sending"));
    parser.addOption(deforceReset);

    QCommandLineOption launchPrgm(QStringList() << QStringLiteral("launch"),
                QCoreApplication::translate("main", "Launch a program specified by <prgm>"),
                QCoreApplication::translate("main", "prgm"));
    parser.addOption(launchPrgm);

    QCommandLineOption noSettings(QStringList() << QStringLiteral("no-settings"),
                QCoreApplication::translate("main", "Do not restore or save settings when running"));
    parser.addOption(noSettings);

    QCommandLineOption forceRomReload(QStringList() << QStringLiteral("reload-rom"),
                QCoreApplication::translate("main", "Forces a rom reload"));
    parser.addOption(forceRomReload);

    QCommandLineOption emuSpeed(QStringList() << QStringLiteral("speed"),
                QCoreApplication::translate("main", "Set emulation speed percentage (value 0-500; step 10)"),
                QCoreApplication::translate("main", "speed"));
    parser.addOption(emuSpeed);

    QCommandLineOption fullscreenOption(QStringList() << QStringLiteral("fullscreen"),
                QCoreApplication::translate("main", "Set fullscreen option (0 = normal, 1 = application, 2 = lcd)"),
                QCoreApplication::translate("main", "fullscreen"));
    parser.addOption(fullscreenOption);

    QCommandLineOption resetOption(QStringList() << QStringLiteral("reset"),
                QCoreApplication::translate("main", "Reset CEmu completely and delete configuration files"));
    parser.addOption(resetOption);

    // IPC hooks (can only use on an already running process)

    parser.process(app);

    // Take command line args and move to CEmuOpts struct
    CEmuOpts opts;
    opts.restoreOnOpen      = !parser.isSet(noState);
    opts.useSettings        = !parser.isSet(noSettings);
    opts.useUnthrottled     = parser.isSet(unthrottledOption);
    opts.suppressTestDialog = parser.isSet(suppressTestDialog);
    opts.deforceReset       = parser.isSet(deforceReset);
    opts.forceReloadRom     = parser.isSet(forceRomReload);
    opts.romFile            = parser.value(loadRomFile);
    opts.settingsFile       = parser.value(settingsFile);
    opts.launchPrgm         = parser.value(launchPrgm);
    opts.imageFile          = parser.value(imageFile);
    opts.debugFile          = parser.value(debugFile);
    opts.screenshotFile     = parser.value(screenshotFile);
    opts.sendFiles          = parser.values(sendFiles);
    opts.sendArchFiles      = parser.values(sendArchFiles);
    opts.sendRAMFiles       = parser.values(sendRAMFiles);
    opts.reset              = parser.isSet(resetOption);
    if (parser.isSet(emuSpeed)) {
        opts.speed          = parser.value(emuSpeed).toInt();
        if (opts.speed < 0)   { opts.speed = 0; }
        if (opts.speed > 500) { opts.speed = 500; }
    } else {
        opts.speed = -1;
    }
    if (parser.isSet(loadTestFile)) {
        opts.autotesterFile = QFileInfo(parser.value(loadTestFile)).absoluteFilePath();
    }
    if (parser.isSet(screenshotFile)) {
        opts.screenshotFile = QFileInfo(parser.value(screenshotFile)).absoluteFilePath();
    }
    if (parser.isSet(fullscreenOption)) {
        int value = parser.value(fullscreenOption).toInt();
        switch (value) {
            case 0: case 1: case 2:
                break;
            default:
                value = -1;
                break;
        }
        opts.fullscreen = value;
    } else {
        opts.fullscreen = -1;
    }

    // get application pid and tie it to the id
    opts.pidString = QString::number(QCoreApplication::applicationPid());
    opts.idString = parser.value(procID);
    if (opts.idString.isEmpty()) {
        opts.idString = QStringLiteral("Calculator");
    }

    configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    if (opts.reset) {
        QDir dir(configPath);
        dir.removeRecursively();
    }

    int ret = 0;
    MainWindow EmuWin(opts);

    if (!EmuWin.isInitialized()) {
        return ret;
    }

    if (!EmuWin.isResetAll()) {
        EmuWin.setup();
        ret = app.exec();
    }

    if (EmuWin.isResetAll() || EmuWin.isReload()) {
        QStringList args = qApp->arguments();
        if (args.length()) {
            QProcess::startDetached(args.first(), QStringList());
        }
    }

    return ret;
}
