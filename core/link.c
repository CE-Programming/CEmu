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

#define FILE_PRODUCT_ID 0x0A
#define FILE_DATA 0x35
#define FILE_DATA_START 0x37

int EMSCRIPTEN_KEEPALIVE emu_send_variable(const char *file, int location) {
    return emu_send_variables(&file, 1, location, NULL, NULL);
}

int EMSCRIPTEN_KEEPALIVE emu_send_variables(const char *const *files, int num, int location,
                                            usb_progress_handler_t *progress_handler,
                                            void *progress_context) {
    static const char *locations[] = { "-r", "-a", "" };

    const size_t argv_size = (1+num) * sizeof(char *);
    char **argv = malloc(argv_size);
    if (!argv) {
        gui_console_printf("[CEmu] Transfer Error: can't allocate transfer commands... wut\n");
        return LINK_ERR;
    }

    char name[] = "dusb";
    argv[0] = name;
    int err = 1;
    for(int i=0; i<num; i++) {
        const size_t arg_size = 7 + strlen(files[i]) + 1;
        argv[i+1] = malloc(arg_size);
        if (!argv[i+1]) {
            gui_console_printf("[CEmu] Transfer Error: can't allocate transfer command... wut\n");
            num = i;
            goto alloc_err;
        }
        snprintf(argv[i+1], arg_size, "send%s:%s", locations[location], files[i]);
    }
    err = usb_init_device(1+num, (const char *const *)argv, progress_handler, progress_context);
    if (err != 0) {
        gui_console_printf("[CEmu] USB transfer error code %d.\n", err);
    }
    else {
        gui_console_printf("[CEmu] USB transfer(s) completed successfully.\n");
    }

alloc_err:
    for(int i=1; i<=num; i++) {
        free(argv[i]);
    }
    free(argv);

    return err == 0 ? LINK_GOOD : LINK_ERR;
}

static inline size_t write_le16(uint16_t val, FILE *fd) {
    uint16_t to_write = to_le16(val);
    return fwrite(&to_write, sizeof to_write, 1, fd);
}

int emu_receive_variable(const char *file, const calc_var_t *vars, int count) {
    static const char header[] = "**TI83F*\x1A\x0A\0Exported via CEmu ";
    FILE *fd;
    calc_var_t var;
    uint16_t header_size = 13, size = 0, checksum = 0;
    uint8_t max_version = 0, product_id;
    int byte;

    fd = fopen_utf8(file, "w+b");
    if (!fd) {
        return LINK_ERR;
    }
    setbuf(fd, NULL);
    if (fwrite(header, sizeof header - 1, 1, fd) != 1) goto w_err;
    if (fseek(fd, FILE_DATA_START, SEEK_SET))          goto w_err;
    while (count--) {
        if (!vat_search_find(vars++, &var))            goto w_err;
        if (write_le16(header_size,        fd) !=   1) goto w_err;
        if (write_le16(var.size,           fd) !=   1) goto w_err;
        if (fputc(var.type,                fd) == EOF) goto w_err;
        if (fwrite(&var.name,        8, 1, fd) !=   1) goto w_err;
        if (fputc(var.version,             fd) == EOF) goto w_err;
        if (fputc(var.archived << 7,       fd) == EOF) goto w_err;
        if (write_le16(var.size,           fd) !=   1) goto w_err;
        if (fwrite(var.data,  var.size, 1, fd) !=   1) goto w_err;
        size += 17 + var.size;
        if ((var.version & ~0x20) > max_version) {
            max_version = var.version & ~0x20;
        }
    }
    if (max_version > 0x0A) {
        product_id = 0x13;
    } else if (max_version == 0x0A) {
        product_id = 0x0F;
    } else {
        product_id = 0x0A;
    }
    if (fseek(fd, FILE_PRODUCT_ID, SEEK_SET))          goto w_err;
    if (fputc(product_id,                  fd) == EOF) goto w_err;
    if (fseek(fd, FILE_DATA, SEEK_SET))                goto w_err;
    if (write_le16(size,                   fd) !=   1) goto w_err;
    if (fflush(fd))                                    goto w_err;
    while (size--) {
        if ((byte = fgetc(fd)) == EOF)                 goto w_err;
        checksum += byte;
    }
    if (write_le16(checksum,               fd) !=   1) goto w_err;
    (void)fclose(fd);

    return LINK_GOOD;

w_err:
    (void)fclose(fd);
    if(remove(file)) {
        gui_console_printf("[CEmu] Transfer Error: Please contact the developers\n");
    }
    return LINK_ERR;
}

int emu_cancel_transfer(void) {
    return usb_init_device(0, NULL, NULL, NULL);
}
