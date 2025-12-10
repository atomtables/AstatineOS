#include <modules/strings.h>
#include <interrupt/isr.h>
#include <display/simple/display.h>
#include "calls.h"
#include <terminal/terminal.h>

int open(struct registers* regs) {
    // we need to create a new FD
    if (regs->ebx == null) {
        regs->eax = -1; // invalid pointer
        return 0;
    }
    char* identifier = (char*)regs->ebx;
    if (strcmp(identifier, "teletype") == 0) {
        for (u32 i = 0; i < (sizeof(open_fds) / sizeof(struct fd)); i++) {
            if (!open_fds[i].exists) {
                if (teletype_fops.open == null) {
                    regs->eax = -1; // open not supported
                    return 0;
                }
                struct fd* new_fd = &open_fds[i];
                int err = teletype_fops.open(new_fd, identifier, (u8)regs->edx);
                if (err != 0) {
                    regs->eax = err; // error code
                    return 0;
                }
                regs->eax = i; // return fd index
                return 0;
            }
        }
        regs->eax = -1; // no free fds
        return 0;
    }
}