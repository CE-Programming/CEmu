#include <tice.h>

volatile unsigned watched_value;
extern void probe(void);

int main(void) {
    os_ClrHome();
    os_PutStrFull("CEmu GDB test");
    probe();
    os_NewLine();
    os_PutStrFull(watched_value == 0x123456 ? "PASS" : "FAIL");
    while (!os_GetCSC()) {}
    return 0;
}
