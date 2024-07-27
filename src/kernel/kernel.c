#include <exception/exception.h>
#include <memory/memory.h>

#include "display/display.h"

/* In our kernel, we can reserve memory
 * 0x100000-0x1FFFFF for the storage of heap data (like variables)
 * Store an array for 32-byte increments
*/

// YO THIS GUY ONLINE WAS ACT LEGIT :skull:
int main() {
    clear_screen();

    for (int i = 0; i < 1000000000; i++) {
        asm ("nop");
    }

    u32* registers[8]; get_registers(registers);
    fatal_error(0, "Testing Fatal Error Screen", (void*)main, &registers[0]);

    init_mem();
    clear_screen();

    printf("Welcome to NetworkOS... Data: %x\n", 0x1234);
    int ptr1 = (int)malloc(10);
    *(char*)ptr1 = 'h';
    *(char*)(ptr1+1) = 'i';
    int ptr2 = (int)malloc(15);
    *(char*)ptr2 = 'h';
    *(char*)(ptr2+1) = 'i';
    int ptr3 = (int)malloc(35);
    *(char*)ptr3 = 'h';
    *(char*)(ptr3+1) = 'i';
    int ptr4 = (int)malloc(10);
    *(char*)ptr4 = 'h';
    *(char*)(ptr4+1) = 'i';
    printf("Pointers: ptr1=%p, ptr2=%x, ptr3=%x, ptr4=%x", (void*)ptr1, ptr2, ptr3, ptr4);


    for(;;);
}