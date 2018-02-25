#include <QDir>
#include <QtCore/QString>
#include <QtCore/QTime>
#include <QtCore/QCoreApplication>

#include "ipc.h"
#include "utils.h"
#include "../../core/os/os.h"
#include "../../core/debug/disasm.h"

#include "tivarslib/TIVarType.h"
#include "tivarslib/TypeHandlers/TypeHandlers.h"

QString execPath;
QString configPath;

bool guiDebug;
bool guiSend;
bool guiReceive;
bool guiEmuValid = false;
bool guiReset = false;

bool isNotValidHex(const std::string& s) {
    return s.find_first_not_of("0123456789ABCDEF") != std::string::npos || s.empty();
}

bool fileExists(const QString& ptath) {
    QString path(ptath);
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
    return (int)strtol(str.toStdString().c_str(), nullptr, 16);
}

QString int2hex(uint32_t a, uint8_t l) {
    return QString::number(a, 16).rightJustified(l, '0', true).toUpper();
}

std::string calc_var_content_string(const calc_var_t& var) {
    const auto func = tivars::TIVarType::createFromID((uint)var.type).getHandlers().second;
    const options_t opts = (var.type == CALC_VAR_TYPE_PROG || var.type == CALC_VAR_TYPE_STRING)
                            ? options_t({ {"prettify", true} }) : options_t();
    return func(data_t(var.data, var.data + var.size), opts);
}

QString getAddressOfEquate(const std::string &in) {
    QString value;
    map_value_t::const_iterator item = disasm.reverseMap.find(in);
    if (item != disasm.reverseMap.end()) {
        value = int2hex(item->second, 6);
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
#include <signal.h>
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
       int index = qrand() % possibleCharacters.length();
       QChar nextChar = possibleCharacters.at(index);
       randomString.append(nextChar);
   }
   return randomString;
}
