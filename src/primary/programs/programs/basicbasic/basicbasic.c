//
// Created by Adithiya Venkatakrishnan on 14/1/2025.
//
// rewriting this so it actually works better

#include <stdint.h>
#include <stdbool.h>
#include <astatine/terminal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define VGA_TEXT_COLOR(fg, bg) (((bg) << 4) | (fg))
#define COLOR_BLACK         0x0
#define COLOR_BLUE          0x1
#define COLOR_GREEN         0x2
#define COLOR_CYAN          0x3
#define COLOR_RED           0x4
#define COLOR_MAGENTA       0x5
#define COLOR_BROWN         0x6
#define COLOR_LIGHT_GREY    0x7
#define COLOR_DARK_GREY     0x8
#define COLOR_LIGHT_BLUE    0x9
#define COLOR_LIGHT_GREEN   0xA
#define COLOR_LIGHT_CYAN    0xB
#define COLOR_LIGHT_RED     0xC
#define COLOR_LIGHT_MAGENTA 0xD
#define COLOR_YELLOW        0xE
#define COLOR_WHITE         0xF

void set_terminal_mode(uint8_t mode) {
    asm volatile (
        "int $0x30\n"
        :
        : "a"(3), "b"(0), "c"(mode)
    );
};

int xopen(char* identifier) {
    int fd;
    asm volatile (
        "int $0x30\n"
        : "=a"(fd)
        : "a"(4), "b"(identifier), "d"(0)
    );
    return fd;
}

void output(void) {
    // draw a white frame
    for (int x = 0; x < 80; x++) {
        printf("\033[0;%dH\033[47m \033[0m", x+1);
        printf("\033[24;%dH\033[47m \033[0m", x+1);
    }
}

struct teletype_packet {
    uint8_t yes;
    uint32_t x;
    uint32_t y;
    char* buffer;
    uint32_t size;
    uint8_t with_color;
    uint8_t color;
    uint8_t move_cursor;
} packet;

void main(void) {
    // printf("\033[H\033[2J");
    setvbuf(stdout, NULL, _IOFBF, 0);
    setvbuf(stderr, NULL, _IOFBF, 0);
    int fd = xopen("teletype");
    packet.yes = true;
    packet.x = 10;
    packet.y = 5;
    char* msg = "Hello from BasicBasic!";
    packet.buffer = msg;
    packet.size = strlen(msg);
    packet.with_color = true;
    // VGA color btw
    packet.color = VGA_TEXT_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK);
    packet.move_cursor = true;
    write(fd, &packet, sizeof(packet));
    // Clear the screen
    while(1) {
        // output();
        fflush(stdout);
        while(1);
    };
}