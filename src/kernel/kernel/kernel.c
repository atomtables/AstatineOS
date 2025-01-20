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
    // about the only thing set up here is the GDT and the text-mode interface. we can use printf before init everything else.
    display.clear_screen();

    u32 mem1 = *((u16*)0x500) * 1000;
    u32 mem2 = *((u16*)0x502) * 64000;
    u32 mem3 = mem1 + mem2;

    draw_string(30, 10, "NetworkOS Kernel v0.1");
    draw_string(33, 12, "Initialising...");
    draw_string(30, 13, "     interrupts     ");

    for (int i = 0; i < 1000000000; i++) {NOP();}

    idt_init();
    isr_init();
    PIC_init();

    draw_string(30, 13, "   internal clock   ");
    timer_init();
    sleep(500);

    draw_string(30, 13, "       memory       ");
    init_mem();
    sleep(500);


    draw_string(30, 13, "   floating point   ");
    fpu_init();
    sleep(500);

    draw_string(30, 13, "        sound       ");
    pcs_init();
    sleep(500);

    draw_string(30, 13, "   ps/2 controller  ");
    ps2_controller_init();
    sleep(500);

    draw_string(30, 13, "      keyboard      ");
    keyboard_init();
    sleep(500);

    display.clear_screen();

    play_sound(NOTE_D5);
    sleep(500);
    nosound();

    display.printf("NetworkOS Kernel v0.1\n");
    display.printf("booting with %u bytes of memory\n", mem3);

    ahsh();

    while(1);
}
