#include "os.h"
#include <stdio.h>

FILE *fopen_utf8(const char *filename, const char *mode)
{
    return fopen(filename, mode);
}
