#include <display/advanced/graphics.h>
#include <exception/exception.h>
#include <fpu/fpu.h>
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

void handler(struct registers* regs) {
    display.printf("int48\n");
}

extern void ahsh();

// only blocking thread.
int main() {
    display.clear_screen();
    display.println_color("AstatineOS v0.1.0-alpha", COLOR_LIGHT_RED);

    u32 mem1 = *((u16*)0x500) * 1000;
    u32 mem2 = *((u16*)0x502) * 64000;
    u32 mem3 = mem1 + mem2;
    display.printf("%u MB of memory detected\n", mem3);

    idt_init();
    isr_init();
    PIC_init();
    display.printf("Target complete: interrupts\n");

    timer_init();
    display.printf("Target complete: timer\n");

    init_mem();
    display.printf("Target complete: memory\n");

    fpu_init();
    display.printf("Target complete: fpu\n");

    pcs_init();
    display.printf("Target complete: pcspeaker\n");

    ps2_controller_init();
    display.printf("Target complete: ps2 controller\n");

    keyboard_init();
    display.printf("Target complete: keyboard\n");

    play_sound(NOTE_D5);
    sleep(500);
    nosound();

    ahsh();

    while(1);
}
