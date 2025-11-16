#include <stdio.h>

void main() {
    int x = 5;

    printf("Hello, World (from user mode ofc)!\n");

    fprintf(stderr, "This is an error message. x = %d.\n", x);

    while(1);
}