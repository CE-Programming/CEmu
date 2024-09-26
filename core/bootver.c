#include "bootver.h"

static const boot_ver_t minRevA_CE = {   5,   0,   0,     0 }; /* Rev A */
static const boot_ver_t minRevI_CE = {   5,   0,   0,     0 }; /* Rev I */
static const boot_ver_t minRevM_CE = {   5,   3,   6,     0 }; /* Rev M */
static const boot_ver_t minRevM_82 = {   5,   6,   3,     0 }; /* Rev pre-A of 82AEP, but otherwise the same JB-007 ASIC as M on CE */
static const boot_ver_t maxRevA_CE = {   5,   3,   5, 65535 }; /* Rev A */
static const boot_ver_t maxRevI_CE = {   5,   3,   5, 65535 }; /* Rev I */
static const boot_ver_t maxRev     = { 255, 255, 255, 65535 }; /* Rev M */

/* NULL means unsupported */
static const boot_ver_t* asic_min_ver_8384CE[] = { &minRevA_CE, &minRevI_CE, &minRevM_CE };
static const boot_ver_t* asic_max_ver_8384CE[] = { &maxRevA_CE, &maxRevI_CE, &maxRev };
static const boot_ver_t* asic_min_ver_82AEP[]  = { NULL,        NULL,        &minRevM_82 };
static const boot_ver_t* asic_max_ver_82AEP[]  = { NULL,        NULL,        &maxRev };

static bool parse_entry(const uint8_t* data, uint32_t entry, uint32_t* addr) {
    if (entry + 4 >= SIZE_BOOTCODE) {
        return false;
    }
    data += entry;

    /* JP addr */
    if (data[0] != 0xC3) {
        return false;
    }

    *addr = data[1] | (((uint32_t)data[2]) << 8) | (((uint32_t)data[3]) << 16);
    return true;
}

static bool parse_ver_8(const uint8_t* data, uint32_t addr, uint8_t* ver) {
    if (addr + 3 >= SIZE_BOOTCODE) {
        return false;
    }
    data += addr;

    /* LD A,version; RET */
    if ((data[0] != 0x3E) || (data[2] != 0xC9)) {
        return false;
    }

    *ver = data[1];
    return true;
}

static bool parse_ver_16(const uint8_t* data, uint32_t addr, uint16_t* ver) {
    if (addr + 5 >= SIZE_BOOTCODE) {
        return false;
    }
    data += addr;

    /* LD A,versionHigh; LD B,versionLow; RET */
    if ((data[0] != 0x3E) || (data[2] != 0x06) || (data[4] != 0xC9)) {
        return false;
    }

    *ver = (((uint16_t)data[1]) << 8) | data[3];
    return true;
}

bool bootver_parse(const uint8_t* data, boot_ver_t* boot_ver) {
    uint32_t addr;
    uint16_t major_minor, build;
    uint8_t revision;

    if (!data) {
        return false;
    }

    /* _boot_GetBootVerMajor */
    if (!parse_entry(data, 0x000080, &addr)) {
        return false;
    }
    if (!parse_ver_16(data, addr, &major_minor)) {
        return false;
    }

    /* _boot_GetBootVerMinor */
    if (!parse_entry(data, 0x00008C, &addr)) {
        return false;
    }
    if (!parse_ver_8(data, addr, &revision)) {
        return false;
    }

    /* _boot_GetBootVerBuild */
    if (!parse_entry(data, 0x000090, &addr)) {
        return false;
    }
    if (!parse_ver_16(data, addr, &build)) {
        return false;
    }

    boot_ver->major = (uint8_t)(major_minor >> 8);
    boot_ver->minor = (uint8_t)major_minor;
    boot_ver->revision = revision;
    boot_ver->build = build;
    return true;
}

bool bootver_check_ver(const boot_ver_t* ver, const boot_ver_t* check) {
    if (!ver || !check) {
        return false;
    }
    if (ver->major != check->major) {
        return ver->major > check->major;
    }
    if (ver->minor != check->minor) {
        return ver->minor > check->minor;
    }
    if (ver->revision != check->revision) {
        return ver->revision > check->revision;
    }
    return ver->build >= check->build;
}

bool bootver_check_rev(const boot_ver_t* ver, asic_rev_t rev, emu_device_t device) {
    unsigned int index = (unsigned int)rev - 1;
    if (((device == TI83PCE || device == TI84PCE) && index >= sizeof(asic_min_ver_8384CE) / sizeof(boot_ver_t*)) ||
        ((device == TI82AEP)                      && index >= sizeof(asic_min_ver_82AEP)  / sizeof(boot_ver_t*))) {
        return false;
    }

    if (!ver) {
        return true;
    }

    const boot_ver_t** asic_min_ver = (device == TI83PCE || device == TI84PCE) ? asic_min_ver_8384CE : asic_min_ver_82AEP;
    const boot_ver_t** asic_max_ver = (device == TI83PCE || device == TI84PCE) ? asic_max_ver_8384CE : asic_max_ver_82AEP;
    return bootver_check_ver(ver, asic_min_ver[index])
        && bootver_check_ver(asic_max_ver[index], ver);
}
