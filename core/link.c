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

static const uint8_t header_data[10] = {
    0x2A, 0x2A, 0x54, 0x49, 0x38, 0x33, 0x46, 0x2A, 0x1A, 0x0A
};

static const uint8_t pgrm_loader[41] = {
  0xF5,                         // push af
  0xE5,                         // push hl
  0xCD, 0x28, 0x06, 0x02,       // call _pushop1
  0xCD, 0x0C, 0x05, 0x02,       // call _chkfindsym
  0xD4, 0x34, 0x14, 0x02,       // call nc,_delvararc
  0xCD, 0xC4, 0x05, 0x02,       // call _popop1
  0xE1,                         // pop hl
  0xF1,                         // pop af
  0xCD, 0x38, 0x13, 0x02,       // call _createvar
  0x13, 0x13,                   // inc de / inc de
  0xED, 0x53, 0xC6, 0x52, 0xD0, // ld (safe_ram_loc),de
  0x18, 0xFE                    // _sink: jr _sink
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
    FILE *file;
    uint8_t tmp_buf[0x80];

    uint8_t var_size_low,
            var_size_high,
            var_type,
            var_arc;

    uint8_t *run_asm_safe = phys_mem_ptr(safe_ram_loc, 1),
            *cxCurApp     = phys_mem_ptr(0xD007E0, 1),
            *op1          = phys_mem_ptr(0xD005F8, 1),
            *var_ptr;

    uint16_t var_size;

    const size_t h_size = sizeof(header_data);
    const size_t op_size = 9;

    /* Return if we are at an error menu */
    if(*cxCurApp == 0x52) {
        return false;
    }

    file = fopen_utf8(var_name,"rb");

    if (!file) return false;
    if (fread(tmp_buf, 1, h_size, file) != h_size)        goto r_err;
    if (memcmp(tmp_buf, header_data, h_size))             goto r_err;

    if (fseek(file, 0x48, 0))                             goto r_err;
    if (fread(&var_size_low, 1, 1, file) != 1)            goto r_err;
    if (fread(&var_size_high, 1, 1, file) != 1)           goto r_err;

    if (fseek(file, 0x3B, 0))                             goto r_err;
    if (fread(&var_type, 1, 1, file) != 1)                goto r_err;

    if (fseek(file, 0x45, 0))                             goto r_err;
    if (fread(&var_arc, 1, 1, file) != 1)                 goto r_err;

    cpu.halted = cpu.IEF_wait = 0;
    cpu.registers.PC = safe_ram_loc;
    memcpy(run_asm_safe, jforcegraph, sizeof(jforcegraph));
    cycle_count_delta = -5000000;
    cpu_execute();

    if (fseek(file, 0x3B, 0))                            goto r_err;
    if (fread(op1, 1, op_size, file) != op_size)         goto r_err;
    cpu.halted = cpu.IEF_wait = 0;
    cpu.registers.PC = safe_ram_loc;
    run_asm_safe[0] = 0x21;
    run_asm_safe[1] = var_size_low;
    run_asm_safe[2] = var_size_high;
    run_asm_safe[3] = 0x00;
    run_asm_safe[4] = 0x3E;
    run_asm_safe[5] = var_type;
    memcpy(&run_asm_safe[6], pgrm_loader, sizeof(pgrm_loader));
    cycle_count_delta = -10000000;
    cpu_execute();

    var_ptr = phys_mem_ptr((run_asm_safe[0])      |
                           (run_asm_safe[1] << 8) |
                           (run_asm_safe[2] << 16), 1);

    var_size = (var_size_high << 8) | var_size_low;

    if (fseek(file, 0x4A, 0))                           goto r_err;
    if (fread(var_ptr, 1, var_size, file) != var_size)  goto r_err;

    if (var_arc == 0x80) {
        cpu.halted = cpu.IEF_wait = 0;
        cpu.registers.PC = safe_ram_loc;
        memcpy(run_asm_safe, archivevar, sizeof(archivevar));
        cycle_count_delta = -1000000;
        cpu_execute();
    }

    cpu.halted = cpu.IEF_wait = 0;
    cpu.registers.PC = safe_ram_loc;
    memcpy(run_asm_safe, jforcehome, sizeof(jforcehome));
    cycle_count_delta = -5000000;
    cpu_execute();

    return !fclose(file);

r_err:
    fclose(file);
    return false;
}

#define STRINGIFYMAGIC(x) #x
#define STRINGIFY(x) STRINGIFYMAGIC(x)
static char header[] = "**TI83F*\x1A\x0A\0File dumped from CEmu " STRINGIFY(CEMU_VERSION);
#undef STRIGIFY
#undef STRIGIFYMAGIC
bool receiveVariableLink(int count, const calc_var_t *vars, const char *file_name) {
    FILE *file;
    calc_var_t var;
    uint16_t header_size = 13, size = 0, checksum = 0;
    int byte;
    file = fopen_utf8(file_name, "w+b");;
    if (!file) return false;
    if (fwrite(header, sizeof header - 1, 1, file) != 1) goto w_err;
    if (fseek(file, 0x37, SEEK_SET))                     goto w_err;
    while (count--) {
        if (!vat_search_find(vars++, &var))              goto w_err;
        if (fwrite(&header_size,       2, 1, file) != 1) goto w_err;
        if (fwrite(&var.size,          2, 1, file) != 1) goto w_err;
        if (fwrite(&var.type,          1, 1, file) != 1) goto w_err;
        if (fwrite(&var.name,          8, 1, file) != 1) goto w_err;
        if (fwrite(&var.version,       1, 1, file) != 1) goto w_err;
        if (fputc(var.archived << 7, file) == EOF)       goto w_err;
        if (fwrite(&var.size,          2, 1, file) != 1) goto w_err;
        if (fwrite(var.data,    var.size, 1, file) != 1) goto w_err;
        size += 17 + var.size;
    }
    if (fseek(file, 0x35, SEEK_SET))                     goto w_err;
    if (fwrite(&size,                  2, 1, file) != 1) goto w_err;
    while ((byte = fgetc(file)) != EOF) {
        checksum += byte;
    }
    if (ferror(file))                                    goto w_err;
    if (fwrite(&checksum,              2, 1, file) != 1) goto w_err;
    return !fclose(file);

  w_err:
    fclose(file);
    remove(file_name);
    return false;
}
