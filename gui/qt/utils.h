#ifndef UTILS_H
#define UTILS_H

#include <string>
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

#endif
