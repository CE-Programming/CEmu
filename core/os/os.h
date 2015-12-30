#ifndef OS_H
#define OS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* Some really crappy APIs don't use UTF-8 in fopen. */
FILE *fopen_utf8(const char *filename, const char *mode);

#ifdef __cplusplus
}
#endif

#endif
