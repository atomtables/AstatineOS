//
// Created by Adithiya Venkatakrishnan on 27/07/2024.
//

#ifndef IDT_H
#define IDT_H
#include <modules/modules.h>
#include <interrupt/isr.h>

void idt_set(u8 index, void(* base)(struct registers*), u16 selector, u8 flags);
void idt_init();

#endif //IDT_H
