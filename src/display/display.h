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
#define VGA_TEXT_HEIGHT 26

#include "modules/modules.h"

extern void disable_vga_cursor();

extern void __append_newline__();

extern void clear_screen();

extern void change_screen_color(u8 color);

extern void print(string str);

extern void print_color(string str, u8 color);

extern void println(string str);

extern void printf(string fmt, ...);

#endif //DISPLAY_H
