//
// Created by Adithiya Venkatakrishnan on 14/12/2024.
//

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <modules/modules.h>

#define KEY_NULL    0x00
#define KEY_ESC     0x1B
#define KEY_BS      0x08
#define KEY_TAB     0x09
#define KEY_LF      0x0A
#define KEY_CR      0x0D

// these are wrong i'm pretty sure
#define KEY_INSERT  0x90
#define KEY_DELETE  0x91
#define KEY_HOME    0x92
#define KEY_END     0x93
#define KEY_PG_UP   0x94
#define KEY_PG_DN   0x95

//
#define KEY_UP      0x48
#define KEY_LEFT    0x4B
#define KEY_RIGHT   0x4D
#define KEY_DOWN    0x50

#define KEY_F1      0x80
#define KEY_F2      (KEY_F1 + 1)
#define KEY_F3      (KEY_F1 + 2)
#define KEY_F4      (KEY_F1 + 3)
#define KEY_F5      (KEY_F1 + 4)
#define KEY_F6      (KEY_F1 + 5)
#define KEY_F7      (KEY_F1 + 6)
#define KEY_F8      (KEY_F1 + 7)
#define KEY_F9      (KEY_F1 + 8)
#define KEY_F10     (KEY_F1 + 9)
#define KEY_F11     (KEY_F1 + 10)
#define KEY_F12     (KEY_F1 + 11)
#define KEY_F13     (KEY_F1 + 12)
#define KEY_F14     (KEY_F1 + 13)
#define KEY_F15     (KEY_F1 + 14)
#define KEY_F16     (KEY_F1 + 15)

#define KEY_LCTRL   0x1D
#define KEY_RCTRL   0x1D

#define KEY_LALT    0x38
#define KEY_RALT    0x38

#define KEY_LSHIFT  0x2A
#define KEY_RSHIFT  0x36

#define KEY_LMETA   0x5B

#define KEY_CAPS_LOCK   0x3A
#define KEY_SCROLL_LOCK 0x46
#define KEY_NUM_LOCK    0x45

#define KEY_MOD_ALT         0x0200
#define KEY_MOD_CTRL        0x0400
#define KEY_MOD_SHIFT       0x0800
#define KEY_MOD_CAPS_LOCK   0x1000
#define KEY_MOD_NUM_LOCK    0x2000
#define KEY_MOD_SCROLL_LOCK 0x4000

#define KEYBOARD_CMD_PORT   0x64
#define KEYBOARD_DATA_PORT  0x60
#define KEYBOARD_RELEASE    0x80
#define KEYBOARD_EXTENDED   0xE0

#define KEY_PRESSED(_s) (!((_s) & KEYBOARD_RELEASE))
#define KEY_RELEASED(_s) (!!((_s) & KEYBOARD_RELEASE))
#define KEY_SCANCODE(_s) ((_s) & 0x7F)

#define CHAR_PRINTABLE(_c) ((_c) >= 0x20 && (_c) <= 0x7E)
#define CHAR_NONPRINTABLE(_c) ((_c) < 0x20 || (_c) == 0x7F)
#define CHAR_SPECIAL(_c) ((_c) > 0x7F)

void    keyboard_init();

u8      wait_for_keypress();
char*  input(char* buffer, u32 size);

#endif //KEYBOARD_H
