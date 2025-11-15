#include "calls.h"
#include <interrupt/isr.h>

void abort(struct registers* regs) {
    // Crash the system to abort the current process
    // sends breakpoint interrupt which crashes the system.
    __asm__ volatile("int3");
}