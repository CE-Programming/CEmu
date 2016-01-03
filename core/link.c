#include <stdio.h>
#include <string.h>

#include "link.h"
#include "asic.h"
#include "emu.h"
#include "os/os.h"

volatile bool emu_is_sending = false;
volatile bool emu_is_recieving = false;

static const int ram_start = 0xD00000;
static const int safe_ram_loc = 0xD052C6;

static const uint8_t jforcegraph[8] = {
    0xFD, 0xCB, 0x03, 0x86,   // res graphdraw,(iy+graphflags)
    0xC3, 0x7C, 0x14, 0x02    // jp _jforcegraphnokey
};

static const uint8_t jforcehome[6] = {
    0x3E, 0x09,                // ld a,kclear
    0xC3, 0x64, 0x01, 0x02     // jp _jforcecmd
};

static const uint8_t archivevar[14] = {
    0xCD, 0xC8, 0x02, 0x02,     // call _op4toop1
    0xCD, 0x0C, 0x05, 0x02,     // call _chkfindsym
    0xCD, 0x4C, 0x14, 0x02,     // call _archivevar
    0x18, 0xFE                  // _sink: jr _sink
};

void enterVariableLink(void) {
  /* Wait for the GUI to finish whatever it needs to do */
  do {
      emu_sleep();
  } while(emu_is_sending || emu_is_recieving);
}

bool listVariablesLink(void) {
    calc_var_t var;
    vat_search_init(&var);
    puts("VAT:");
    while (vat_search_next(&var)) {
        printf("type: %s, name: %s, size: %hu\n",
               calc_var_type_names[var.type],
               calc_var_name_to_utf8(var.name),
               var.size);
    }
    return true;
}

/* Really hackish way to send a variable -- Like, on a scale of 1 to hackish, it's like really hackish */
/* Proper USB emulation should really be a thing at some point :P */
bool sendVariableLink(const char *var_name) {
    FILE *var;
    uint32_t var_start_ptr;
    uint8_t var_size_low, var_size_high, var_type, var_arc, context;
    uint8_t tmp_buf[0x80];
    uint8_t *run_asm_safe = &mem.ram.block[safe_ram_loc - ram_start];

    uint8_t pgrm_loader[41] = {
        0xF5,                               // push af
        0xE5,                               //  push hl
        0xCD, 0x28, 0x06, 0x02,             //   call _pushop1
        0xCD, 0x0C, 0x05, 0x02,             //   call _chkfindsym
        0xD4, 0x34, 0x14, 0x02,             //   call nc,_delvararc
        0xCD, 0xC4, 0x05, 0x02,             //   call _popop1
        0xE1,                               //  pop hl
        0xF1,                               // pop af
        0xCD, 0x38, 0x13, 0x02,             // call _createvar
        0x13, 0x13,                         // inc de / inc de
        0xED, 0x53, 0xC6, 0x52, 0xD0,       // ld (safe_ram_loc),de
        0x18, 0xFE                          // _sink: jr _sink
    };

    context = mem.ram.block[0xD007E0 - ram_start];

    /* Return if we are at an error menu */
    if(context == 0x52) {
        return false;
    }

    var = fopen_utf8(var_name,"r+b");

    /* Check the name */
    fread(&tmp_buf[0], 1, 8, var);
    if ((tmp_buf[0] != 0x2A) && (tmp_buf[1] != 0x2A) && (tmp_buf[2] != 0x54) &&
        (tmp_buf[3] != 0x49) && (tmp_buf[4] != 0x38) && (tmp_buf[5] != 0x33) &&
        (tmp_buf[6] != 0x46) && (tmp_buf[7] != 0x2A)) {
            fclose(var);
            return false;
    }

    fseek(var, 0x48, 0);
    fread(&var_size_low, 1, 1, var);
    fread(&var_size_high, 1, 1, var);

    fseek(var, 0x3B, 0);
    fread(&var_type, 1, 1, var);

    memcpy(&tmp_buf[0], &mem.ram.block[safe_ram_loc - ram_start], 0x80);

    /* if the variable is not archived, don't archive it */
    fseek(var, 0x45, 0);
    fread(&var_arc, 1, 1, var);

    /* switch context to graph screen */
    cpu.halted = cpu.IEF_wait = 0;
    cpu.registers.PC = safe_ram_loc;
    memcpy(&run_asm_safe[0], &jforcegraph[0], sizeof(jforcegraph));
    cycle_count_delta = -5000000;
    cpu_execute();

    /* Create the variable */
    fseek(var, 0x3B, 0);
    fread(&mem.ram.block[0xD005F8-ram_start], 1, 9, var);
    cpu.halted = cpu.IEF_wait = 0;
    cpu.registers.PC = safe_ram_loc;
    run_asm_safe[0] = 0x21;
    run_asm_safe[1] = var_size_low;
    run_asm_safe[2] = var_size_high;
    run_asm_safe[3] = 0x00;
    run_asm_safe[4] = 0x3E;
    run_asm_safe[5] = var_type;
    memcpy(&run_asm_safe[6],&pgrm_loader[0],sizeof(pgrm_loader));
    cycle_count_delta = -10000000;
    cpu_execute();
    var_start_ptr = run_asm_safe[0] | (run_asm_safe[1]<<8) | (run_asm_safe[2]<<16);
    fseek(var, 0x4A, 0);
    fread(&mem.ram.block[var_start_ptr - ram_start], 1, (var_size_high<<8) | var_size_low, var);

    /* If it is archived, archive it */
    if (var_arc == 0x80) {
        cpu.halted = cpu.IEF_wait = 0;
        cpu.registers.PC = safe_ram_loc;
        memcpy(&run_asm_safe[0],&archivevar[0],sizeof(archivevar));
        cycle_count_delta = -1000000;
        cpu_execute();
    }

    /* Switch context to the home screen */
    cpu.halted = cpu.IEF_wait = 0;
    cpu.registers.PC = safe_ram_loc;
    memcpy(&run_asm_safe[0], &jforcehome[0], sizeof(jforcehome));
    cycle_count_delta = -5000000;
    cpu_execute();

    memcpy(&mem.ram.block[safe_ram_loc - ram_start], &tmp_buf[0], 0x80);

    fclose(var);
    return true;
}
