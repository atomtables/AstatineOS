//
// Created by Adithiya Venkatakrishnan on 24/07/2024.
//

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <modules/modules.h>
#include <interrupt/isr.h>

#define as_str(x) #x
#define as_str_2(x) as_str(x)
#define panic(reason) panicking(__FILE__ ", " as_str_2(__LINE__) ": " reason)


void reboot();

void panicking(char* reason);
void interrupt_panic(int code, char* reason, const struct registers* registers);

#endif //EXCEPTION_H
