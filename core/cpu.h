/*
 * Copyright (c) 2014 The KnightOS Group
 * Modified to support the eZ80 processor by CEmu developers
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions 
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
*/

#ifndef CPU_H
#define CPU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "registers.h"

/* eZ80 CPU Opcode Context */
typedef union eZ80context {
    uint8_t opcode;
    struct {
        uint8_t z : 3;
        uint8_t y : 3;
        uint8_t x : 2;
    };
    struct {
        uint8_t r : 1;
        uint8_t   : 2;
        uint8_t q : 1;
        uint8_t p : 2;
    };
} eZ80context_t;

/* eZ80 CPU State */
PACK(typedef ALIGNED_(8) struct eZ80cpu {
    eZ80registers_t registers;
    struct {
        uint8_t NMI         : 1;  /* Non-Maskable interrupt  */
        uint8_t IEF1        : 1;  /* Interrupt enable flag 1 */
        uint8_t IEF2        : 1;  /* Interrupt enable flag 2 */
        uint8_t ADL         : 1;  /* ADL bit                 */
        uint8_t MADL        : 1;  /* Mixed-Memory modes      */
        uint8_t IM          : 2;  /* Current interrupt mode  */

        /* Internal use: */
        uint8_t PREFIX      : 2;  /* Which index register is in use. 0: hl, 2: ix, 3: iy                                         */
        uint8_t SUFFIX      : 1;  /* There was an explicit suffix                                                                */
      /*uint8_t S           : 1;*//* The CPU data block operates in Z80 mode using 16-bit registers. All addresses use MBASE.    */
        uint8_t L           : 1;  /* The CPU data block operates in ADL mode using 24-bit registers. Addresses do not use MBASE. */
      /*uint8_t IS          : 1;*//* The CPU control block operates in Z80 mode.                                                 */
        uint8_t IL          : 1;  /* The CPU control block operates in ADL mode.                                                 */
        uint8_t IEF_wait    : 2;  /* Wait for interrupt                                                                          */
        uint8_t halted      : 1;  /* Have we halted the CPU?                                                                     */
        uint8_t inBlock     : 1;  /* Are we processing a block instruction?                                                      */
    };
    eZ80context_t context;
    uint32_t cycles, next;
    int64_t cycles_offset;
    uint8_t prefetch, bus;
    uint32_t cpuEventsState;
}) eZ80cpu_t;

/* Externals */
extern eZ80cpu_t cpu;

/* Available Functions */
void cpu_init(void);
void cpu_reset(void);
void cpu_flush(uint32_t, bool);
void cpu_nmi(void);
void cpu_execute(void);

/* Save/Restore */
typedef struct emu_image emu_image;
bool cpu_restore(const emu_image*);
bool cpu_save(emu_image*);

#ifdef __cplusplus
}
#endif

#endif
