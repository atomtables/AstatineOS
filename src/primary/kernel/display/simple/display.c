///
/// VGA Text-mode driver (display.c, display.h)
/// Created by Adithiya Venkatakrishnan on 17/07/2024.
///
/// The current implementation stays at the last line
/// during overflow. This may cause a tiny bit of performance
/// loss, but should be negligent (seeing a transition to VGA
/// color-pixel mode)
///

#include <stdarg.h>

#include "display.h"
#include <modules/modules.h>
#include <driver_base/teletype/teletype.h>

#define ATD active_teletype_driver

static struct TeletypeMode mode;

/**
 * Used as a temporary buffer for storing results
 * of functions that return a string(char*).
 */
static char buf[11] = {0};

/**
 * @brief Structure for holding the current X and Y coordinates of the cursor.
 *
 * The variable `display_data` is responsible
 * for holding the current X and Y coordinates
 * of the cursor in memory. This struct is only
 * to be seen by internal functions.
 *
 * @param x The X coordinate of the cursor
 * @param y The Y coordinate of the cursor
 * @param top The top line of the currently displayed text
 * @param bottom The bottom line of the currently displayed text
 */
static struct display_data {
    i32 x;
    i32 y;
} display_data;

/**
 * @brief Resets the display_data cursor position.
 *
 * Sets the x and y value of display_data to 0.
 * Only for internal use, as this does not also
 * configure the text mode VGA cursor.
 */
static void __reset_displaydata__() {
    display_data.x = 0;
    display_data.y = 0;
}

/**
 * @brief Sets the display_data cursor position.
 *
 * Sets the x and y value of display_data to
 * the specified values and validates them.
 * Only for internal use, as this does not also
 * configure the text mode VGA cursor.
 * @param x The X coordinate to set display_data to
 * @param y The Y coordinate to set display_data
 */
static void __set_displaydata__(const int x, const int y) {
    if (x >= VGA_TEXT_WIDTH) return;
    if (y >= VGA_TEXT_HEIGHT) return;
    display_data.x = x;
    display_data.y = y;
}

/**
 * @brief Increments display_data by a character.
 *
 * Sets the x and y value of display_data to
 * the specified values and validates them.
 * Only for internal use, as this does not also
 * configure the text mode VGA cursor.
 */
