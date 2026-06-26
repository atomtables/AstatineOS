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

#define COM1_PORT 0x3F8
/* Serial Port Hardware Check Macros */
#define SERIAL_IS_TX_EMPTY() (inportb((COM1_PORT) + 5) & 0x20)
/* High-level Write Macros */
#define SERIAL_PUTC(c) do { \
while (!SERIAL_IS_TX_EMPTY()); \
outportb(COM1_PORT, c); \
} while(0)
#define SERIAL_PRINT(str) do { \
const char* __s = (str); \
while (*__s) { \
SERIAL_PUTC(*__s); \
__s++; \
} \
} while(0)

void reboot();

void panicking(char* reason);
void interrupt_panic(int code, char* reason, const struct registers* registers);

#endif //EXCEPTION_H
