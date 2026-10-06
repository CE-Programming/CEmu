.assume ADL=1
.section .text
.global _probe, _probe_write, _probe_read, _probe_after_read
.type _probe, @function
.cfi_sections .debug_frame
_probe:
    .cfi_startproc
    .cfi_return_column 5
    .cfi_def_cfa 4, 3
    .cfi_offset 5, -3
    ld hl, 0x123456
_probe_write:
    ld (_watched_value), hl
_probe_read:
    ld de, (_watched_value)
_probe_after_read:
    ret
    .cfi_endproc
.size _probe, .-_probe
