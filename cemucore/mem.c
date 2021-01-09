#include "mem.h"

#include <stdlib.h>

bool mem_init(mem_t *mem)
{
    if (mem && (mem->flash = calloc(0x400000, sizeof(uint8_t))))
    {
        if ((mem->ram = calloc(0x65800, sizeof(uint8_t))))
        {
#ifdef CEMUCORE_NODEBUG
            return true;
#else
            if ((mem->dbg = calloc(0x1000000, sizeof(uint8_t))))
            {
                return true;
            }
            free(mem->ram);
#endif
        }
        free(mem->flash);
    }
    return false;
}

void mem_destroy(mem_t *mem)
{
#ifndef CEMUCORE_NODEBUG
    free(mem->dbg);
#endif
    free(mem->ram);
    free(mem->flash);
}
