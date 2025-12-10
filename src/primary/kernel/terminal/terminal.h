#ifndef TERMINAL_H
#define TERMINAL_H

#include <systemcalls/calls/calls.h>

void terminal_install(void);
int terminal_write(struct fd* self, const void* buffer, u32 size);

extern struct fop teletype_fops;

#endif