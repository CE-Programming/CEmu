#include <stdio.h>
#include <string>
#include <string.h>

#include "link.h"
#include "asic.h"
#include "emu.h"
#include "os/os.h"

link_state_t link_state;

static const int ram_start = 0xD00000;

static const uint8_t pgrm_loader[34] = {
      0xF3, 0xF5, 0xE5, 0xCD, 0x28, 0x06, 0x02, 0xCD, 0x0C, 0x05, 0x02, 0xD4, 0x34,
      0x14, 0x02, 0xCD, 0xC4, 0x05, 0x02, 0xE1, 0xF1, 0xCD, 0x38, 0x13, 0x02,
      0x13, 0x13, 0xED, 0x53, 0xC6, 0x52, 0xD0, 0x18, 0xFE
};

/* Really hackish way to send a variable -- Like, on a scale of 1 to hackish, it's like really hackish */
/* Proper USB emulation should really be a thing at some point :P */
bool sendVariableLink(void) {
    FILE *var;
    eZ80cpu_t cpu_state;

    uint8_t *run_asm_safe = mem.ram.block + 0xD052C6 - ram_start;

    uint8_t tmp_buf[0xFF];
    uint8_t var_size_low, var_size_high, var_type;
    int var_start_ptr;

    gui_send_entered(true);

    /* Wait for the GUI to send the file */
    do {
        emu_sleep();
    } while(!link_state.is_sending);

    var = fopen_utf8(link_state.current_file.c_str(),"r+b");

    /* check to see if we are executing an asm program */
    if (!(mem.ram.block[0xD0118C-ram_start] | mem.ram.block[0xD0118D-ram_start]<<8)) {
        /* save the current state of the cpu */
        memcpy(&cpu_state, &cpu, sizeof(eZ80cpu_t));

        fread(&tmp_buf[0], 1, 8, var);
        if((tmp_buf[0] != 0x2A) && (tmp_buf[1] != 0x2A) && (tmp_buf[2] != 0x54) &&
           (tmp_buf[3] != 0x49) && (tmp_buf[4] != 0x38) && (tmp_buf[5] != 0x33) &&
           (tmp_buf[6] != 0x46) && (tmp_buf[7] != 0x2A)) {
           return false;
        }

	fseek(var, 0x48, 0);
	fread(&var_size_low, 1, 1, var);
	fread(&var_size_high, 1, 1, var);

        fseek(var, 0x3B, 0);
        fread(&var_type, 1, 1, var);

        fseek(var, 0x3C, 0);
        fread(&tmp_buf[0], 1, 8, var);

        /* Write the variable name to OP1 */
        memcpy(&mem.ram.block[0xD005F9-ram_start], &tmp_buf[0], 8);

        cpu.halted = false;
        cpu.IEF1 = cpu.IEF2 = 0;

        cpu.registers.PC = 0xD052C6;
        run_asm_safe[0] = 0x21;
        run_asm_safe[1] = var_size_low;
        run_asm_safe[2] = var_size_high;
        run_asm_safe[3] = 0x00;
        run_asm_safe[4] = 0x3E;
        run_asm_safe[5] = var_type;
        memcpy(&run_asm_safe[6],&pgrm_loader[0],34);

        /* just sink into the loop :) */
        cycle_count_delta = -500000;
        cpu_execute();

        /* Okay, now copy the actual data into the program (pointer was set by the asm routine) */
        var_start_ptr = run_asm_safe[0] | (run_asm_safe[1]<<8) | (run_asm_safe[2]<<16);

        /* variable was created in RAM, so we can just read the data directly in */
        fseek(var, 0x4A, 0);
        fread(&mem.ram.block[var_start_ptr - ram_start], 1, (var_size_high<<8) | var_size_low, var);

        /* restor the state of the cpu */
        memcpy(&cpu, &cpu_state, sizeof(eZ80cpu_t));
    }

    fclose(var);
    gui_send_entered(false);
    return true;
}
