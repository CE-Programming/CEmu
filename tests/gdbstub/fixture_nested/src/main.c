#include <tice.h>

volatile unsigned nested_value;

__attribute__((noinline)) unsigned inner(unsigned value) {
    nested_value = value ^ 0x13579b;
    return nested_value + 3;
}

__attribute__((noinline)) unsigned outer(unsigned value) {
    volatile unsigned saved = value + 7;
    return inner(saved) + saved;
}

__attribute__((noinline)) unsigned long wide_return(unsigned long value) {
    return value ^ 0x89abcdefUL;
}

int main(void) {
    os_ClrHome();
    os_PutStrFull("Nested GDB test");
    unsigned result = outer(0x2468ac);
    unsigned long wide = wide_return(0x12345678UL);
    unsigned long forced = wide_return(0);
    os_NewLine();
    os_PutStrFull(result == 0x5ba7de && wide == 0x9b9f9b97UL && forced == 0x89abcdefUL ? "PASS" : "FAIL");
    while (!os_GetCSC()) {}
    return 0;
}