static void __inc_displaydata__() {
    display_data.x++;
    if (display_data.x >= VGA_TEXT_WIDTH) {
        display_data.y++;
        display_data.x = 0;
    }
    if (display_data.y >= VGA_TEXT_HEIGHT)
        display_data.y = VGA_TEXT_HEIGHT - 1;
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
static u16 __get_vga_cursor_pos__() {
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
static void __set_vga_cursor_pos__(const int x, const int y) {
    if (active_teletype_driver == null) return;
    ATD->functions.set_cursor_position(ATD, x, y);
}

/**
 * @brief Enables the VGA cursor.
 *
 * Writes to the VGA registor ports to stop
 * the cursor from blinking. Only relevant in
 * the case of a recovery from a blue-screen,
 * interactive display, or a jump into a text mode.
 * @param width The width of where the cursor can go.
 * @param height The height of where the cursor can go.
 */
// static void enable_cursor(const u8 width, const u8 height) {
//     outportb(0x3D4, 0x0A);
//     outportb(0x3D5, (inportb(0x3D5) & 0xC0) | width);
//
//     outportb(0x3D4, 0x0B);
//     outportb(0x3D5, (inportb(0x3D5) & 0xE0) | height);
// }

/**
 * @brief Disables the VGA cursor.
 *
 * Writes to the VGA register ports to stop
 * the cursor from blinking. Only relevant in
 * the case of a blue-screen, non-interactive
 * display, or a jump into a graphical mode.
 */
// static void disable_vga_cursor() {
//     outportb(0x3D4, 0x0A);
//     outportb(0x3D5, 0x20);
// }

/**
 * @brief Writes a character to a position on the screen. Internal only.
 *
 * Writes a single ASCII character to the
 * designated x and y coordinates, ignoring color.
 * Only for internal use, since other functions
 * like print/printf do a better job.
 * @param x The X coordinate to write to
 * @param y The Y coordinate to write to
 * @param character The character to write.
 */
static void __write_char__(const int x, int y, const char character) {
    if (active_teletype_driver == null) return;
    if (mode.width == 0) ATD->functions.get_mode(ATD, &mode);

    if (mode.cells_valid) {
        if (character == 0) return;
        if (x >= mode.width) return;
        // if y is greater than height, set y to height-1 and move lines 1-23 to 0-22 using memcpy
        if (y >= mode.height) {
            y = mode.height - 1;
            for (int i = 0; i < mode.height - 1; i++) {
                memcpy((void*)mode.cells + i * mode.width * 2, (void*)mode.cells + (i + 1) * mode.width * 2, mode.width * 2);
            }
            memset_step((void*)mode.cells + (mode.height - 1) * mode.width * 2, 0, mode.width, 2);
        }

        u8* cell = (u8*)(mode.cells + y * mode.width * 2 + x * 2);
        *cell = character;

        __set_displaydata__(x, y);
        __set_vga_cursor_pos__(x, y);
    } else {
        if (character == 0) return;
        if (x >= mode.width) return;
        // if y is greater than height, set y to height-1 and move lines 1-23 to 0-22 using memcpy
        if (y >= mode.height) {
            y = mode.height - 1;
            for (int i = 0; i < mode.height - 1; i++) {
                for (int j = 0; j < mode.width; j++) {
                    ATD->functions.set_char(ATD, j, i, ATD->functions.get_char(ATD, j, i + 1) >> 8, ATD->functions.get_char(ATD, j, i + 1) & 0xff);
                }
            }
            for (int j = 0; j < mode.width; j++) {
                ATD->functions.set_char(ATD, j, mode.height - 1, 0, 0x0f);
            }
        }
        ATD->functions.set_char(ATD, x, y, character, 0x0f);
        __set_displaydata__(x, y);
        __set_vga_cursor_pos__(x, y);
    }
}

/**
 * @brief Writes a character with color to a position on the screen. Internal only.
 *
 * Writes a single ASCII character to the
 * designated x and y coordinates, overwriting color.
 * Only for internal use, since other functions
 * like print/printf do a better job.
 * @param x The X coordinate to write to
 * @param y The Y coordinate to write to
 * @param character The character to write
 * @param color The color to write
 */
static void __write_char_color__(const int x, int y, const char character, const u8 color) {
    if (active_teletype_driver == null) return;
    if (mode.width == 0) ATD->functions.get_mode(ATD, &mode);

    if (mode.cells_valid) {
        if (character == 0) return;
        if (x >= mode.width) return;
        // if y is greater than height, set y to height-1 and move lines 1-23 to 0-22 using memcpy
        if (y >= mode.height) {
            y = mode.height - 1;
            for (int i = 0; i < mode.height - 1; i++) {
                memcpy((void*)mode.cells + i * mode.width * 2, (void*)mode.cells + (i + 1) * mode.width * 2, mode.width * 2);
            }
            memset_step((void*)mode.cells + (mode.height - 1) * mode.width * 2, 0, mode.width, 2);
        }

        u8* cell = (u8*)(mode.cells + y * mode.width * 2 + x * 2);
        *cell = character;
        *(cell + 1) = color;

        __set_displaydata__(x, y);
        __set_vga_cursor_pos__(x, y);
    } else {
        if (character == 0) return;
        if (x >= mode.width) return;
        // if y is greater than height, set y to height-1 and move lines 1-23 to 0-22 using memcpy
        if (y >= mode.height) {
            y = mode.height - 1;
            for (int i = 0; i < mode.height - 1; i++) {
                for (int j = 0; j < mode.width; j++) {
                    ATD->functions.set_char(ATD, j, i, ATD->functions.get_char(ATD, j, i + 1) >> 8, ATD->functions.get_char(ATD, j, i + 1) & 0xff);
                }
            }
            for (int j = 0; j < mode.width; j++) {
                ATD->functions.set_char(ATD, j, mode.height - 1, 0, 0x0f);
            }
        }
        ATD->functions.set_char(ATD, x, y, character, color);
        __set_displaydata__(x, y);
        __set_vga_cursor_pos__(x, y);
    }
}

/**
 * @brief Writes a color to a position on the screen. Internal only.
 *
 * As color should not be overwritten when simply writing
 * text to the screen, this function is used to write
 * just a color to the screen at a specific position.
 * This function is internal only, since other functions
 * like print/printf do a better job.
 * @param x The X coordinate to write to.
 * @param y The Y coordinate to write to.
 * @param color The color to write.
 */
static void __write_color__(const int x, int y, const u8 color) {
    if (active_teletype_driver == null) return;
    if (mode.width == 0) ATD->functions.get_mode(ATD, &mode);

    if (mode.cells_valid) {
        if (x >= mode.width) return;
        // if y is greater than height, set y to height-1 and move lines 1-23 to 0-22 using memcpy
        if (y >= mode.height) {
            y = mode.height - 1;
            for (int i = 0; i < mode.height - 1; i++) {
                memcpy((void*)mode.cells + i * mode.width * 2, (void*)mode.cells + (i + 1) * mode.width * 2, mode.width * 2);
            }
            memset_step((void*)mode.cells + (mode.height - 1) * mode.width * 2, 0, mode.width, 2);
        }

        u8* cell = (u8*)(mode.cells + y * mode.width * 2 + x * 2 + 1);
        *cell = color;

        __set_displaydata__(x, y);
        __set_vga_cursor_pos__(x, y);
    } else {
        if (x >= mode.width) return;
        // if y is greater than height, set y to height-1 and move lines 1-23 to 0-22 using memcpy
        if (y >= mode.height) {
            y = mode.height - 1;
            for (int i = 0; i < mode.height - 1; i++) {
                for (int j = 0; j < mode.width; j++) {
                    ATD->functions.set_char(ATD, j, i, ATD->functions.get_char(ATD, j, i + 1) >> 8, ATD->functions.get_char(ATD, j, i + 1) & 0xff);
                }
            }
            for (int j = 0; j < mode.width; j++) {
                ATD->functions.set_char(ATD, j, mode.height - 1, 0, 0x0f);
            }
        }
        u8 current_char = ATD->functions.get_char(ATD, x, y) >> 8;
        ATD->functions.set_char(ATD, x, y, current_char, color);
        __set_displaydata__(x, y);
        __set_vga_cursor_pos__(x, y);
    }
}

/**
 * @brief Appends a character to the screen. Internal only.
 *
 * Writes a single ASCII character after the
 * next character, determined by the values
 * of display_data. It does not overwrite color.
 * Only for internal use, since other functions
 * like print/printf do a better job.
 * @param character The character to append.
 */
static void __append_char__(const char character) {
    __write_char__(display_data.x, display_data.y, character);

    __inc_displaydata__();
    __set_vga_cursor_pos__(display_data.x, display_data.y);
}

/**
 * @brief Appends a character to the screen with a specified color. Internal only.
 *
 * Writes a single ASCII character after the
 * next character, determined by the values
 * of display_data. It also overwrites the color
 * with the one specified.
 * Only for internal use, since other functions
 * like print/printf do a better job.
 * @param character The character to append.
 * @param color The color to append it in.
 */
static void __append_char_color__(const char character, const u8 color) {
    __write_char_color__(display_data.x, display_data.y, character, color);

    __inc_displaydata__();
    __set_vga_cursor_pos__(display_data.x, display_data.y);
}

static void __append_newline__();
static void __append_backspace__();

/**
 * @brief Appends a char* to the screen. Internal only.
 *
 * Writes a char* of ASCII characters after
 * the next character, determined by the values
 * of display_data. This will point to a char*,
 * which is looped over until the null byte is reached.
 * Color is not specified or written over.
 * Only for debug use, since other functions
 * like print/printf are just better.
 * @param str The char* to append.
 */
static void __append_string__(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            __append_newline__();
            continue;
        }
        if (str[i] == 0x08) {
            __append_backspace__();
            continue;
        }
        __append_char__(str[i]);
    }
}

