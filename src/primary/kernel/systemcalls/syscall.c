#include <display/simple/display.h>
#include <interrupt/isr.h>

// sample syscall handler to check out stuff
void handler(struct registers* regs) {
    switch (regs->eax) {
        case 0: // append char
            char c[2] = { regs->ebx, 0 };
            display.printf(c);
        break;
        case 1: // append string
            display.printf((char*)regs->ebx);
        break;
    }
}

void syscall_install() {
    isr_install(48, handler);
}