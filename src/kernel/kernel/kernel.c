#include <exception/exception.h>
#include <idt/interrupt.h>
#include <ps2/keyboard.h>
#include <memory/memory.h>
#include <modules/strings.h>
#include <pcspeaker/pcspeaker.h>
#include <ps2/controller.h>
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
    u32 mem = *(u32*)0x15;
    return mem * 64;
}

typedef struct Command {
    char* name;
    void (*function)(int, char**);
} Command;

void echo(int argc, char** argv) {
    for (int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
}

static Command commands[] = {
    {"echo", echo},
    {"beep", beep},
    {"clear", clear_screen},
    {"reboot", reboot},
};

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

    ps2_controller_init();
    keyboard_init();

    beep();
    // sleep(500);

    printf("creating a simple prompt:\n");

    // code doesn't have to be good, just functional

    while (1) {
        char* prompt = malloc(64);
        printf("NetworkOS> ");
        prompt = input(prompt, 64);
        StrtokA prompt_s = strtok_a(prompt, " ");
        for (u32 i = 0; i < sizeof(commands) / sizeof(Command); i++) {
            if (strcmp(commands[i].name, prompt_s.ret[0]) == 0) {
                commands[i].function(prompt_s.count - 1, &prompt_s.ret[1]);
                goto complete;
            }
        }
        printf("%s\n", "Command not found...");
    complete:
        free(prompt, 64);
    }
}
