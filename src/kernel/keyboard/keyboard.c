//
// Created by Adithiya Venkatakrishnan on 14/12/2024.
//

#include "keyboard.h"

#include <display/display.h>
#include <modules/modules.h>
#include <idt/interrupt.h>
#include <timer/PIT.h>

const bool DEBUG = false;

u8 keyboard_layout_us[2][128] = {
    {
        KEY_NULL,
        KEY_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', KEY_BS, KEY_TAB,
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', KEY_LF, 0,
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
        'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0, ' ', 0,
        KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
        KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, 0, 0, KEY_HOME, KEY_UP,
        KEY_PG_UP, '-', KEY_LEFT, '5', KEY_RIGHT, '+', KEY_END, KEY_DOWN,
        KEY_PG_DN, KEY_INSERT, KEY_DELETE, 0, 0, 0, KEY_F11, KEY_F12
    },
    {
        KEY_NULL,
        KEY_ESC, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', KEY_BS,
        KEY_TAB, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', KEY_LF,
        0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0, '|', 'Z', 'X', 'C', 'V', 'B', 'N',
        'M', '<', '>', '?', 0, 0, 0, ' ', 0, KEY_F1, KEY_F2, KEY_F3, KEY_F4,
        KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, 0, 0, KEY_HOME, KEY_UP,
        KEY_PG_UP, '-', KEY_LEFT, '5', KEY_RIGHT, '+', KEY_END, KEY_DOWN,
        KEY_PG_DN, KEY_INSERT, KEY_DELETE, 0, 0, 0, KEY_F11, KEY_F12
    }
};

struct keyboard_state {
    bool caps_lock : 1;
    bool num_lock : 1;
    bool scroll_lock : 1;
    bool shift : 1;
    bool ctrl : 1;
    bool alt : 1;
    bool meta : 1;
    u8 current_key;
    u8 current_char;
    bool pressed : 4;
    bool tick : 4; // on every keypress, this will be toggled to indicate a new keypress
} state;

u8 wait_for_keypress() {
    bool tick = state.tick;
    while (state.tick == tick) { NOP(); }
    return state.current_char;
}

string input(string buffer, u32 size) {
    u32 i = 0;
    while (i < size) {
        buffer[i] = (char)wait_for_keypress();
        if (buffer[i] == KEY_LF) {
            buffer[i] = 0;
            break;
        }
        if (buffer[i] == KEY_BS) {
            printf("%c", buffer[i]);
            buffer[i] = 0;
            if (i > 0) i--;
            buffer[i] = 0;
            continue;
        }
        printf("%c", buffer[i]);
        i++;
    }
    printf("\n");
    return buffer;
}

void keyboard_handler(struct registers* regs) {
    // get the character from the keyboard
    // first 8 bits are the scancode, last 8 bits is the pressed/released bit
    u16 scancode = inportw(KEYBOARD_PORT);

    if (scancode == KEYBOARD_EXTENDED) {
        // I don't wanna deal with this right now
        return;
    }

    if (KEY_PRESSED(scancode)) {
        switch (KEY_SCANCODE(scancode)) {
        case KEY_LCTRL:
            state.ctrl = true;
            if (DEBUG) printf("ctrl was pressed\n");
            break;
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            state.shift = true;
            if (DEBUG) printf("shift was pressed\n");
            break;
        case KEY_LALT:
            state.alt = true;
            if (DEBUG) printf("alt was pressed\n");
            break;
        case KEY_CAPS_LOCK:
            state.caps_lock = !state.caps_lock;
            if (DEBUG) printf("caps lock was pressed\n");
            break;
        case KEY_SCROLL_LOCK:
            state.scroll_lock = !state.scroll_lock;
            if (DEBUG) printf("scroll lock was pressed\n");
            break;
        case KEY_NUM_LOCK:
            state.num_lock = !state.num_lock;
            if (DEBUG) printf("num lock was pressed\n");
            break;
        default:
            // get the character from the keyboard
            u8 character = keyboard_layout_us[
                state.caps_lock ^ state.shift ? 1 : 0
            ][KEY_SCANCODE(scancode)];
            if (character != 0) {
                // print the character
                //printf("character %c was pressed\n", character);
                state.current_char = character;
                state.current_key = KEY_SCANCODE(scancode);
                state.pressed = true;

                state.tick = !state.tick;
            }
        }
    }
    else {
        switch (KEY_SCANCODE(scancode)) {
        case KEY_LCTRL:
            state.ctrl = false;
            if (DEBUG) printf("ctrl was released\n");
            break;
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            state.shift = false;
            if (DEBUG) printf("shift was released\n");
            break;
        case KEY_LALT:
            state.alt = false;
            if (DEBUG) printf("alt was released\n");
            break;
        default:
            state.current_char = 0;
            state.current_key = 0;
            state.pressed = false;
        }

        // this doesn't matter as much unless we're making a game where its important to know if a key is held down
    }

    // printf("Scancode: %x, Pressed: \n", scancode);
}

void keyboard_init() {
    u8 bytein, retries = 0;
    do {
        outportb(KEYBOARD_PORT, 0xF0);
        outportb(KEYBOARD_DATA_PORT, 0x01);
        bytein = inportb(KEYBOARD_PORT);
        retries++;
    } while (bytein == 0xFE || retries < 3);
    if (bytein != 0xFA) {
        printf("Keyboard setup failed...\n");
        sleep(500);
        FATALERROR();
    }

    PIC_install(1, keyboard_handler);
}
