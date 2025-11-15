#ifndef SYSCALL_H
#define SYSCALL_H

extern void(* syscall_handlers[500])(struct registers* regs);

void syscall_install();

#endif