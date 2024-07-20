//
// Created by Adithiya Venkatakrishnan on 18/07/2024.
//

#ifndef DISPLAY_H
#define DISPLAY_H

#define VGA_TEXT_WIDTH 80
#define VGA_TEXT_HEIGHT 24

#include "modules.h"

extern void __append_string__(const string str);

extern void __append_newline__();

extern void clear_screen();

extern void write_number_to_text_memory(u32 number);

extern void write_hex_to_text_memory(u32 number);

#endif //DISPLAY_H
