#include <stdio.h>

void main() {
    int x = 5;

    printf("Hello, World (from user mode ofc)!\n");
    printf("Please enter a number: ");
    scanf("%d", &x);
    fprintf(stderr, "This is an error message. x = %d.\n", x);

    while(1);
}