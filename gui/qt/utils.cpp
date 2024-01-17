#include "utils.h"
#include "ipc.h"
#include "debugger/disasm.h"
#include "../../core/cpu.h"
#include "../../core/os/os.h"
#include "tivars_lib_cpp/src/TIVarType.h"
#include "tivars_lib_cpp/src/TypeHandlers/TypeHandlers.h"

#include <array>

#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QTime>
#include <QtCore/QCoreApplication>
#include <QtWidgets/QApplication>
#include <QtGui/QPalette>
#include <QtGui/QStyleHints>

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#include <QtCore/QRandomGenerator>
#endif

QString execPath;
QString configPath;

bool guiDebug;
bool guiDebugBasic;
bool guiSend;
bool guiReceive;
bool guiEmuValid = false;
bool guiReset = false;

bool isNotValidHex(const std::string &s) {
    return s.find_first_not_of("0123456789ABCDEF") != std::string::npos || s.empty();
}

bool fileExists(const QString &location) {
    QString path(location);
    path = QDir::toNativeSeparators(path);

    if (path.isEmpty()) {
        return false;
    }

    if (FILE *file = fopen_utf8(path.toStdString().c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}

int hex2int(const QString &str) {
    return static_cast<int>(strtol(str.toStdString().c_str(), nullptr, 16));
}

QString int2hex(uint32_t a, uint8_t l) {
    return QString::number(a, 16).rightJustified(l, '0', true).toUpper();
}

std::string calc_var_content_string(const calc_var_t &var) {
    tivars::stringFromData_handler_t func;
    // We need to special case some specific temp-equ variables...
    if (var.type == CALC_VAR_TYPE_EQU && var.name[0] == '$') {
        func = &tivars::TH_TempEqu::makeStringFromData;
    } else {
        func = tivars::TIVarType::createFromID(static_cast<uint>(var.type)).getHandlers().second;
    }
    const options_t opts = (calc_var_is_prog(&var) || var.type == CALC_VAR_TYPE_STRING)
                            ? options_t({ {"prettify", true} }) : options_t();
    return func(data_t(var.data, var.data + var.size), opts);
}

int utf8_strlen(const std::string &str) {
    if (str.compare("𝑖") == 0 ||
        str.compare("x̄") == 0 ||
        str.compare("ȳ") == 0) {
        return 1;
    }
    if (str.compare("p̂₂") == 0 ||
        str.compare("p̂₁") == 0) {
        return 2;
    }
    return QString::fromUtf8(str.c_str()).length();
}

bool isRunningInDarkMode() {
    QPalette palette = qApp->palette();
    return palette.window().color().value() < palette.windowText().color().value();
}

bool isSystemInDarkMode() {
    bool isDarkMode;

#if (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
    isDarkMode = qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    // TODO: handle other OS' way to know if we're running in dark mode
    isDarkMode = isRunningInDarkMode();
#endif

    return isDarkMode;
}


QString getAddressOfEquate(const std::string &in) {
    QString value;
    map_value_t::const_iterator item = disasm.reverse.find(in);
    if (item != disasm.reverse.end()) {
        value = int2hex(item->second, 6);
    } else {
        uint32_t conv = 0xFFFFFFFFu;
        if (in == "AF") {
            conv = cpu.registers.AF;
        } else if (in == "HL") {
            conv = cpu.registers.HL;
        } else if (in == "DE") {
            conv = cpu.registers.DE;
        } else if (in == "BC") {
            conv = cpu.registers.BC;
        } else if (in == "IX") {
            conv = cpu.registers.IX;
        } else if (in == "IY") {
            conv = cpu.registers.IY;
        } else if (in == "AF\'") {
            conv = cpu.registers._AF;
        } else if (in == "HL\'") {
            conv = cpu.registers._HL;
        } else if (in == "DE\'") {
            conv = cpu.registers._DE;
        } else if (in == "BC\'") {
            conv = cpu.registers._BC;
        } else if (in == "SPL") {
            conv = cpu.registers.SPL;
        } else if (in == "SPS") {
            conv = cpu.registers.SPS;
        } else if (in == "PC") {
            conv = cpu.registers.PC;
        }
        if (conv != 0xFFFFFFFFu) {
            value = int2hex(conv, 6);
        }
    }
    return value;
}

void guiDelay(int ms) {
    QTime dt = QTime::currentTime().addMSecs(ms);
    while (QTime::currentTime() < dt) {
        qApp->processEvents();
    }
}

QString sendingROM(QDragEnterEvent *e, bool *value) {
    QString ret;
    if (value) { *value = false; }
    if (e->mimeData()->urls().size() == 1) {
        ret = e->mimeData()->urls().at(0).toLocalFile();
        if (ret.endsWith(QStringLiteral("rom"), Qt::CaseInsensitive)) {
            e->accept();
            if (value) { *value = true; }
        }
    }
    return ret;
}

#ifdef Q_OS_WIN
#include <windows.h>
#include <tchar.h>
#include <psapi.h>
static bool checkProc(DWORD processID) {
    QString processName;

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);

    if (hProcess) {
        HMODULE hMod;
        DWORD cbNeeded;
        wchar_t szProcessName[MAX_PATH];

        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
            if(GetModuleBaseNameW(hProcess, hMod, szProcessName, sizeof(szProcessName)/sizeof(TCHAR)) > 0) {
                processName = QString::fromUtf16((unsigned short*)szProcessName);
            }
        }
        CloseHandle(hProcess);
    }

    if (!processName.isEmpty()) {
        return true;
    }

    return false;
}
#else
#include <sys/types.h>
#include <csignal>
static bool checkProc(pid_t pid) {
    if (!kill(pid, 0)) {
        return true;
    }
    return false;
}
#endif

bool isProcRunning(pid_t procID) {
#ifdef Q_OS_WIN
    return checkProc(static_cast<DWORD>(procID));
#else
    return checkProc(procID);
#endif
}

QString randomString(const int length) {
   const QString possibleCharacters(QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"));

   QString randomString;
   for(int i=0; i<length; ++i) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
       int index = QRandomGenerator::global()->generate() % possibleCharacters.length();
#else
       int index = qrand() % possibleCharacters.length();
#endif
       QChar nextChar = possibleCharacters.at(index);
       randomString.append(nextChar);
   }
   return randomString;
}

QDir appDir() {
    QDir appDir = qApp->applicationDirPath();
#ifdef Q_OS_MACX
    appDir.cdUp(); // On macOS, the binary is
    appDir.cdUp(); // actually 3 levels deep
    appDir.cdUp(); // in the .app folder
#endif
    return appDir;
}
