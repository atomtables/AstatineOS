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

    u32 mem1 = *((u16*)0x8000) * 1000;
    u32 mem2 = *((u16*)0x8002) * 64000;
    u32 mem3 = mem1 + mem2;
    display.printf("%u MB of memory detected\n", mem3);

    idt_init();
    isr_init();
    PIC_init();
    display.printf("Target complete: interrupts\n");

    timer_init();
    display.printf("Target complete: timer\n");
    sleep(1000);
    init_mem();
    display.printf("Target complete: memory\n");
    sleep(1000);
    fpu_init();
    display.printf("Target complete: fpu\n");

    sleep(1000);
    pcs_init();
    display.printf("Target complete: pcspeaker\n");

    sleep(1000);
    ps2_controller_init();
    display.printf("Target complete: ps2 controller\n");

    sleep(1000);
    keyboard_init();
    display.printf("Target complete: keyboard\n");

    // clear the 0x100000-0x1FFFFF region to prevent dynamic memory corruption
    sleep(1000);
    display.printf("Setting up dymem region...");
    for (u32* ptr = (u32*)0x100000; ptr < (u32*)0x200000; ptr++) {
        *ptr = 0;
        if ((u32)ptr % 0x1000 == 0) {
            display.printf(".");
            sleep(0.2); // make it seem like it does something
        }
    }
    display.printf("Done\n");

    sleep(1000);
    play_sound(NOTE_D5);
    sleep(500);
    nosound();

    ahsh();

    while(1);
}
