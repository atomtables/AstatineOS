//
// Created by Adithiya Venkatakrishnan on 24/07/2024.
//

#include "exception.h"

#include <display/display.h>

// works with 32-bit
void fatal_error(const int code, char* reason, void* function, u32** registers) {
    int* eip = __builtin_return_address(0) - 5;
    int* early_call = __builtin_return_address(1) - 5;

    clear_screen();

    disable_vga_cursor();

    u32** eipS = (u32**)((u32)eip / 16 * 16 - 0x20);

    change_screen_color(0x1f);
    print("\n\n\n");
    print("                            ");
    print_color(" NetworkOS Fatal Error ", 0x71);
    print("\n\n");
    // print ("                                                                                ");
    printf("       An exception at %p (caller %p) has resulted in\n", eip, function);
    print("       a fatal error. This error was caused by:\n\n");
    printf("       ERRNO %d: %s", code, reason);
    print("\n\n       Press ENTER to restart. The system will restart in 10 seconds.\n\n");
    print("       Developer/Technical Information:\n\n");
    printf("       EIP: %p -> Caller: %p -> Parent: %p\n", eip, function, early_call);
    printf("       EAX:%p, EBX:%p, ECX:%p, EDX:%p\n", registers[0], registers[1], registers[2], registers[3]);
    printf("       ESI:%p, EDI:%p, EBP:%p, ESP:%p\n\n", registers[4], registers[5], registers[6], registers[7]);
    printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    printf("       %p:   %p   %p   %p   %p\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));

    while (1) {
        // for (int i = 0; i < 1000000000; i++) {
        //     __asm__ ("nop");
        // }
        // print("hi");
        // // Disable interrupts
        // __asm__ ("cli");
        //
        // // Load an invalid IDT (Interrupt Descriptor Table)
        // unsigned char invalid_idt[6] = {0};
        // __asm__ ("lidt %0" : : "m"(invalid_idt));
        //
        // // Trigger an interrupt (e.g., divide by zero)
        // __asm__ ("int3");
    };
}
