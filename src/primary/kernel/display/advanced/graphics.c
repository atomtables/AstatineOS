// Graphics Driver for Video Games and Custom Programs
// as the program is meant to manage the intricacies of the
// graphics, this file only provides simplified subroutines
// for modifying graphics on screen.
//
// Created by Adithiya Venkatakrishnan on 21/12/2024.
//

#include "graphics.h"

#include <stdarg.h>
#include <memory/memory.h>
#include <modules/modules.h>

// implementing double-buffering because flickering is a huge problem
static u8 _sbuffers[VGA_TEXT_BSIZE][2];
static u8 _sback = 0;

static bool double_buffering = false;

// i don't get jdh's implementation but hes smart soooo
#define CURRENT (double_buffering ? _sbuffers[_sback] : (u8*)0xb8000)
#define SWAP() (_sback = 1 - _sback)

void enable_double_buffering() {
    double_buffering = true;
}

void disable_double_buffering() {
    double_buffering = false;
}

/**
 * @brief Enables the VGA cursor.
 *
 * Writes to the VGA registor ports to stop
 * the cursor from blinking. Only relevant in
 * the case of a recovery from a blue-screen,
 * interactive display, or a jump into a text mode.
 */
void enable_vga_cursor() {
    outportb(0x3D4, 0x0A);
    outportb(0x3D5, (inportb(0x3D5) & 0xC0) | 79);

    outportb(0x3D4, 0x0B);
    outportb(0x3D5, (inportb(0x3D5) & 0xE0) | 24);
}

/**
 * @brief Disables the VGA cursor.
 *
 * Writes to the VGA register ports to stop
 * the cursor from blinking. Only relevant in
 * the case of a blue-screen, non-interactive
 * display, or a jump into a graphical mode.
 */
void disable_vga_cursor() {
    outportb(0x3D4, 0x0A);
    outportb(0x3D5, 0x20);
}

/**
 * @brief Gets the VGA cursor position. Internal only.
 * @deprecated as display_data has position parity anyway.
 *
 * Gets the current x and y value of the VGA
 * cursor position from the VGA registers. The individual
 * coordinates can be found by dividing the result by 80, or
 * modulating the result by 80.
 * @return Coordinates of the VGA cursor, in pixel-count format.
 */
u16 get_vga_cursor() {
    u16 pos = 0;

    outportb(0x3D4, 0x0F);
    pos |= inportb(0x3D5);

    return pos;
}

/**
 * @brief Sets the VGA cursor position. Internal only.
 *
 * Sets the current x and y value of the VGA
 * cursor position to the VGA registers.
 * Only for internal use, as this does not also
 * configure the display_data variables.
 * @param x The X coordinate to set the cursor to
 * @param y The Y coordinate to set the cursor to
 */
void set_vga_cursor(const int x, const int y) {
    const u16 pos = y * VGA_TEXT_WIDTH + x;

    outportb(0x3D4, 0x0F);
    outportb(0x3D5, (u8)(pos & 0xFF));

    outportb(0x3D4, 0x0E);
    outportb(0x3D5, (u8)((pos >> 8) & 0xFF));
}

void enable_vga_blink() {
    inportb(0x3DA);
    outportb(0x3C0, 0x30);

    u8 x = inportb(0x3C1);
    outportb(0x3C0, x | 0x08);
}

void disable_vga_blink() {
    inportb(0x3DA);
    outportb(0x3C0, 0x30);

    u8 x = inportb(0x3C1);
    outportb(0x3C0, x & 0xF7);
}

void swap_graphics_buffer() {
    ret_if(!double_buffering);
    memcpy((void*)0xb8000, CURRENT, VGA_TEXT_BSIZE);
    SWAP();
}

void clear_graphics_buffer() {
    set_vga_cursor(0, 0);
    memset_step(CURRENT, 0, VGA_TEXT_SIZE, 2);
    memset_step(CURRENT + 1, 0x0f, VGA_TEXT_SIZE, 2);
}

void set_screen_color(u8 color) {
    memset_step(CURRENT, color, VGA_TEXT_SIZE, 2);
}

void clear_and_set_screen_color(u8 color) {
    memset_step(CURRENT, 0, VGA_TEXT_SIZE, 2);
    memset_step(CURRENT + 1, color, VGA_TEXT_SIZE, 2);
}

void draw_char(int x, int y, char character) {
    if (x >= VGA_TEXT_WIDTH) return;
    if (y >= VGA_TEXT_HEIGHT) return;

    volatile char* vga = (char*)CURRENT;
    vga += y * VGA_TEXT_WIDTH * 2 + x * 2;
    *vga = character;
}

void draw_color(int x, int y, u8 color) {
    if (x >= VGA_TEXT_WIDTH) return;
    if (y >= VGA_TEXT_HEIGHT) return;

    volatile char* vga = (char*)CURRENT;
    vga += y * VGA_TEXT_WIDTH * 2 + x * 2;
    *(vga + 1) = color;
}

void draw_char_with_color(int x, int y, char character, u8 color) {
    if (x >= VGA_TEXT_WIDTH) return;
    if (y >= VGA_TEXT_HEIGHT) return;

    volatile char* vga = (char*)CURRENT;
    vga += y * VGA_TEXT_WIDTH * 2 + x * 2;
    *vga = character;
    *(vga+1) = color;
}

void draw_string(int x, int y, char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        draw_char(x, y, str[i]);
        x++;
    }
}

void draw_string_with_color(int x, int y, char* str, u8 color) {
    for (int i = 0; str[i] != '\0'; i++) {
        draw_char_with_color(x, y, str[i], color);
        x++;
    }
}

void draw_sprite(sprite s, int x, int y) {
    for (int i = 0; i < s.height; i++) {
        for (int j = 0; j < s.width; j++) {
            if (s.sprite[i][j] != null) {
                draw_char(x + j, y + i, s.sprite[i][j]);
            }
        }
    }
}

void draw_sprite_with_color(sprite s, int x, int y, ...) {
    u8* colormap = malloc(s.colors + 1);

    va_list args;
    va_start(args, y);
    for (int i = 0; i < s.colors; i++) {
        colormap[i] = (u8)va_arg(args, int);
    }

    for (int i = 0; i < s.height; i++) {
        for (int j = 0; j < s.width; j++) {
            if (s.sprite[i][j] != null) {
                draw_char_with_color(x + j, y + i, s.sprite[i][j], colormap[s.colormap[i][j] - 1]);
            }
        }
    }

    free(colormap, s.colors);

    va_end(args);
}