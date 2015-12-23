#ifndef CPU_H
#define CPU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <core/registers.h>
#include <core/memory.h>
#include <core/apb.h>

/* eZ80 CPU State */
struct eZ80cpu {
    eZ80portrange_t prange[0x10];    /* 0x0000-0xF000 */
    eZ80registers_t registers;
    struct {
        uint8_t IEF1        : 1;  /* IFlag1                 */
        uint8_t IEF2        : 1;  /* IFlag2                 */
        uint8_t ADL         : 1;  /* ADL bit                */
        uint8_t MADL        : 1;  /* Mixed-Memory modes     */
        uint8_t IM          : 2;  /* Current interrupt mode */

        /* Internal use: */
        uint8_t PREFIX      : 2;  /* Which index register is in use. 0: hl, 2: ix, 3: iy                                         */
        uint8_t SUFFIX      : 1;  /* There was an explicit suffix                                                                */
        uint8_t S           : 1;  /* The CPU data block operates in Z80 mode using 16-bit registers. All addresses use MBASE.    */
        uint8_t L           : 1;  /* The CPU data block operates in ADL mode using 24-bit registers. Addresses do not use MBASE. */
        uint8_t IS          : 1;  /* The CPU control block operates in Z80 mode.                                                 */
        uint8_t IL          : 1;  /* The CPU control block operates in ADL mode.                                                 */
        uint8_t IEF_wait    : 1;  /* Wait for interrupt                                                                          */
        uint8_t halted      : 1;  /* Have we halted the CPU?                                                                     */
    };
    int cycles;
    uint8_t bus;
    mem_state_t *memory;  // pointer to memory
    int (*get_mem_wait_states)();
    uint8_t (*read_byte)(uint32_t address);
    void (*write_byte)(uint32_t address, uint8_t byte);
    int interrupt;
};

/* Type definitions */
typedef struct eZ80cpu eZ80cpu_t;

/* Externals */
extern eZ80cpu_t cpu;

/* Available Functions */
void cpu_init(void);

int cpu_execute(int);

#ifdef __cplusplus
}
#endif

#endif
