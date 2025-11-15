#include <display/simple/display.h>
#include <interrupt/isr.h>
#include "syscall.h"
#include "calls/calls.h"

// sample syscall handler to check out stuff
void handler(struct registers* regs) {
    if (regs->eax < sizeof(syscall_handlers)) {
        syscall_handlers[regs->eax](regs);
    } else {
        return;
    }
}

void syscall_install() {
    isr_install(48, handler);
}