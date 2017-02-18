#ifndef UTILS_H
#define UTILS_H

#include <string>

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

bool fileExists(const std::string& path);
std::string calc_var_content_string(const calc_var_t& var);

// Qt Specific
#include <QtCore/QString>

extern QString bppModeStr[];
extern QByteArray *bppArray;

// bpp conversions
uint8_t int2Bpp(int in);
QString bpp2Str(uint8_t bpp);

// integer to hex strings and vice versa
int hex2int(QString str);
QString int2hex(uint32_t a, uint8_t l);
void guiDelay(int ms);

// local config path
extern QString configPath;
extern QString execPath;

bool IsProcRunning(pid_t procID);

QString randomString(const int length);

#endif
