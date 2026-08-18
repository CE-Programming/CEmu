/*
 * MIT License; see LICENSE.
 *
 * UF2/FAT behavior is derived from Microsoft's uf2-samdx1 bootloader.
 * The SPI transport implements the protocol observed between the CE OS and
 * TI-Python coprocessor; no TI bootloader code or data is included.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define REG8(address)  (*(volatile uint8_t *)(address))
#define REG16(address) (*(volatile uint16_t *)(address))
#define REG32(address) (*(volatile uint32_t *)(address))

#define FLASH_SIZE             UINT32_C(0x40000)
#define APP_START_ADDRESS       UINT32_C(0x2000)
#define RAM_START_ADDRESS       UINT32_C(0x20000000)
#define RAM_END_ADDRESS         UINT32_C(0x20008000)
#define DOUBLE_TAP_ADDRESS      UINT32_C(0x20007FFC)
#define DOUBLE_TAP_MAGIC        UINT32_C(0xF01669EF)

#define PM_APBCMASK             UINT32_C(0x40000420)
#define PM_RCAUSE               UINT32_C(0x40000438)
#define PM_RCAUSE_POR           UINT8_C(0x01)
#define GCLK_CLKCTRL            UINT32_C(0x40000C02)
#define GCLK_CLKCTRL_ID(value)  ((uint16_t)(value))
#define GCLK_CLKCTRL_GEN(value) ((uint16_t)(value) << 8)
#define GCLK_CLKCTRL_CLKEN      (UINT16_C(1) << 14)
#define GCLK_ID_SERCOM0_CORE    UINT16_C(0x14)
#define GCLK_ID_SERCOM3_CORE    UINT16_C(0x17)
#define GCLK_GEN_0              UINT16_C(0)

#define SERCOM0_BASE            UINT32_C(0x42000800)
#define SERCOM3_BASE            UINT32_C(0x42001400)
#define SERCOM_CTRLA(base)      REG32((base) + 0x00)
#define SERCOM_CTRLB(base)      REG32((base) + 0x04)
#define SERCOM_BAUD(base)       REG16((base) + 0x0C)
#define SERCOM_INTFLAG(base)    REG8((base) + 0x18)
#define SERCOM_DATA(base)       REG16((base) + 0x28)

#define SERCOM_CTRLA_SWRST      (UINT32_C(1) << 0)
#define SERCOM_CTRLA_ENABLE     UINT32_C(0x00000002)
#define SERCOM_CTRLA_RUNSTDBY   (UINT32_C(1) << 7)
#define SERCOM_CTRLA_DORD       (UINT32_C(1) << 30)
#define SERCOM_SPI_MODE_SLAVE   (UINT32_C(2) << 2)
#define SERCOM_SPI_DOPO(value)  ((uint32_t)(value) << 16)
#define SERCOM_USART_MODE_INT   (UINT32_C(1) << 2)
#define SERCOM_USART_RXPO(value) ((uint32_t)(value) << 20)
#define SERCOM_SPI_PLOADEN      (UINT32_C(1) << 6)
#define SERCOM_SPI_SSDE         (UINT32_C(1) << 9)
#define SERCOM_SPI_RXEN         UINT32_C(0x00020000)
#define SERCOM_USART_TXEN       (UINT32_C(1) << 16)
#define SERCOM_USART_RXEN       (UINT32_C(1) << 17)
#define SERCOM_INTFLAG_DRE      UINT8_C(0x01)
#define SERCOM_INTFLAG_TXC      UINT8_C(0x02)
#define SERCOM_INTFLAG_RXC      UINT8_C(0x04)
#define SERCOM_INTFLAG_SSL      UINT8_C(0x08)

#define NVMCTRL_BASE            UINT32_C(0x41004000)
#define NVMCTRL_CTRLA           REG16(NVMCTRL_BASE + 0x00)
#define NVMCTRL_CTRLB           REG32(NVMCTRL_BASE + 0x04)
#define NVMCTRL_INTFLAG         REG8(NVMCTRL_BASE + 0x14)
#define NVMCTRL_ADDR            REG32(NVMCTRL_BASE + 0x1C)
#define NVMCTRL_READY           UINT8_C(0x01)
#define NVMCTRL_MANW            UINT32_C(0x80)
#define NVMCTRL_COMMAND(value)  (UINT16_C(0xA500) | (value))
#define NVMCTRL_ERASE_ROW       UINT16_C(0x02)
#define NVMCTRL_WRITE_PAGE      UINT16_C(0x04)
#define NVMCTRL_CLEAR_BUFFER    UINT16_C(0x44)

#define SCB_VTOR                REG32(UINT32_C(0xE000ED08))
#define SCB_AIRCR               REG32(UINT32_C(0xE000ED0C))
#define SCB_AIRCR_RESET         UINT32_C(0x05FA0004)

#define UF2_MAGIC_START0        UINT32_C(0x0A324655)
#define UF2_MAGIC_START1        UINT32_C(0x9E5D5157)
#define UF2_MAGIC_END           UINT32_C(0x0AB16F30)
#define UF2_FLAG_NOFLASH        UINT32_C(0x00000001)

#define SECTOR_SIZE             512u
#define DISK_SECTORS            8000u
#define SECTORS_PER_FAT         32u
#define ROOT_DIRECTORY_SECTORS  4u
#define START_ROOT_DIRECTORY    (1u + 2u * SECTORS_PER_FAT)
#define START_CLUSTERS          (START_ROOT_DIRECTORY + ROOT_DIRECTORY_SECTORS)
#define CURRENT_UF2_CLUSTER     4u
#define CURRENT_UF2_SECTORS     (FLASH_SIZE / 256u)
#define MAX_UF2_BLOCKS          (FLASH_SIZE / 256u)

#define SCSI_TEST_UNIT_READY    UINT8_C(0x00)
#define SCSI_INQUIRY            UINT8_C(0x12)
#define SCSI_READ_CAPACITY      UINT8_C(0x25)
#define SCSI_READ_10            UINT8_C(0x28)
#define SCSI_WRITE_10           UINT8_C(0x2A)

typedef struct __attribute__((packed)) {
    uint32_t magic_start0;
    uint32_t magic_start1;
    uint32_t flags;
    uint32_t target_address;
    uint32_t payload_size;
    uint32_t block_number;
    uint32_t number_of_blocks;
    uint32_t family_or_size;
    uint8_t data[476];
    uint32_t magic_end;
} uf2_block_t;

typedef struct __attribute__((packed)) {
    uint8_t name[11];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t access_date;
    uint16_t high_cluster;
    uint16_t update_time;
    uint16_t update_date;
    uint16_t start_cluster;
    uint32_t size;
} directory_entry_t;

static uint8_t io_buffer[SECTOR_SIZE];
static uint8_t uf2_written[MAX_UF2_BLOCKS / 8u];
static uint32_t expected_uf2_blocks;
static uint32_t received_uf2_blocks;
static bool reset_pending;

static void uart_poll_identification(void);

static const char info_file[] =
    "UF2 Bootloader CEmu free 1.0\r\n"
    "Model: TI-Python compatible\r\n"
    "Board-ID: TI Python\r\n";

static const char index_file[] =
    "<!doctype html>\n<html><body>CEmu free UF2 bootloader</body></html>\n";

static void copy_bytes(void *destination, const void *source, size_t size) {
    uint8_t *dest = destination;
    const uint8_t *src = source;
    while (size--) {
        *dest++ = *src++;
    }
}

static void fill_bytes(void *destination, uint8_t value, size_t size) {
    uint8_t *dest = destination;
    while (size--) {
        *dest++ = value;
    }
}

static size_t string_length(const char *text) {
    size_t size = 0;
    while (text[size]) {
        ++size;
    }
    return size;
}

static uint16_t read_be16(const uint8_t *data) {
    return (uint16_t)((uint16_t)data[0] << 8 | data[1]);
}

static uint32_t read_le32(const uint8_t *data) {
    return (uint32_t)data[0] |
           (uint32_t)data[1] << 8 |
           (uint32_t)data[2] << 16 |
           (uint32_t)data[3] << 24;
}

static uint32_t read_be32(const uint8_t *data) {
    return (uint32_t)data[0] << 24 |
           (uint32_t)data[1] << 16 |
           (uint32_t)data[2] << 8 |
           (uint32_t)data[3];
}

static void write_le16(uint8_t *data, uint16_t value) {
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
}

static void write_le32(uint8_t *data, uint32_t value) {
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
}

static void write_be32(uint8_t *data, uint32_t value) {
    data[0] = (uint8_t)(value >> 24);
    data[1] = (uint8_t)(value >> 16);
    data[2] = (uint8_t)(value >> 8);
    data[3] = (uint8_t)value;
}

static void spi_begin_transaction(void) {
    SERCOM_CTRLB(SERCOM0_BASE) |= SERCOM_SPI_RXEN;
    SERCOM_INTFLAG(SERCOM0_BASE) = SERCOM_INTFLAG_TXC;
    while (!(SERCOM_INTFLAG(SERCOM0_BASE) & SERCOM_INTFLAG_SSL)) {
        uart_poll_identification();
    }
    SERCOM_INTFLAG(SERCOM0_BASE) = SERCOM_INTFLAG_SSL;
}

static size_t spi_receive(uint8_t *data, size_t capacity) {
    size_t received = 0;
    spi_begin_transaction();
    for (;;) {
        uart_poll_identification();
        uint8_t flags = SERCOM_INTFLAG(SERCOM0_BASE);
        if (flags & SERCOM_INTFLAG_RXC) {
            uint8_t value = (uint8_t)SERCOM_DATA(SERCOM0_BASE);
            if (received < capacity) {
                data[received] = value;
            }
            ++received;
        }
        if (flags & SERCOM_INTFLAG_TXC) {
            SERCOM_INTFLAG(SERCOM0_BASE) = SERCOM_INTFLAG_TXC;
            return received;
        }
    }
}

static size_t spi_send(const uint8_t *data, size_t size) {
    size_t sent = 0;
    if (size) {
        while (!(SERCOM_INTFLAG(SERCOM0_BASE) & SERCOM_INTFLAG_DRE)) {}
        SERCOM_DATA(SERCOM0_BASE) = data[sent++];
    }
    spi_begin_transaction();
    for (;;) {
        uint8_t flags = SERCOM_INTFLAG(SERCOM0_BASE);
        if (flags & SERCOM_INTFLAG_RXC) {
            (void)SERCOM_DATA(SERCOM0_BASE);
        }
        if (flags & SERCOM_INTFLAG_TXC) {
            SERCOM_INTFLAG(SERCOM0_BASE) = SERCOM_INTFLAG_TXC;
            return sent;
        }
        if ((flags & SERCOM_INTFLAG_DRE) && sent < size) {
            SERCOM_DATA(SERCOM0_BASE) = data[sent++];
        }
    }
}

static void spi_initialize(void) {
    REG32(PM_APBCMASK) |= UINT32_C(1) << 2;
    REG16(GCLK_CLKCTRL) = GCLK_CLKCTRL_ID(GCLK_ID_SERCOM0_CORE) |
                          GCLK_CLKCTRL_GEN(GCLK_GEN_0) | GCLK_CLKCTRL_CLKEN;
    SERCOM_CTRLA(SERCOM0_BASE) = SERCOM_CTRLA_SWRST;
    SERCOM_CTRLA(SERCOM0_BASE) = SERCOM_SPI_MODE_SLAVE |
                                  SERCOM_CTRLA_RUNSTDBY | SERCOM_SPI_DOPO(1);
    SERCOM_CTRLB(SERCOM0_BASE) = SERCOM_SPI_PLOADEN | SERCOM_SPI_SSDE;
    SERCOM_CTRLA(SERCOM0_BASE) |= SERCOM_CTRLA_ENABLE;
}

static void uart_initialize(void) {
    REG32(PM_APBCMASK) |= UINT32_C(1) << 5;
    REG16(GCLK_CLKCTRL) = GCLK_CLKCTRL_ID(GCLK_ID_SERCOM3_CORE) |
                          GCLK_CLKCTRL_GEN(GCLK_GEN_0) | GCLK_CLKCTRL_CLKEN;
    SERCOM_CTRLA(SERCOM3_BASE) = SERCOM_CTRLA_SWRST;
    SERCOM_CTRLA(SERCOM3_BASE) = SERCOM_USART_MODE_INT |
                                  SERCOM_USART_RXPO(1) | SERCOM_CTRLA_DORD;
    SERCOM_CTRLB(SERCOM3_BASE) = SERCOM_USART_TXEN | SERCOM_USART_RXEN;
    SERCOM_BAUD(SERCOM3_BASE) = UINT16_C(0xF62B);
    SERCOM_CTRLA(SERCOM3_BASE) |= SERCOM_CTRLA_ENABLE;
}

static void uart_poll_identification(void) {
    if (SERCOM_INTFLAG(SERCOM3_BASE) & SERCOM_INTFLAG_RXC) {
        uint8_t value = (uint8_t)SERCOM_DATA(SERCOM3_BASE);
        if (value == UINT8_C(0x14)) {
            static const uint8_t identity[] = { 'P', 'O', 'B' };
            for (size_t i = 0; i < sizeof(identity); ++i) {
                while (!(SERCOM_INTFLAG(SERCOM3_BASE) & SERCOM_INTFLAG_DRE)) {}
                SERCOM_DATA(SERCOM3_BASE) = identity[i];
            }
        }
    }
}

static bool application_valid(void) {
    const uint32_t *vectors = (const uint32_t *)APP_START_ADDRESS;
    uint32_t stack = vectors[0];
    uint32_t entry = vectors[1];
    return stack >= RAM_START_ADDRESS && stack <= RAM_END_ADDRESS &&
           (entry & 1u) && entry >= APP_START_ADDRESS && entry < FLASH_SIZE;
}

__attribute__((noreturn))
static void start_application(void) {
    const uint32_t *vectors = (const uint32_t *)APP_START_ADDRESS;
    uint32_t stack = vectors[0];
    uint32_t entry = vectors[1];
    SCB_VTOR = APP_START_ADDRESS;
    __asm volatile("msr msp, %0\n"
                   "bx %1\n" :: "r"(stack), "r"(entry) : "memory");
    __builtin_unreachable();
}

static void nvm_wait_ready(void) {
    while (!(NVMCTRL_INTFLAG & NVMCTRL_READY)) {}
}

static void nvm_command(uint16_t command) {
    nvm_wait_ready();
    NVMCTRL_CTRLA = NVMCTRL_COMMAND(command);
    nvm_wait_ready();
}

static void flash_write_row(uint32_t address, const uint8_t *data) {
    NVMCTRL_CTRLB |= NVMCTRL_MANW;
    NVMCTRL_ADDR = address >> 1;
    nvm_command(NVMCTRL_ERASE_ROW);
    for (uint32_t page = 0; page < 4; ++page) {
        nvm_command(NVMCTRL_CLEAR_BUFFER);
        volatile uint32_t *destination = (volatile uint32_t *)(address + page * 64u);
        const uint8_t *source = data + page * 64u;
        for (uint32_t word = 0; word < 16; ++word) {
            destination[word] = read_le32(source + word * 4u);
        }
        NVMCTRL_ADDR = (address + page * 64u) >> 1;
        nvm_command(NVMCTRL_WRITE_PAGE);
    }
}

static void padded_name(uint8_t destination[11], const char *name) {
    size_t index = 0;
    while (index < 11u) {
        destination[index] = name[index] ? (uint8_t)name[index] : (uint8_t)' ';
        if (name[index]) {
            ++index;
        } else {
            while (++index < 11u) {
                destination[index] = (uint8_t)' ';
            }
        }
    }
}

static void make_directory_entry(directory_entry_t *entry, const char *name,
                                 uint8_t attributes, uint16_t cluster,
                                 uint32_t size) {
    fill_bytes(entry, 0, sizeof(*entry));
    padded_name(entry->name, name);
    entry->attributes = attributes;
    entry->creation_date = UINT16_C(0x4D99);
    entry->update_date = UINT16_C(0x4D99);
    entry->start_cluster = cluster;
    entry->size = size;
}

static void read_sector(uint32_t sector, uint8_t data[SECTOR_SIZE]) {
    fill_bytes(data, 0, SECTOR_SIZE);
    if (sector == 0u) {
        data[0] = UINT8_C(0xEB);
        data[1] = UINT8_C(0xFE);
        data[2] = UINT8_C(0x90);
        copy_bytes(data + 3, "MSDOS5.0", 8);
        write_le16(data + 11, SECTOR_SIZE);
        data[13] = 1;
        write_le16(data + 14, 1);
        data[16] = 2;
        write_le16(data + 17, 64);
        write_le16(data + 19, DISK_SECTORS - 2u);
        data[21] = UINT8_C(0xF8);
        write_le16(data + 22, SECTORS_PER_FAT);
        write_le16(data + 24, 1);
        write_le16(data + 26, 1);
        data[38] = UINT8_C(0x29);
        write_le32(data + 39, UINT32_C(0x00420042));
        copy_bytes(data + 43, "TIBOOTPYOB", 11);
        copy_bytes(data + 54, "FAT16   ", 8);
        data[510] = UINT8_C(0x55);
        data[511] = UINT8_C(0xAA);
        return;
    }
    if (sector < START_ROOT_DIRECTORY) {
        uint32_t fat_sector = sector - 1u;
        if (fat_sector >= SECTORS_PER_FAT) {
            fat_sector -= SECTORS_PER_FAT;
        }
        if (fat_sector == 0u) {
            data[0] = UINT8_C(0xF8);
            for (uint32_t index = 1; index < 10; ++index) {
                data[index] = UINT8_C(0xFF);
            }
        }
        for (uint32_t index = 0; index < 256u; ++index) {
            uint32_t cluster = fat_sector * 256u + index;
            if (cluster >= CURRENT_UF2_CLUSTER &&
                cluster < CURRENT_UF2_CLUSTER + CURRENT_UF2_SECTORS) {
                uint16_t next = cluster + 1u == CURRENT_UF2_CLUSTER + CURRENT_UF2_SECTORS
                    ? UINT16_C(0xFFFF) : (uint16_t)(cluster + 1u);
                write_le16(data + index * 2u, next);
            }
        }
        return;
    }
    if (sector < START_CLUSTERS) {
        if (sector == START_ROOT_DIRECTORY) {
            directory_entry_t *entries = (directory_entry_t *)data;
            make_directory_entry(&entries[0], "TIBOOTPYOB", UINT8_C(0x28), 0, 0);
            make_directory_entry(&entries[1], "INFO_UF2TXT", 0, 2,
                                 (uint32_t)string_length(info_file));
            make_directory_entry(&entries[2], "INDEX   HTM", 0, 3,
                                 (uint32_t)string_length(index_file));
            make_directory_entry(&entries[3], "CURRENT UF2", 0, CURRENT_UF2_CLUSTER,
                                 FLASH_SIZE * 2u);
        }
        return;
    }

    uint32_t file_sector = sector - START_CLUSTERS;
    if (file_sector == 0u) {
        copy_bytes(data, info_file, string_length(info_file));
    } else if (file_sector == 1u) {
        copy_bytes(data, index_file, string_length(index_file));
    } else {
        uint32_t address = (file_sector - 2u) * 256u;
        if (address < FLASH_SIZE) {
            uf2_block_t *block = (uf2_block_t *)data;
            block->magic_start0 = UF2_MAGIC_START0;
            block->magic_start1 = UF2_MAGIC_START1;
            block->target_address = address;
            block->payload_size = 256;
            block->block_number = address / 256u;
            block->number_of_blocks = FLASH_SIZE / 256u;
            copy_bytes(block->data, (const void *)address, 256);
            block->magic_end = UF2_MAGIC_END;
        }
    }
}

static void accept_uf2_block(const uint8_t data[SECTOR_SIZE]) {
    const uf2_block_t *block = (const uf2_block_t *)data;
    if (block->magic_start0 != UF2_MAGIC_START0 ||
        block->magic_start1 != UF2_MAGIC_START1 ||
        block->magic_end != UF2_MAGIC_END ||
        block->payload_size != 256u) {
        return;
    }

    if (!(block->flags & UF2_FLAG_NOFLASH) &&
        !(block->target_address & 255u) &&
        block->target_address >= APP_START_ADDRESS &&
        block->target_address <= FLASH_SIZE - 256u) {
        flash_write_row(block->target_address, block->data);
    }
    if (block->number_of_blocks && block->number_of_blocks <= MAX_UF2_BLOCKS &&
        block->block_number < block->number_of_blocks) {
        if (!expected_uf2_blocks) {
            expected_uf2_blocks = block->number_of_blocks;
        }
        if (expected_uf2_blocks == block->number_of_blocks) {
            uint32_t byte = block->block_number >> 3;
            uint8_t bit = (uint8_t)(1u << (block->block_number & 7u));
            if (!(uf2_written[byte] & bit)) {
                uf2_written[byte] |= bit;
                ++received_uf2_blocks;
            }
            if (received_uf2_blocks >= expected_uf2_blocks) {
                reset_pending = true;
            }
        }
    }
}

static void send_ready(uint8_t command) {
    uint8_t value = 0;
    (void)spi_send(&value, 1);
    value = (uint8_t)(command + 1u);
    (void)spi_send(&value, 1);
}

static void send_csw(const uint8_t cbw[31], uint8_t status, uint32_t residue) {
    uint8_t csw[13];
    fill_bytes(csw, 0, sizeof(csw));
    copy_bytes(csw, "USBS", 4);
    copy_bytes(csw + 4, cbw + 4, 4);
    write_le32(csw + 8, residue);
    csw[12] = status;
    send_ready(cbw[15]);
    (void)spi_send(csw, sizeof(csw));
}

static void handle_cbw(const uint8_t cbw[31]) {
    uint8_t command = cbw[15];
    uint32_t transfer_size = read_le32(cbw + 8);
    uint32_t transferred = 0;
    uint8_t status = 0;

    if (read_le32(cbw) != UINT32_C(0x43425355)) {
        return;
    }

    if (command == SCSI_TEST_UNIT_READY && transfer_size == 0u) {
        /* Nothing to transfer. */
    } else if (command == SCSI_INQUIRY && (cbw[12] & UINT8_C(0x80))) {
        static const uint8_t inquiry[36] = {
            0x00, 0x80, 0x02, 0x02, 0x20, 0x00, 0x00, 0x00,
            'T', 'I', ' ', ' ', ' ', ' ', ' ', ' ',
            'P', 'y', 't', 'h', 'o', 'n', ' ', ' ',
            ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
            '1', '.', '1', '1',
        };
        uint32_t size = transfer_size < sizeof(inquiry) ? transfer_size : sizeof(inquiry);
        send_ready(command);
        transferred = (uint32_t)spi_send(inquiry, size);
    } else if (command == SCSI_READ_CAPACITY && (cbw[12] & UINT8_C(0x80))) {
        uint8_t capacity[8];
        write_be32(capacity, DISK_SECTORS - 1u);
        write_be32(capacity + 4, SECTOR_SIZE);
        send_ready(command);
        transferred = (uint32_t)spi_send(capacity, sizeof(capacity));
    } else if (command == SCSI_READ_10 && (cbw[12] & UINT8_C(0x80))) {
        uint32_t sector = read_be32(cbw + 17);
        uint16_t count = read_be16(cbw + 22);
        send_ready(command);
        for (uint16_t index = 0; index < count; ++index) {
            read_sector(sector + index, io_buffer);
            transferred += (uint32_t)spi_send(io_buffer, sizeof(io_buffer));
        }
    } else if (command == SCSI_WRITE_10 && !(cbw[12] & UINT8_C(0x80))) {
        uint16_t count = read_be16(cbw + 22);
        send_ready(command);
        for (uint16_t index = 0; index < count; ++index) {
            size_t size = spi_receive(io_buffer, sizeof(io_buffer));
            if (size == sizeof(io_buffer)) {
                accept_uf2_block(io_buffer);
                transferred += sizeof(io_buffer);
            } else {
                status = 1;
            }
        }
    } else {
        status = 1;
    }

    send_csw(cbw, status, transfer_size > transferred ? transfer_size - transferred : 0u);
}

