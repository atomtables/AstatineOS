//
// Created by Adithiya Venkatakrishnan on 27/07/2024.
//

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <interrupt/idt.h>
#include <interrupt/PIC.h>
#include <interrupt/isr.h>

#define INIT_INTERRUPTS() idt_init(); isr_init(); PIC_init();

#endif //INTERRUPT_H
