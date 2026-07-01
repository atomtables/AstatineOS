#include <stdio.h>
#include <astatine/terminal.h>
#include <unistd.h>
#include "elf.h"

void loadnew(char* path) {
    __asm__ volatile (
        "int $0x30\n"
        :
        : "a"(5), "b"(path)
    );
};

void freenew(u32* addrs, u32 count) {
    __asm__ volatile (
        "int $0x30\n"
        :
        : "a"(6), "b"(addrs), "c"(count)
    );
};

void set_terminal_mode(uint8_t mode) {
    __asm__ volatile (
        "int $0x30\n"
        : 
        : "a"(3), "b"(0), "c"(mode)
    );
};

void main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    int x = 5;

    printf("\033[31mHello, World (from user mode ofc)!\n\033[0m");
    printf("\033[15;15HPlease enter a number: ");
    scanf("%d", &x);
    // char buf[1];
    // read(0, buf, 1);
    // x = buf[0] - '0';
    fprintf(stderr, "This is an error message. x = %d.\n", x);
    // char str[10];
    printf("Please enter a string: ");
    // scanf("%10s", str);

    printf("Loading ELF...\n");
    loadnew("/primary/basicbasic.aex");
}