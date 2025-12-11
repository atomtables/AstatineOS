// write to a file or to a device
// 0 is stdin, 1 is stdout, 2 is stderr
#include "calls.h"
#include <modules/strings.h>
#include <interrupt/isr.h>
#include <display/simple/display.h>

// Unlike a string print, write should not stop at null bytes.
// Input should be sanitised before being sent, otherwise users may
// get weird output.
int read(struct registers* regs) {
    if (regs->ebx < (sizeof(open_fds) / sizeof(struct fd))) {
        struct fd* filedesc = &open_fds[regs->ebx];
        if (!filedesc->exists) {
            printf("Attempted to read from non-existent fd %d\n", regs->ebx);
            return -1; // fd does not exist
        }
        if (filedesc->fops->read == null) {
            printf("Read not supported on this fd %d\n", regs->ebx);
            return -1; // read not supported
        }
        return filedesc->fops->read(filedesc, (const void*)regs->ecx, (u32)regs->edx);
    } else {
        // For this temporary implementation we will only allow 256 open file descriptors.
        return -1; // invalid fd
    }
}