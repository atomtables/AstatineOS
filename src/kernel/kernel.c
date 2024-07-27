#include <idt/interrupt.h>
#include <memory/memory.h>

#include "display/display.h"

/* In our kernel, we can reserve memory
 * 0x100000-0x1FFFFF for the storage of heap data (like variables)
 * Store an array for 32-byte increments
*/

// YO THIS GUY ONLINE WAS ACT LEGIT :skull:
int main() {
    INIT_INTERRUPTS();

    for (int i = 0; i < 1000000000; i++) { asm ("nop"); }

    init_mem();
    clear_screen();

    printf("Welcome to NetworkOS... Data: %d\n", 1234);


    for (;;);
}
