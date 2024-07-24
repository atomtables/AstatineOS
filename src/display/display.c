//
// VGA Text-mode driver (display.c, display.h)
// Created by Adithiya Venkatakrishnan on 17/07/2024.
//
// The current implementation goes back to y=0 after
// reaching the last line.

#include <stdarg.h>
#include "display.h"
#include "modules/modules.h"

/// VARIABLE: buf
/// Used as a temporary buffer for storing results
/// of functions that return a string (char*).
char buf[10] = {0};

/// STRUCTURE: display_data
/// Property: integer x
/// Property: integer y
/// ----------------------------------------
/// The variable `display_data` is responsible
/// for holding the current X and Y coordinates
/// of the cursor in memory. This struct is only
/// to be seen by internal functions.
struct display_data {
    int x;
    int y;
} display_data;

/// FUNCTION: __reset_displaydata__
/// INTERNAL ONLY
/// ----------------------------------------
/// Sets the x and y value of display_data to 0.
/// Only for internal use, as this does not also
/// configure the text mode VGA cursor.
void __reset_displaydata__() {
    display_data.x = 0;
    display_data.y = 0;
}

/// FUNCTION: __set_displaydata__
/// INTERNAL ONLY
/// Parameter: integer x
/// Parameter: integer y
/// ----------------------------------------
/// Sets the x and y value of display_data to
/// the specified values and validates them.
/// Only for internal use, as this does not also
/// configure the text mode VGA cursor.
void __set_displaydata__(const int x, const int y) {
    if (x >= VGA_TEXT_WIDTH) return;
    if (y >= VGA_TEXT_HEIGHT) return;
    display_data.x = x;
    display_data.y = y;
}

/// FUNCTION: __inc_displaydata__
/// INTERNAL ONLY
/// ----------------------------------------
/// Sets the x and y value of display_data to
/// the specified values and validates them.
/// Only for internal use, as this does not also
/// configure the text mode VGA cursor.
void __inc_displaydata__() {
    display_data.x++;
    if (display_data.x >= VGA_TEXT_WIDTH) {
        display_data.y++;
        display_data.x = 0;
    }
    if (display_data.y >= VGA_TEXT_HEIGHT)
        display_data.y = 0;
}


/// FUNCTION: __get_vga_cursor_pos__
/// INTERNAL ONLY
/// ----------------------------------------
/// Gets the current x and y value of the VGA
/// cursor position from the VGA registers.
/// This is not necessary, as display_data and
/// the VGA cursor should have position parity.
u16 __get_vga_cursor_pos__() {
    u16 pos = 0;

    outportb(0x3D4, 0x0F);
    pos |= inportb(0x3D5);

    return pos;
}

/// FUNCTION: __set_vga_cursor_pos__
/// INTERNAL ONLY
/// Parameter: integer x
/// Parameter: integer y
/// ----------------------------------------
/// Sets the current x and y value of the VGA
/// cursor position to the VGA registers.
/// Only for internal use, as this does not also
/// configure the display_data variables.
void __set_vga_cursor_pos__(const int x, const int y) {
    const u16 pos = y * VGA_TEXT_WIDTH + x;

    outportb(0x3D4, 0x0F);
    outportb(0x3D5, (u8)(pos & 0xFF));

    outportb(0x3D4, 0x0E);
    outportb(0x3D5, (u8)((pos >> 8) & 0xFF));
}

/// FUNCTION: __write_char__
/// INTERNAL ONLY
/// Parameter: integer x
/// Parameter: integer y
/// Parameter: 8-bit-char character
/// Parameter: 8-bit-integer color
/// ----------------------------------------
/// Writes a single ASCII character to the
/// designated x and y coordinates with color.
/// Only for internal use, since other functions
/// like print and println do a better job.
void __write_char__(const int x, const int y, const char character, const u8 color) {
    if (x >= VGA_TEXT_WIDTH) return;

    volatile char* vga = (char*)0xb8000;
    vga += ((y * VGA_TEXT_WIDTH * 2) + (x * 2));
    *vga = character;
    *(vga + 1) = color;

    __set_displaydata__(x, y);
    __set_vga_cursor_pos__(x, y);
}

/// FUNCTION: __append_char__
/// INTERNAL ONLY
/// Parameter: 8-bit-char character
/// Parameter: 8-bit-integer
/// ----------------------------------------
/// Writes a single ASCII character after the
/// next character, determined by the values
/// of display_data.
/// Only for internal use, since other functions
/// like print and println do a better job.
void __append_char__(const char character, const u8 color) {
    __write_char__(display_data.x, display_data.y, character, color);

    __inc_displaydata__();
    __set_vga_cursor_pos__(display_data.x, display_data.y);
}

