#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

bool extractor(const char *filename, const char *tmppath, bool (*mark)(const char*));

#ifdef __cplusplus
}
#endif

#endif
