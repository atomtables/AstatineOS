//
// Created by Adithiya Venkatakrishnan on 24/07/2024.
//

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <modules/modules.h>
#include <idt/isr.h>

void fatal_error(int code, char* reason, void* function, u32** registers);
void interrupt_panic(int code, char* reason, const struct registers* registers);

#endif //EXCEPTION_H
