# CEmu TI-Python-compatible SAMD21 bootloader

This directory builds the freely redistributable bootloader that CEmu installs
in an otherwise empty Python coprocessor flash. It replaces the separate 8 KiB
TI bootloader dump; the Python application firmware is still uploaded by the
calculator's installed Python App.

The UF2 format, virtual FAT layout, flash-writing behavior, application handoff,
and double-reset ABI are derived from Microsoft's MIT-licensed `uf2-samdx1`
bootloader, whose license and bundled Atmel notice are retained in `LICENSE`:

https://github.com/microsoft/uf2-samdx1

TI's calculator-facing transport carries USB mass-storage CBW, data, and CSW
transactions over the SAMD21's SERCOM0 SPI slave. The implementation here is
based on independently observed traffic between the CE OS and the coprocessor.
It contains no bytes copied from TI's bootloader.

Supported calculator-facing operations are the `0xA5`/`0x5A` handshake,
USB mass-storage CBW/CSW framing, GET MAX LUN, TEST UNIT READY, INQUIRY,
READ CAPACITY, READ(10), WRITE(10), and the UART `0x14` -> `POB`
identification response. This intentionally implements only what the CE OS and
Python App need to install and start the application firmware.

Run `make` in this directory to rebuild `free_bootloader.bin` and the generated
`../free_bootloader_image.h`. The checked-in header lets normal CEmu builds
work without requiring an ARM cross compiler. `make clean` removes only local
compiler outputs and deliberately keeps that checked-in header.
