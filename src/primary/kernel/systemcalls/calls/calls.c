#include "calls.h"

struct fd open_fds[256];
int(* syscall_handlers[500])(struct registers* regs) = {
    abort, // 0
    write, // 1
    read,  // 2
    setmode, // 3
    open, // 4
    loadnew,
    freenew
};