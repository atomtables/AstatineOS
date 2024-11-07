//
// Created by Adithiya Venkatakrishnan on 24/07/2024.
//

#include "exception.h"

#include <display/display.h>
#include <idt/isr.h>
#include <timer/PIT.h>

void reboot() {
    u8 good = 0x02;
    while (good & 0x02)
        good = inportb(0x64);
    outportb(0x64, 0xFE);
    asm ("hlt");
}

// works with 32-bit
// is now legacy, just do `asm ("int3")`
void fatal_error(const int code, char* reason, void* function, u32** registers) {
    int* eip = __builtin_return_address(0) - 5;
    // int* early_call = __builtin_return_address(1) - 5;
    int* early_call = 0; // legacy

    clear_screen();

    disable_vga_cursor();

    u32** eipS = (u32**)((u32)eip / 16 * 16 - 0x20);

    change_screen_color(0x1f);
    print("\n\n");
    print("                            ");
    print_color(" NetworkOS Fatal Error ", 0x71);
    print("\n\n");
    // print ("                                                                                ");
    printf("       An exception at %p (caller %p) has resulted in\n", eip, function);
    print("       a fatal error. This error was caused by:\n\n");
    printf("       ERRNO %d: %s", code, reason);
    print("\n\n       Press ENTER to restart. The system will restart in 5 seconds.\n\n");
    print("       Developer/Technical Information:\n\n");
    printf("       EIP: %p -> Caller: %p -> Parent: %p\n", eip, function, early_call);
    printf("       EAX:%p, EBX:%p, ECX:%p, EDX:%p\n", registers[0], registers[1], registers[2], registers[3]);
    printf("       ESI:%p, EDI:%p, EBP:%p, ESP:%p\n\n", registers[4], registers[5], registers[6], registers[7]);
    printf("       Memory Dump around EIP:\n");
    printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));

    sleep(5);
    reboot();
}



void interrupt_panic(const int code, char* reason, const struct registers* registers) {
    int* eip = (int*)registers->eip;

    clear_screen();

    disable_vga_cursor();

    u32** eipS = (u32**)((u32)eip / 16 * 16 - 0x20);

    change_screen_color(0x1f);
    print("\n\n");
    print("                            ");
    print_color(" NetworkOS Fatal Error ", 0x71);
    print("\n\n");
    printf("       A system interrupt has resulted in a fatal error that cannot\n");
    printf("       be recovered from. NetworkOS has shut down to prevent further \n");
    printf("       damage to the system. The exception was:\n\n");
    printf("       INTNO %d: %s", code, reason);
    print("\n\n       Press ENTER to restart. The system will restart in 2 seconds.\n\n");
    print ("       Developer/Technical Information:\n\n");
    printf("       EIP:%p, EFL:%p, USERESP:%p, ERRNO:%d\n", (void*)registers->eip, (void*)registers->efl, (void*)registers->useresp, registers->err_no);
    printf("       EAX:%p, EBX:%p, ECX:%p, EDX:%p\n", (void*)registers->eax, (void*)registers->ebx, (void*)registers->ecx, (void*)registers->edx);
    printf("       ESI:%p, EDI:%p, EBP:%p, ESP:%p\n\n", (void*)registers->esi, (void*)registers->edi, (void*)registers->ebp, (void*)registers->esp);
    printf("       Memory Dump around EIP:\n");
    printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));

    while(1);
}
