#include <display/simple/display.h>
#include <interrupt/isr.h>
#include "syscall.h"
#include "calls/calls.h"

// sample syscall handler to check out stuff
void handler(struct registers* regs) {
    if (regs->eax < sizeof(syscall_handlers)) {
        // so we just look for any syscall_handlers that are installed.
        syscall_handlers[regs->eax](regs);
    } else {
        return; //-1;
    }
}

void syscall_install() {
    isr_install(48, handler);
}