/**
 * @brief Appends a char* to the screen with a specified color. Internal only.
 *
 * Writes a char* of ASCII characters after
 * the next character, determined by the values
 * of display_data. This will point to a char*,
 * which is looped over until the null byte is reached.
 * It also prints with a specified color.
 * Only for debug use, since other functions
 * like print/printf are just better.
 * @param str The char* to append.
 * @param color The color to print it in.
 */
static void __append_string_color__(char* str, const u8 color) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            __append_newline__();
            continue;
        }
        __append_char_color__(str[i], color);
    }
}

/**
 * @brief Moves the cursor to the next line. Internal only.
 *
 * Increments the y value of display_data and
 * sets the value of x to 0, allowing writes
 * to start from the next line of the previous
 * character.
 * Only for internal use, since other functions
 * like print and println should add this to be
 * built in.
 */
static void __append_newline__() {
    if (active_teletype_driver == null) return;
    if (mode.width == 0) ATD->functions.get_mode(ATD, &mode);
    display_data.x = 0;
    display_data.y++;
    if (display_data.y >= mode.height) {
        display_data.y = mode.height - 1;
        if (mode.cells_valid) {
            for (int i = 0; i < mode.height - 1; i++) {
                memcpy((void*)mode.cells + i * mode.width * 2, (void*)mode.cells + (i + 1) * mode.width * 2, mode.width * 2);
            }
            memset_step((void*)mode.cells + (mode.height - 1) * mode.width * 2, 0, mode.width, 2);
        } else {
            for (int i = 0; i < mode.height - 1; i++) {
                for (int j = 0; j < mode.width; j++) {
                    ATD->functions.set_char(ATD, j, i, ATD->functions.get_char(ATD, j, i + 1) >> 8, ATD->functions.get_char(ATD, j, i + 1) & 0xff);
                }
            }
            for (int j = 0; j < mode.width; j++) {
                ATD->functions.set_char(ATD, j, mode.height - 1, 0, 0x0f);
            }
        }
    }
}

