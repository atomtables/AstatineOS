#ifndef CALLS_H
#define CALLS_H

#include <interrupt/isr.h>
#include <modules/modules.h>

// End execution of the current process
// stubbed by crashing the system. 
void abort(struct registers* regs);

// Write to a file or device
void write(struct registers* regs);

// File descriptor structure
struct fd {
    u16     type;       // 0 = device, 1 = file
    u8      exists;     // 1 if fd is open;
    u32     id;         // fd number
    u32     position;   // current position in the file
    char*   identifier;
};

extern struct fd open_fds[256];
extern void(* syscall_handlers[500])(struct registers* regs);

#endif