#include "archive/extractor.h"

#ifdef LIB_ARCHIVE_SUPPORT
#include <archive.h>
#include <archive_entry.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

bool extractor(const char *filename, const char *tmppath, bool (*mark)(const char*)) {
#ifndef LIB_ARCHIVE_SUPPORT
    (void)filename;
    (void)tmppath;
    (void)mark;
    return false;
#else
    struct archive *a = archive_read_new();
    struct archive *ext;
    struct archive_entry *entry;
    static char arr[4096];
    const void *buff;
    size_t size;
    la_int64_t offset;
    int r;

    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    if (archive_read_open_filename(a, filename, 10240)) {
        archive_read_free(a);
        return false;
    }

    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_FFLAGS);
    archive_write_disk_set_standard_lookup(ext);

    for (;;) {
        r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF)
            break;
        if (r != ARCHIVE_OK)
            return false;
        if (archive_entry_filetype(entry) == AE_IFREG) {
            archive_entry_pathname(entry);
            memset(arr, 0, sizeof arr);
            strncpy(arr, tmppath, 3000);
            strcat(arr, "/");
            strcat(arr, archive_entry_pathname(entry));
            if (!mark(arr))
                continue;
            archive_entry_set_pathname(entry, arr);

            r = archive_write_header(ext, entry);
            if (r != ARCHIVE_OK)
                return false;

            for (;;) {
                r = archive_read_data_block(a, &buff, &size, &offset);
                if (r == ARCHIVE_EOF)
                    break;
                if (r == ARCHIVE_OK)
                    r = (int)archive_write_data_block(ext, buff, size, offset);
                if (r != ARCHIVE_OK)
                    return false;
            }

            r = archive_write_finish_entry(ext);
            if (r != ARCHIVE_OK)
                return false;
        }
    }
    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);
    return true;
#endif
}
