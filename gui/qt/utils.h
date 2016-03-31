#ifndef UTILS_H
#define UTILS_H

#include <string>
#include "../../core/vat.h"

bool fileExists(const std::string& path);
std::string calc_var_content_string(const calc_var_t& var);

#endif // UTILS_H
