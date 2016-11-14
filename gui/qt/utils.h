#ifndef UTILS_H
#define UTILS_H

#include <QString>

#include <string>
#include "../../core/vat.h"

bool fileExists(const std::string& path);
std::string calc_var_content_string(const calc_var_t& var);

int hex2int(QString str);
QString int2hex(uint32_t a, uint8_t l);

#endif // UTILS_H
