#include "calls.h"
#include <interrupt/isr.h>

int abort(struct registers* regs) {
    // Crash the system to abort the current process
    // sends breakpoint interrupt which crashes the system.
    __asm__ volatile("int3");
    return 0;
}