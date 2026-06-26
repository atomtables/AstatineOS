//
// Created by Adithiya Venkatakrishnan on 24/07/2024.
//

#include "exception.h"

#include <display/advanced/graphics.h>
#include <display/simple/display.h>
#include <interrupt/isr.h>
#include <timer/PIT.h>

#include "modules/strings.h"

void reboot() {
    u8 good = 0x02; 
    while (good & 0x02)
        good = inportb(0x64);
    outportb(0x64, 0xFE);
    asm ("hlt");
}

void panicking(char* reason) {
    // just in case the screen driver hasn't loaded yet
    char* screen_text = (char*)0xb8000;
    for (u32 i = 0; reason[i] != 0; i++) {
        screen_text[i * 2] = reason[i];
        screen_text[i * 2 + 1] = 0x4f; // white on red
    }

    SERIAL_PRINT(reason);

    clear_screen();

    disable_vga_cursor();

    // change_screen_color(0x1f);
    // print("\n\n\n\n\n\n\n\n");
    // print("                            ");
    print_color(" NetworkOS Fatal Error ", 0x71);
    // print("\n\n");
    // print ("                                                                                ");
    printf("       An exception has resulted in a KERNEL PANIC.\n");
    printf("       This error was caused by:\n\n");
    printf("%s", reason);
    printf("\n\n       Press ENTER to restart. The system will restart in 5 seconds.\n\n");

    // sleep(5000);
    reboot();
}



void interrupt_panic(const int code, char* reason, const struct registers* registers) {
    int* eip = (int*)registers->eip;

    char* screen_text = (char*)0xb8000;
    for (u32 i = 0; reason[i] != 0; i++) {
        screen_text[i * 2] = reason[i];
        screen_text[i * 2 + 1] = 0x4f; // white on red
    }

    clear_screen();

    disable_vga_cursor();

    u32* eipS = (u32*)((u32)eip / 16 * 16 - 0x20);

    change_screen_color(0x1f);
    // print("\n\n");
    // print("                            ");
    print_color(" AstatineOS Fatal Error ", 0x71);
    // print("\n\n");
    char buf[127] = "INT";
    itoa(code, &buf[strlen(buf)]);
    strcat(buf, ": ");
    strcat(buf, reason);
    strcat(buf, "(errno ");
    itoa(registers->err_no, &buf[strlen(buf)]);
    strcat(buf, ")");
    printf(" a fault occurred in the current task: ");
    print_color(buf, COLOR_GREEN);
    printf("\n\nDeveloper/Technical Information: \n");
    printf("EIP:%p, EFL:%p, USERESP:%p, EAX:%p, EBX:%p, ECX:%p, EDX:%p, ESI:%p, EDI:%p, EBP:%p, ESP:%p\n\n", (void*)registers->eip, (void*)registers->efl, (void*)registers->useresp, (void*)registers->eax, (void*)registers->ebx, (void*)registers->ecx, (void*)registers->edx, (void*)registers->esi, (void*)registers->edi, (void*)registers->ebp, (void*)registers->esp);
    printf("Memory Dump around EIP:\n");
    printf("%p: %y %y %y %y\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    printf("%p: %y %y %y %y\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    printf("%p: %y %y %y %y\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    printf("%p: %y %y %y %y\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));
    eipS += 4;
    printf("%p: %y %y %y %y\n", eipS, *eipS, *(eipS + 1), *(eipS + 2), *(eipS + 3));

    STI();
    sleep(5000);
    reboot();
}
