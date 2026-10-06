extern volatile unsigned reg_input;

__attribute__((noinline)) unsigned iy_leaf(unsigned value) {
    volatile unsigned local = value ^ 0x13579b;
    __asm__ volatile(".globl _iy_frame_checkpoint\n_iy_frame_checkpoint:"
                     : : : "memory");
    return local + 3;
}

__attribute__((noinline)) unsigned reg_leaf(void) {
    unsigned value = reg_input;
    __asm__ volatile(".globl _reg_frame_checkpoint\n_reg_frame_checkpoint:"
                     : "+r"(value));
    return value ^ 0xabcdef;
}
