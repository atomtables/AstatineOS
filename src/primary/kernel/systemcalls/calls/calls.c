#include "calls.h"

struct fd open_fds[256];
void(* syscall_handlers[500])(struct registers* regs) = {
    abort, // 0
    write, // 1
};