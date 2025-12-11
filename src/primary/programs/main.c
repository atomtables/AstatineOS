#include <stdio.h>
#include <astatine/terminal.h>
#include <unistd.h>
#include "elf.h"

u32* loadnew(char* path, u32* out_count, u32* entrypoint, u32* stack_top) {
    u32* addrs;
    asm volatile (
        "int $0x30\n"
        : "=a"(addrs), "=c"(*out_count), "=d"(*entrypoint), "=S"(*stack_top)
        : "a"(5), "b"(path)
    );
    return addrs;
};

void freenew(u32* addrs, u32 count) {
    asm volatile (
        "int $0x30\n"
        :
        : "a"(6), "b"(addrs), "c"(count)
    );
};

void set_terminal_mode(uint8_t mode) {
    asm volatile (
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

    u32 count = 0;
    u32 entrypoint = 0;
    u32 stack_top = 0;
    printf("Loading ELF...\n");
    u32* addrs = loadnew("/primary/basicbasic.aex", &count, &entrypoint, &stack_top);
    if (!addrs || addrs == (u32*)-1 || entrypoint == 0 || stack_top == 0) {
        printf("Failed to load ELF file (addr=%p entry=%x stack=%x).\n", addrs, entrypoint, stack_top);
        return;
    }
    printf("Loaded ELF with %d pages, entrypoint at %x.\n", count, entrypoint);
    printf("Jumping to entrypoint...\n");
    // switch to freshly allocated user stack before jumping
    asm volatile ("mov %0, %%esp; mov %0, %%ebp" :: "r"(stack_top));
    void (*entry_func)() = (void(*)())entrypoint;
    entry_func();
    printf("Returned from ELF program.\n");
    freenew(addrs, count);


    while(1);
}