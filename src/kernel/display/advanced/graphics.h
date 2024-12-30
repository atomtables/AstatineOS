//
// Created by Adithiya Venkatakrishnan on 21/12/2024.
//

#ifndef GRAPHICS_H
#define GRAPHICS_H

#define VGA_TEXT_WIDTH  80
#define VGA_TEXT_HEIGHT 25
#define VGA_TEXT_SIZE   VGA_TEXT_WIDTH * VGA_TEXT_HEIGHT
#define VGA_TEXT_BSIZE  VGA_TEXT_SIZE * 2

#define COLOR_BLACK         0
#define COLOR_BLUE          1
#define COLOR_GREEN         2
#define COLOR_CYAN          3
#define COLOR_RED           4
#define COLOR_MAGENTA       5
#define COLOR_BROWN         6
#define COLOR_LIGHT_GREY    7
#define COLOR_DARK_GREY     8
#define COLOR_LIGHT_BLUE    9
#define COLOR_LIGHT_GREEN   10
#define COLOR_LIGHT_CYAN    11
#define COLOR_LIGHT_RED     12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_YELLOW        14
#define COLOR_WHITE         15

#define VGA_TEXT_COLOR(fg, bg) (fg | bg << 4)

#include <modules/modules.h>

typedef struct sprite {
    u8 width;
    u8 height;

    char** sprite; // data

    u32 colors;
    char** colormap;
} sprite;

void enable_double_buffering();
void disable_double_buffering();

void enable_vga_cursor(const u8 width, const u8 height);
void disable_vga_cursor();
u16 get_vga_cursor();
void set_vga_cursor(const int x, const int y);
void swap_graphics_buffer();
void clear_graphics_buffer();
void clear_and_set_screen_color(u8 color);
void draw_char(int x, int y, char character);
void draw_color(int x, int y, u8 color);
void draw_char_with_color(int x, int y, char character, u8 color);
void draw_string(int x, int y, char* str);
void draw_string_with_color(int x, int y, char* str, u8 color);
void draw_sprite(sprite s, int x, int y);
void draw_sprite_with_color(sprite s, int x, int y, ...);

#endif //GRAPHICS_H
