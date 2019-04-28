#ifndef UTILS_H
#define UTILS_H

#include "ipc.h"
#include "../../core/vat.h"

#include <QtCore/QMimeData>
#include <QtCore/QObject>
#include <QtCore/QDir>
#include <QtGui/QDrag>
#include <QtGui/QDragEnterEvent>
#include <string>
#include <type_traits>

#ifdef _MSC_VER
// Define a custom version of pid_t for MSVC (DWORD native type)
#include <windows.h>
#define pid_t DWORD
#else
#include <unistd.h>
#endif

#define set_reset(in, out, var) ((var) = static_cast<bool>(in) ? ((var) | (out)) : ((var) & ~(out)))

bool fileExists(const QString& path);
std::string calc_var_content_string(const calc_var_t& var);

bool isRunningInDarkMode();

// Qt Specific
#include <QtCore/QString>

// local config path
extern QString configPath;
extern QString execPath;

QString getAddressOfEquate(const std::string &in);
QString sendingROM(QDragEnterEvent *e, bool *value);

// integer to hex strings and vice versa
int hex2int(const QString &str);
QString int2hex(uint32_t a, uint8_t l);
void guiDelay(int ms);

bool isProcRunning(pid_t procID);
void killAll();

QString randomString(const int length);
QDir appDir();

bool isNotValidHex(const std::string& s);

extern bool guiDebug;
extern bool guiDebugBasic;
extern bool guiSend;
extern bool guiReceive;
extern bool guiEmuValid;
extern bool guiReset;

template<class T>
typename std::enable_if<std::is_pointer<T>::value, T>::type
findSelfOrParent(QObject *object) {
    while (object) {
        if (T result = qobject_cast<T>(object)) {
            return result;
        }
        object = object->parent();
    }
    return Q_NULLPTR;
}

template<class T>
typename std::enable_if<std::is_pointer<T>::value, T>::type
findParent(QObject *object) {
    if (!object) {
        return Q_NULLPTR;
    }
    return findSelfOrParent<T>(object->parent());
}

#endif
