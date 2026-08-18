MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 8K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 32K
}

/* The top SRAM word is the double-tap reset marker and must not be used by
 * the stack before bootloader_main() has inspected it. */
_stack_top = ORIGIN(RAM) + LENGTH(RAM) - 4;

SECTIONS
{
    .vectors :
    {
        KEEP(*(.vectors))
    } > FLASH

    .text :
    {
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .data : AT(_etext)
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM
    _sidata = LOADADDR(.data);

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /DISCARD/ :
    {
        *(.comment*)
        *(.ARM.attributes*)
    }
}

ASSERT(_etext + SIZEOF(.data) <= ORIGIN(FLASH) + LENGTH(FLASH),
       "free bootloader exceeds the 8 KiB boot region")
