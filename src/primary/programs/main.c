#include <stdio.h>
#include <astatine/terminal.h>
#include <unistd.h>

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
    printf("Please enter a number: ");
    scanf("%d", &x);
    // char buf[1];
    // read(0, buf, 1);
    // x = buf[0] - '0';
    fprintf(stderr, "This is an error message. x = %d.\n", x);

    while(1);
}