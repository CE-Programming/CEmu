#include "utils.h"
#include "../../core/os/os.h"

bool fileExists(const std::string& path)
{
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
