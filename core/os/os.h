#ifndef OS_H
#define OS_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Some really crappy APIs don't use UTF-8 in fopen. */
FILE *fopen_utf8(const char *filename, const char *mode);

#ifdef __cplusplus
}
#endif

#endif
