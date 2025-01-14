///
/// VGA Text-mode driver (display.c, display.h)
/// Created by Adithiya Venkatakrishnan on 17/07/2024.
///
/// The current implementation stays at the last line
/// during overflow. This may cause a tiny bit of performance
/// loss, but should be negligent (seeing a transition to VGA
/// color-pixel mode)
///

#ifndef DISPLAY_H
#define DISPLAY_H

#define VGA_TEXT_WIDTH  80
#define VGA_TEXT_HEIGHT 25
#define VGA_TEXT_SIZE   VGA_TEXT_WIDTH * VGA_TEXT_HEIGHT

#define VGA_TEXT_PAGING_WIDTH  80
#define VGA_TEXT_PAGING_HEIGHT 48
#define VGA_TEXT_PAGING_SIZE   VGA_TEXT_PAGING_WIDTH * VGA_TEXT_PAGING_HEIGHT * 2

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

typedef struct {
    void(* clear_screen)(void);
    void(* change_screen_color)(u8 color);
    void(* print)(char* str);
    void(* print_color)(char* str, u8 color);
    void(* println)(char* str);
    void(* println_color)(char* str, u8 color);
    void(* printf)(const char* fmt, ...);
} PDisplay;

extern PDisplay display;

#endif //DISPLAY_H