static void __append_backspace__() {
    display_data.x--;
    if (display_data.x < 0) {
        display_data.y--;
        display_data.x = VGA_TEXT_WIDTH - 1;
    }
    if (display_data.y < 0) {
        display_data.y = 0;
        display_data.x = 0;
    }

    __write_char__(display_data.x, display_data.y, ' ');
}

/**
 * @brief Clears all text and color off the screen.
 *
 * Resets the text mode screen by writing null
 * bytes to all cells and setting the color of
 * all cells to white-on-black or monochrome.
 * It also resets display_data and sets the VGA
 * cursor to 0, 0.
 */
void clear_screen() {
    if (active_teletype_driver == null) return;
    ATD->functions.clear_screen(ATD, 0x0f);
    __reset_displaydata__();
    __set_vga_cursor_pos__(0, 0);
}


/**
 * Changes all color cells on the screen to
 * the specified color.
 * @param color The color to change to.
 */
void change_screen_color(const u8 color) {
    if (active_teletype_driver == null) return;
    if (mode.width == 0) ATD->functions.get_mode(ATD, &mode);
    if (mode.cells_valid) {
        memset_step((void*)mode.cells + 1, color, mode.width * mode.height, 2);
    } else {
        for (u32 y = 0; y < mode.height; y++) {
            for (u32 x = 0; x < mode.width; x++) {
                u8 current_char = ATD->functions.get_char(ATD, x, y) >> 8;
                ATD->functions.set_char(ATD, x, y, current_char, color);
            }
        }
    }
}

/**
 * Print a char* to the screen.
 * @param str The char* to print.
 */
void print(char* str) { __append_string__(str); }

/**
 * Print a char* to the screen with
 * a specified color.
 * @param str The char* to print.
 * @param color The color to print in.
 */
void print_color(char* str, const u8 color) { __append_string_color__(str, color); }

/**
 * Print a char* to the screen with an
 * extra line after.
 * @param str The char* to print.
 */
void println(char* str) {
    __append_string__(str);
    __append_newline__();
}

/**
 * Print a char* to the screen with a
 * specified color and an extra line after.
 * @param str The char* to print.
 * @param color The color to print it in.
 */
void println_color(char* str, const u8 color) {
    __append_string_color__(str, color);
    __append_newline__();
}

/**
 * Prints a char* to the screen with
 * fmt-str data like numbers, strings, pointers.
 * @param fmt The format char* to print.
 * @param ... The variadic arguments to print.
 */
void printf(const char* fmt, ...) {
    va_list args;
    va_start(args, 0);

    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%') {
            i++;
            switch (fmt[i]) {
            case 'd': {
                char* digits = itoa_signed(va_arg(args, i32), &buf[0]);
                __append_string__(digits);
                break;
            }
            case 'x': {
                char* digits = xtoa(va_arg(args, u32), &buf[0]);
                __append_string__(digits);
                break;
            }
            case 'p': {
                __append_string__("0x");
                char* digits = xtoa_padded(va_arg(args, u32), &buf[0]);
                __append_string__(digits);
                break;
            }
            case 's': {
                __append_string__(va_arg(args, char*));
                break;
            }
            case 'c': {
                char arg = va_arg(args, i32);
                if (arg == '\n') __append_newline__();
                if (arg == 0x08) __append_backspace__();
                else __append_char__(arg);
                break;
            }
            case 'u': {
                char* digits = itoa(va_arg(args, u32), &buf[0]);
                __append_string__(digits);
                break;
            }
            default: {
                __append_char__('%');
                __append_char__(fmt[i]);
                break;
            }
            }
        }
        else if (fmt[i] == '\n') { __append_newline__(); }
        else if (fmt[i] == 0x08) { __append_backspace__(); }
        else { __append_char__(fmt[i]); }
    }

    va_end(args);
}