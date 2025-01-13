//
// Created by Adithiya Venkatakrishnan on 24/07/2024.
//

#include "exception.h"

#include <display/advanced/graphics.h>
#include <display/simple/display.h>
#include <interrupt/isr.h>
#include <timer/PIT.h>

void reboot() {
    u8 good = 0x02;
    while (good & 0x02)
        good = inportb(0x64);
    outportb(0x64, 0xFE);
    asm ("hlt");
}

void panic(char* reason) {
    display.clear_screen();

    disable_vga_cursor();

    display.change_screen_color(0x1f);
    display.print("\n\n\n\n\n\n\n\n");
    display.print("                            ");
    display.print_color(" NetworkOS Fatal Error ", 0x71);
    display.print("\n\n");
    // print ("                                                                                ");
    display.printf("       An exception has resulted in a KERNEL PANIC.\n");
    display.printf("       This error was caused by:\n\n");
    display.printf("       %s", reason);
    display.printf("\n\n       Press ENTER to restart. The system will restart in 5 seconds.\n\n");

    // sleep(5000);
    reboot();
}



void interrupt_panic(const int code, char* reason, const struct registers* registers) {
    int* eip = (int*)registers->eip;

    display.clear_screen();

    disable_vga_cursor();

    u32** eipS = (u32**)((u32)eip / 16 * 16 - 0x20);

    display.change_screen_color(0x1f);
    display.print("\n\n");
    display.print("                            ");
    display.print_color(" NetworkOS Fatal Error ", 0x71);
    display.print("\n\n");
    display.printf("       A system interrupt has resulted in a fatal error that cannot\n");
    display.printf("       be recovered from. NetworkOS has shut down to prevent further \n");
    display.printf("       damage to the system. The exception was:\n\n");
    display.printf("       INTNO %d: %s\n\n", code, reason);
    display.printf("       Press ENTER to restart. The system will restart in 2 seconds.\n\n");
    display.printf("       Developer/Technical Information:\n\n");
    display.printf("       EIP:%p, EFL:%p, USERESP:%p, ERRNO:%d\n", (void*)registers->eip, (void*)registers->efl, (void*)registers->useresp, registers->err_no);
    display.printf("       EAX:%p, EBX:%p, ECX:%p, EDX:%p\n", (void*)registers->eax, (void*)registers->ebx, (void*)registers->ecx, (void*)registers->edx);
    display.printf("       ESI:%p, EDI:%p, EBP:%p, ESP:%p\n\n", (void*)registers->esi, (void*)registers->edi, (void*)registers->ebp, (void*)registers->esp);
    display.printf("       Memory Dump around EIP:\n");
    display.printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    display.printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    display.printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    display.printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    display.printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));

    STI();
    // sleep(2000);
    reboot();
}
