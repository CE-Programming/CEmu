#include "memory.h"

#include <stdlib.h>

bool memory_init(memory_t *memory)
{
    if (memory && (memory->flash = calloc(0x400000, sizeof(uint8_t))))
    {
        if ((memory->ram = calloc(0x65800, sizeof(uint8_t))))
        {
#ifdef CEMUCORE_NODEBUG
            return true;
#else
            if ((memory->debug = calloc(0x1000000, sizeof(uint8_t))))
            {
                return true;
            }
            free(memory->ram);
#endif
        }
        free(memory->flash);
    }
    return false;
}

void memory_destroy(memory_t *memory)
{
#ifndef CEMUCORE_NODEBUG
    free(memory->debug);
#endif
    free(memory->ram);
    free(memory->flash);
}
