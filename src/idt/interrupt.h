//
// Created by Adithiya Venkatakrishnan on 27/07/2024.
//

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <idt/idt.h>
#include <idt/irq.h>
#include <idt/isr.h>

#define INIT_INTERRUPTS() idt_init(); isr_init(); irq_init();

#endif //INTERRUPT_H
