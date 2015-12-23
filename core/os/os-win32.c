#include "os.h"
#include <stdio.h>
#include <windows.h>

FILE *fopen_utf8(const char *filename, const char *mode)
{
    wchar_t filename_w[MAX_PATH];
    wchar_t mode_w[5];
    MultiByteToWideChar(CP_UTF8, 0, filename, -1, filename_w, MAX_PATH);
    MultiByteToWideChar(CP_UTF8, 0, mode, -1, mode_w, 5);
    return _wfopen(filename_w, mode_w);
}
