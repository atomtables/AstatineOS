#include <display/advanced/graphics.h>
#include <exception/exception.h>
#include <fpu/fpu.h>
#include <fungame/fungame.h>
#include <interrupt/interrupt.h>
#include <ps2/keyboard.h>
#include <memory/memory.h>
#include <modules/strings.h>
#include <pcspeaker/pcspeaker.h>
#include <ps2/controller.h>
#include <timer/PIT.h>

#include "display/simple/display.h"

/* In our kernel, we can reserve memory
 * 0x100000-0x1FFFFF for the storage of heap data (like variables)
 * Store an array for 32-byte increments
*/

// YO THIS GUY ONLINE WAS ACT LEGIT :skull:

u32 get_conventional_memory_kb() {
    return *(u32*)0x413;
}

u64 get_extended_memory_kb() {
    return *(u32*)0x15 * 64;
}

void handler(struct registers* regs) {
    printf("int48\n");
}

extern void ahsh();

// only blocking thread.
int main() {
    // about the only thing set up here is the GDT and the text-mode interface. we can use printf before init everything else.
    clear_screen();

    draw_string(35, 10, "NetworkOS");
    draw_string(34, 11, "Booting...");

    idt_init();
    isr_init();
    PIC_init();


    init_mem();

    fpu_init();
    timer_init();
    pcs_init();

    ps2_controller_init();
    keyboard_init();

    beep();
    sleep(500);

    printf("NetworkOS Kernel v0.1\n");
    printf("booting with %u bytes of memory\n", get_extended_memory_kb());

    asm ("xchg %bx, %bx");

    ahsh();

    while(1);
}
