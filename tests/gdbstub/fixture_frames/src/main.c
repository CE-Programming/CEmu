#include <tice.h>

volatile unsigned reg_input = 0x314159;
volatile unsigned frame_result;
extern unsigned iy_leaf(unsigned value);
extern unsigned reg_leaf(void);

__attribute__((noinline)) unsigned ix_outer(unsigned value) {
    volatile unsigned saved = value + 7;
    unsigned first = iy_leaf(saved);
    return first + reg_leaf() + saved;
}

int main(void) {
    os_ClrHome();
    os_PutStrFull("Frame GDB test");
    frame_result = ix_outer(0x2468ac);
    os_NewLine();
    os_PutStrFull(frame_result == 0xf63494 ? "PASS" : "FAIL");
    while (!os_GetCSC()) {}
    return 0;
}
