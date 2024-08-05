//
// VGA Text-mode driver (display.c, display.h)
// Created by Adithiya Venkatakrishnan on 17/07/2024.
//
// The current implementation goes back to y=0 after
// reaching the last line.
//
// The VGA driver requires the memory regions between 0xa0000 and 0xbffff
// to be freed, as this is reserved for video memory.

#ifndef DISPLAY_H
#define DISPLAY_H

#define VGA_TEXT_WIDTH  80
#define VGA_TEXT_HEIGHT 25
#define VGA_TEXT_SIZE   VGA_TEXT_WIDTH * VGA_TEXT_HEIGHT

#define VGA_TEXT_PAGING_WIDTH  80
#define VGA_TEXT_PAGING_HEIGHT 48
#define VGA_TEXT_PAGING_SIZE   VGA_TEXT_PAGING_WIDTH * VGA_TEXT_PAGING_HEIGHT * 2

#include "modules/modules.h"

void disable_vga_cursor();

void clear_screen();

void change_screen_color(u8 color);

void print(string str);

void print_color(string str, u8 color);

void println(string str);

void printf(string fmt, ...);

#endif //DISPLAY_H
