#include <QString>

#include "utils.h"
#include "../../core/os/os.h"
#include "tivarslib/autoloader.h"

bool fileExists(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    if (FILE *file = fopen_utf8(path.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}

int hex2int(QString str) {
    return (int)strtol(str.toStdString().c_str(), nullptr, 16);
}

QString int2hex(uint32_t a, uint8_t l) {
    return QString::number(a, 16).rightJustified(l, '0', true).toUpper();
}

std::string calc_var_content_string(const calc_var_t& var)
{
    auto func = tivars::TypeHandlerFuncGetter::getStringFromDataFunc((int)var.type);
    return func(data_t(var.data, var.data + var.size), options_t());
}
