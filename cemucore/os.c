/*
 * Copyright (c) 2015-2021 CE Programming.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "os.h"

#ifdef _WIN32
# include <windows.h>
#endif

FILE *fopen_utf8(const char *name, const char *mode)
{
#ifdef _WIN32
    wchar_t wName[MAX_PATH];
    wchar_t wMode[5];
    MultiByteToWideChar(CP_UTF8, 0, name, -1, wName, MAX_PATH);
    MultiByteToWideChar(CP_UTF8, 0, mode, -1, wMode, 5);
    return _wfopen(wName, wMode);
#else
    return fopen(name, mode);
#endif
}
