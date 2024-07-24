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

#define VGA_TEXT_WIDTH 80
#define VGA_TEXT_HEIGHT 24

#include "modules/modules.h"

extern void __append_string__(const string str);

extern void __append_newline__();

extern void clear_screen();

extern void write_number_to_text_memory(u32 number);

extern void write_hex_to_text_memory(u32 number);

extern void printf(string fmt, ...);

#endif //DISPLAY_H
