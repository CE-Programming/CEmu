#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <QtCore/QMimeData>
#include <QtGui/QDrag>
#include <QtGui/QDragEnterEvent>

#ifdef _MSC_VER
// Define a custom version of pid_t for MSVC (DWORD native type)
#include <windows.h>
#define pid_t DWORD
#else
#include <unistd.h>
#endif

#include "ipc.h"
#include "../../core/vat.h"

#define set_reset(in, out, var) ((var) = (bool)(in) ? ((var) | (out)) : ((var) & ~(out)))

bool fileExists(const QString& path);
std::string calc_var_content_string(const calc_var_t& var);

// Qt Specific
#include <QtCore/QString>

// local config path
extern QString configPath;
extern QString execPath;

QString sendingROM(QDragEnterEvent *e, bool *value);

// integer to hex strings and vice versa
int hex2int(const QString &str);
QString int2hex(uint32_t a, uint8_t l);
void guiDelay(int ms);

bool isProcRunning(pid_t procID);

QString randomString(const int length);

bool isNotValidHex(const std::string& s);

extern bool guiDebug;
extern bool guiSend;
extern bool guiReceive;
extern bool guiEmuValid;

#endif
