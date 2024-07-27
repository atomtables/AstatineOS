//
// Created by Adithiya Venkatakrishnan on 27/07/2024.
//

#ifndef ISR_H
#define ISR_H
#include <modules/modules.h>

struct registers {
    u32 __ignored, fs, es, ds;
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
    u32 int_no, err_no;
    u32 eip, cs, efl, useresp, ss;
};

void isr_install(u32 i, void (*handler)(struct registers*));
void isr_init();

#endif //ISR_H
