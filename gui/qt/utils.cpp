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

std::string calc_var_content_string(const calc_var_t& var)
{
    auto func = tivars::TypeHandlerFuncGetter::getStringFromDataFunc((int)var.type);
    return func(data_t(var.data, var.data + var.size), options_t());
}
