#include <exception/exception.h>
#include <idt/interrupt.h>
#include <keyboard/keyboard.h>
#include <memory/memory.h>
#include <modules/strings.h>
#include <pcspeaker/pcspeaker.h>
#include <timer/PIT.h>

#include "display/display.h"

/* In our kernel, we can reserve memory
 * 0x100000-0x1FFFFF for the storage of heap data (like variables)
 * Store an array for 32-byte increments
*/

// YO THIS GUY ONLINE WAS ACT LEGIT :skull:

u32 get_conventional_memory_kb() {
    u32* mem = (u32*)0x413;
    return *mem;
}

u64 get_extended_memory_kb() {
    u32* mem = (u32*)0x15;
    return *mem * 64;
}

int main() {
    clear_screen();

    printf("NetworkOS Kernel v0.1\n");
    printf("booting with %u bytes of memory\n", get_extended_memory_kb());

    idt_init();
    isr_init();
    PIC_init();

    init_mem();

    timer_init();
    pcs_init();

    keyboard_init();

    beep();
    sleep(500);

    printf("creating a simple prompt:\n");



    while (1) {
        char* prompt = malloc(64);
        printf("NetworkOS> ");
        prompt = input(prompt, 64);
        char** prompt_s = strtok_a(prompt, " ");
        for (int i = 0; prompt_s[i]; i++) {
            printf("%s\n", prompt_s[i]);
        }
        free(prompt, 64);
    }
}
