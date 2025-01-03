//
// Created by Adithiya Venkatakrishnan on 24/07/2024.
//

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <modules/modules.h>
#include <interrupt/isr.h>

void reboot();

void panic(char* reason);
void interrupt_panic(int code, char* reason, const struct registers* registers);

#endif //EXCEPTION_H
