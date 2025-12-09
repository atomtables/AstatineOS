#ifndef CALLS_H
#define CALLS_H

#include <interrupt/isr.h>
#include <modules/modules.h>

// End execution of the current process
// stubbed by crashing the system. 
int abort(struct registers* regs);

// Write to a file or device
int write(struct registers* regs);

// Read from a file or device
int read(struct registers* regs);

int setmode(struct registers* regs);

// File descriptor structure
struct fd {
    u8          exists;     // 1 if fd is open;
    u8          mode;
    u32         position;   // current position in file
    void*       internal;   // driver specific data
    struct fop* fops;
};

// File entry structure
// This will pretty much be like an operation table
struct fop {
    int(* read)(struct fd* self, void* buffer, u32 size);
    int(* write)(struct fd* self, const void* buffer, u32 size);
    int(* open)(struct fd* self, char* identifier, u8 mode);
    int(* close)(struct fd* self);
    int(* setmode)(struct fd* self, u8 mode);
};

extern struct fd open_fds[256];
extern int(* syscall_handlers[500])(struct registers* regs);

#endif