#include "link.h"
#include "asic.h"
#include "cpu.h"
#include "mem.h"
#include "control.h"
#include "interrupt.h"
#include "usb/usb.h"
#include "emu.h"
#include "os/os.h"

#include <stdio.h>
#include <string.h>

#define ADDR_SAFE_RAM   0xD052C6
#define ADDR_ERRNO      0xD008DF
#define ADDR_PRGM_SIZE  0xD0118C

#define FILE_DATA 0x35
#define FILE_DATA_START 0x37

int EMSCRIPTEN_KEEPALIVE emu_send_variable(const char *file, int location) {
    const uint8_t cxError = 0x52;
    uint8_t *cxCurApp = phys_mem_ptr(0xD007E0, 1);

    /* Return if we are at an error menu */
    if (*cxCurApp == cxError) {
        gui_console_printf("[CEmu] Transfer Error: OS in error screen.\n");
        return LINK_ERR;
    }

    static char buf[1024] = {0};
    strcpy(buf, "send:");
    strcat(buf, file);
    const char * argv[2] = { "dusb", buf };
    int err = usb_init_device(2, argv);
    gui_console_printf("[CEmu] Transfer. usb_init_device ret = %d.\n", err);

    return err == 0 ? LINK_GOOD : LINK_ERR;
}

static const char header[] = "**TI83F*\x1A\x0A\0Exported via CEmu ";
int emu_receive_variable(const char *file, const calc_var_t *vars, int count) {
    FILE *fd;
    calc_var_t var;
    uint16_t header_size = 13, size = 0, checksum = 0;
    int byte;

    fd = fopen_utf8(file, "w+b");
    if (!fd) {
        goto w_err;
    }
    setbuf(fd, NULL);
    if (fwrite(header, sizeof header - 1, 1, fd) != 1) goto w_err;
    if (fseek(fd, FILE_DATA_START, SEEK_SET))          goto w_err;
    while (count--) {
        if (!vat_search_find(vars++, &var))            goto w_err;
        if (fwrite(&header_size,       2, 1, fd) != 1) goto w_err;
        if (fwrite(&var.size,          2, 1, fd) != 1) goto w_err;
        if (fwrite(&var.type,          1, 1, fd) != 1) goto w_err;
        if (fwrite(&var.name,          8, 1, fd) != 1) goto w_err;
        if (fwrite(&var.version,       1, 1, fd) != 1) goto w_err;
        if (fputc(var.archived << 7, fd) == EOF)       goto w_err;
        if (fwrite(&var.size,          2, 1, fd) != 1) goto w_err;
        if (fwrite(var.data,    var.size, 1, fd) != 1) goto w_err;
        size += 17 + var.size;
    }
    if (fseek(fd, FILE_DATA, SEEK_SET))                goto w_err;
    if (fwrite(&size,                  2, 1, fd) != 1) goto w_err;
    if (fflush(fd))                                    goto w_err;
    while (size--) {
        if ((byte = fgetc(fd)) == EOF)                 goto w_err;
        checksum += byte;
    }
    if (fwrite(&checksum,              2, 1, fd) != 1) goto w_err;
    (void)fclose(fd);

    return LINK_GOOD;

w_err:
    (void)fclose(fd);
    if(remove(file)) {
        gui_console_printf("[CEmu] Transfer Error: Please contact the developers\n");
    }
    return LINK_ERR;
}
