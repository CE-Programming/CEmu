#include "utils.h"
#include "os/os.h"

bool fileExists(const std::string& path)
{
    if (FILE *file = fopen_utf8(path.c_str(), "r"))
    {
        fclose(file);
        return true;
    } else {
        return false;
    }
}
