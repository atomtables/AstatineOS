//
// Created by Adithiya Venkatakrishnan on 2/1/2025.
//

#include <display/simple/display.h>
#include <pcspeaker/pcspeaker.h>
#include <exception/exception.h>
#include <memory/memory.h>
#include <modules/strings.h>
#include <programs/fungame/fungame.h>
#include <programs/netnotes/netnotes.h>
#include <ps2/keyboard.h>
#include <timer/PIT.h>

typedef struct Command {
    char* name;
    void(* function)(int, char**);
} Command;

void echo(int argc, char** argv) {
    for (int i = 0; i < argc; i++) { display.printf("%s ", argv[i]); }
    display.printf("\n");
}

void onesecond() {
    sleep(1000);
    display.printf("1 second has passed\n");
}

void clear(int argc, char** argv) {
    display.clear_screen();
}

Command commands[] = {
    {"echo", echo},
    {"beep", beep},
    {"sleep", onesecond},
    {"fungame", fungame},
    {"netnotes", netnotes},
    {"clear", clear},
    {"reboot", reboot},
};

void ahsh() {
    display.printf("creating a simple prompt:\n");

    // code doesn't have to be good, just functional

    while (1) {
        char* prompt = malloc(64);
        display.printf("NetworkOS %p> ", &prompt);
        prompt = input(prompt, 64);
        StrtokA prompt_s = strtok_a(prompt, " ");
        for (u32 i = 0; i < sizeof(commands) / sizeof(Command); i++) {
            if (strcmp(commands[i].name, prompt_s.ret[0]) == 0) {
                if (commands[i].function) {
                    commands[i].function(prompt_s.count - 1, &prompt_s.ret[1]);
                }
                goto complete;
            }
        }
        display.printf("%s\n", "Command not found...");

        complete:
            free(prompt_s.ret, prompt_s.size);
        free(prompt, 64);
    }
}
