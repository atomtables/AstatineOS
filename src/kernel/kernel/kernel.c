#include <exception/exception.h>
#include <idt/interrupt.h>
#include <memory/memory.h>
#include <pcspeaker/pcspeaker.h>
#include <timer/PIT.h>

#include "display/display.h"

/* In our kernel, we can reserve memory
 * 0x100000-0x1FFFFF for the storage of heap data (like variables)
 * Store an array for 32-byte increments
*/

// YO THIS GUY ONLINE WAS ACT LEGIT :skull:
int main() {
    idt_init();
    isr_init();
    PIC_init();

    init_mem();
    clear_screen();

    timer_init();
    pcs_init();

    sleep(500);

    beep();

    printf("Welcome to NetworkOS...\n\n");
    printf("NetworkOS is a basic operating system written in C and Assembly.\n");
    printf("It is intended to be used with user-space programs, but mainly for\n");
    printf("network utilities. (Under Development)\n\n");

    sleep(1000);

    void* ptr = malloc(32);
    printf("Allocated 32 bytes at %p\n", ptr);
    sleep(500);
    void* ptr2 = malloc(64);
    printf("Allocated 64 bytes at %p\n", ptr2);
    sleep(500);
    void* ptr3 = malloc(32);
    printf("Allocated 32 bytes at %p\n\n", ptr3);
    sleep(500);

    printf("Dividing by zero in 1000 milliseconds...", ptr3);

    sleep(1000);

    DIV_BY_ZERO()

    while(1);

}
