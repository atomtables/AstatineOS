#include <exception/exception.h>
#include <idt/interrupt.h>
#include <memory/memory.h>

#include "display/display.h"

/* In our kernel, we can reserve memory
 * 0x100000-0x1FFFFF for the storage of heap data (like variables)
 * Store an array for 32-byte increments
*/

// YO THIS GUY ONLINE WAS ACT LEGIT :skull:
int main() {
    idt_init();
    isr_init();
    irq_init();

    init_mem();
    clear_screen();

    int eax = 0;
    while(true) {
        printf("Welcome to NetworkOS... Data: %d\n", eax);
        eax += 1;

        if (eax > 1200) {
            asm ("int3");
        }
    }

}
