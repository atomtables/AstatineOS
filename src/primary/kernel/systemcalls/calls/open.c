#include <modules/strings.h>
#include <interrupt/isr.h>
#include <display/simple/display.h>
#include "calls.h"

void open(struct registers* regs) {
    if (regs->ebx < (sizeof(open_fds) / sizeof(struct fd))) {

        // return filedesc->fops->open(filedesc);
    } else {
        // For this temporary implementation we will only allow 256 open file descriptors.
        return; // invalid fd
    }
}