/// FUNCTION: __append_string__
/// DEBUG ONLY
/// Parameter: string(char*) str
/// ----------------------------------------
/// Writes a string of ASCII characters after
/// the next character, determined by the values
/// of display_data. This will point to a char*,
/// which is looped over until the null byte is reached.
/// Only for debug use, since other functions
/// like print and println are just better.
void __append_string__(const string str) { for (int i = 0; str[i] != '\0'; i++) { __append_char__(str[i], 0x0f); } }

/// FUNCTION: __append_string__
/// INTERNAL ONLY
/// ----------------------------------------
/// Increments the y value of display_data and
/// sets the value of x to 0, allowing writes
/// to start from the next line of the previous
/// character.
/// Only for internal use, since other functions
/// like print and println should add this to be
/// built in.
void __append_newline__() {
    display_data.y++;
    if (display_data.y > VGA_TEXT_HEIGHT) { display_data.y = 0; }
    display_data.x = 0;

    __set_vga_cursor_pos__(display_data.x, display_data.y);
}

/// FUNCTION: clear_screen
/// ----------------------------------------
/// Resets the text mode screen by writing null
/// bytes to all cells and setting the color of
/// all cells to white-on-black or monochrome.
/// It also resets display_data and sets the VGA
/// cursor to 0, 0.
void clear_screen() {
    for (int i = 0; i <= VGA_TEXT_WIDTH; i++)
        for (int j = 0; j <= VGA_TEXT_HEIGHT; j++)
            __write_char__(i, j, '\0', 0x0f);
    __reset_displaydata__();
    __set_vga_cursor_pos__(0, 0);
}

/// FUNCTION: write_number_to_text_memory
/// DEBUG ONLY
/// Parameter: 32-bit-integer number
/// ----------------------------------------
/// Writes a number in decimal form to the screen.
/// Only for debug use, is integrated into print
/// and println.
void write_number_to_text_memory(u32 number) {
    u8 digits[10] = {0};
    int count = 9;

    while (number) {
        const int digit = number % 10;
        number = number / 10;
        digits[count] = digit;
        count--;
    }

    for (int i = 0; i < 10; i++) { __append_char__(find_char_for_int(digits[i]), 0x0f); }
}

/// FUNCTION: write_hex_to_text_memory
/// DEBUG ONLY
/// Parameter: 32-bit-integer number
/// ----------------------------------------
/// Writes a number in hexadecimal form to the
/// screen.
/// Only for debug use, is integrated into print
/// and println.
void write_hex_to_text_memory(u32 number) {
    u8 digits[8] = {0};
    int count = 9;

    while (number) {
        const int digit = number % 16;
        number = number / 16;
        digits[count] = digit;
        count--;
    }

    for (int i = 0; i < 8; i++) { __append_char__(find_char_for_hex(digits[i]), 0x0f); }
}

/// FUNCTION: print
/// Parameter:

/// FUNCTION: printf
/// Parameter: string(char*) fmt
/// Variadic Parameters: ...
/// ----------------------------------------
/// Prints a string to the screen, including fmt-str like
/// data like %d, %x, %s, etc.
void printf(const string fmt, ...) {
    va_list args;
    va_start(args, 0);

    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%') {
            i++;
            switch (fmt[i]) {
            case 'd': {
                const string digits = itoa(va_arg(args, u32), &buf[0]);
                __append_string__(digits);
                break;
            }
            case 'x': {
                const string digits = xtoa(va_arg(args, u32), &buf[0]);
                __append_string__(digits);
                break;
            }
            case 'p': {
                __append_string__("0x");
                const string digits = xtoa_padded(va_arg(args, u32), &buf[0]);
                __append_string__(digits);
                break;
            }
            case 's': {
                __append_string__(va_arg(args, string));
                break;
            }
            case 'c': {
                __append_char__(va_arg(args, i32), 0x0f);
                break;
            }
            default: {
                __append_char__('%', 0x0f);
                __append_char__(fmt[i], 0x0f);
                break;
            }
            }
        }
        else if (fmt[i] == '\n') {
            __append_newline__();
        }
        else { __append_char__(fmt[i], 0x0f); }
    }

    va_end(args);
}
