//
// Created by Adithiya Venkatakrishnan on 27/07/2024.
//

#ifndef IRQ_H
#define IRQ_H

#include <interrupt/isr.h>
#include <modules/modules.h>

void PIC_install(u32 i, void (*handler)(struct registers*));
void PIC_init();

#endif //IRQ_H