static void handle_setup(const uint8_t setup[8]) {
    if (setup[0] == UINT8_C(0xA1) && setup[1] == UINT8_C(0xFE)) {
        uint8_t value = 0;
        (void)spi_send(&value, 1);
        value = UINT8_C(0xFF);
        (void)spi_send(&value, 1);
        value = 0;
        (void)spi_send(&value, 1);
    }
}

static void process_spi_command(void) {
    uint8_t packet[31];
    size_t size = spi_receive(packet, sizeof(packet));
    if (size != 1u || packet[0] != UINT8_C(0xA5)) {
        return;
    }

    uint8_t response = 0;
    (void)spi_send(&response, 1);
    response = UINT8_C(0x5A);
    (void)spi_send(&response, 1);

    size = spi_receive(packet, sizeof(packet));
    if (size == sizeof(packet)) {
        handle_cbw(packet);
    } else if (size == 8u) {
        handle_setup(packet);
    }
}

static void request_system_reset(void) {
    for (volatile uint32_t delay = 0; delay < UINT32_C(200000); ++delay) {}
    SCB_AIRCR = SCB_AIRCR_RESET;
    for (;;) {}
}

void bootloader_main(void) {
    volatile uint32_t *double_tap = (volatile uint32_t *)DOUBLE_TAP_ADDRESS;
    bool requested = *double_tap == DOUBLE_TAP_MAGIC;
    if (requested) {
        *double_tap = 0;
    }
    if (application_valid() && !requested) {
        start_application();
    }

    spi_initialize();
    uart_initialize();
    for (;;) {
        process_spi_command();
        uart_poll_identification();
        if (reset_pending) {
            request_system_reset();
        }
    }
}
