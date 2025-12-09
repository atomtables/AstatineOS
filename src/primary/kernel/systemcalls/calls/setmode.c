#include "calls.h"
#include <modules/strings.h>
#include <interrupt/isr.h>
#include <display/simple/display.h>

int setmode(struct registers* regs) {
    if (regs->ebx < (sizeof(open_fds) / sizeof(struct fd))) {
        struct fd* filedesc = &open_fds[regs->ebx];
        if (!filedesc->exists) {
            return -1; // fd does not exist
        }
        if (!filedesc->fops || filedesc->fops->setmode == null) {
            return -1; // setmode not supported
        }
        return filedesc->fops->setmode(filedesc, (u8)regs->ecx);
    } else {
        // For this temporary implementation we will only allow 256 open file descriptors.
        return -1; // invalid fd
    }
}