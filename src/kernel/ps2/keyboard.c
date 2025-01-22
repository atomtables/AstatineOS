//
// Created by Adithiya Venkatakrishnan on 14/12/2024.
//

#include "keyboard.h"
#include "manualkeyboard.h"

#include <display/simple/display.h>
#include <exception/exception.h>
#include <modules/modules.h>
#include <interrupt/interrupt.h>
#include <timer/PIT.h>


bool    keyboard_irq_enabled = false;
u8      keyboard_current_scancode = 0x00;
bool    keyboard_translation_enabled = false;

#define delay() \
    for (int i = 0; i < 1000000; i++) { NOP(); }

static const bool DEBUG = false;

static u8 keyboard_layout_us[2][128] = {
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

bool verify_keyboard_ready_read() {
    return BIT_GET(inportb(KEYBOARD_CMD_PORT), 0) == 1;
}

bool verify_keyboard_ready_write() {
    return BIT_GET(inportb(KEYBOARD_CMD_PORT), 1) == 0;
}

bool verify_keyboard_response(u8 response) {
    if (response == 0xFE || response == 0x00 || response == 0xFF) {
        return false;
    }
    return true;
}

struct keyboard_state simple_state;

volatile struct keyboard_advanced_state keyboard;

u8 wait_for_keypress() {
    bool tick = simple_state.tick;
    while (simple_state.tick == tick) { NOP(); }
    return simple_state.current_char;
}

char* input(char* buffer, u32 size) {
    u32 i = 0;
    while (i < size) {
        buffer[i] = (char)wait_for_keypress();
        if (buffer[i] == KEY_LF) {
            buffer[i] = 0;
            break;
        }
        if (buffer[i] == KEY_BS) {
            if (i > 0) {
                display.printf("%c", buffer[i]);
                buffer[i] = 0;
                i--;
                buffer[i] = 0;
            }
            continue;
        }
        display.printf("%c", buffer[i]);
        i++;
    }
    buffer[i] = 0;
    display.printf("\n");
    return buffer;
}

void keyboard_handler(struct registers* regs) {
    // get the character from the keyboard
    // first 8 bits are the scancode, last 8 bits is the pressed/released bit
    wait_for(verify_keyboard_ready_read());
    u8 scancode = inportb(KEYBOARD_DATA_PORT);

    // printf("Key was detected: %x", scancode);

    if (scancode == KEYBOARD_EXTENDED) {
        // I don't wanna deal with this right now
        return;
    }

    keyboard.keys[KEY_SCANCODE(scancode)] = KEY_PRESSED(scancode);
    keyboard.chars[KEY_CHAR(scancode)] = KEY_PRESSED(scancode);
    if (DEBUG) display.printf("scancode: %x\n", scancode);

    if (KEY_PRESSED(scancode)) {
        switch (KEY_SCANCODE(scancode)) {
        case KEY_LCTRL:
            simple_state.ctrl = true;
            keyboard.ctrl = true;
            if (DEBUG) display.printf("ctrl was pressed\n");
            break;
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            simple_state.shift = true;
            keyboard.shift = true;
            if (DEBUG) display.printf("shift was pressed\n");
            break;
        case KEY_LALT:
            simple_state.alt = true;
            keyboard.alt = true;
            if (DEBUG) display.printf("alt was pressed\n");
            break;
        case KEY_CAPS_LOCK:
            simple_state.caps_lock = !simple_state.caps_lock;
            keyboard.caps_lock = !keyboard.caps_lock;
            if (DEBUG) display.printf("caps lock was pressed\n");
            break;
        case KEY_SCROLL_LOCK:
            simple_state.scroll_lock = !simple_state.scroll_lock;
            keyboard.scroll_lock = !keyboard.scroll_lock;
            if (DEBUG) display.printf("scroll lock was pressed\n");
            break;
        case KEY_NUM_LOCK:
            simple_state.num_lock = !simple_state.num_lock;
            keyboard.num_lock = !keyboard.num_lock;
            if (DEBUG) display.printf("num lock was pressed\n");
            break;
        case KEY_UP:
        case KEY_DOWN:
        case KEY_LEFT:
        case KEY_RIGHT:
            break;
        default:
            // get the character from the keyboard
            char character = keyboard_layout_us[
                simple_state.caps_lock ^ simple_state.shift ? 1 : 0
            ][KEY_SCANCODE(scancode)];
            if (character != 0) {
                // print the character
                // printf("character %c was pressed\n", character);
                simple_state.current_char = character;
                simple_state.current_key = KEY_SCANCODE(scancode);
                simple_state.pressed = true;

                simple_state.tick = !simple_state.tick;
            }
        }
    }
    else {
        switch (KEY_SCANCODE(scancode)) {
        case KEY_LCTRL:
            simple_state.ctrl = false;
            keyboard.ctrl = false;
            if (DEBUG) display.printf("ctrl was released\n");
            break;
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            simple_state.shift = false;
            keyboard.shift = false;
            if (DEBUG) display.printf("shift was released\n");
            break;
        case KEY_LALT:
            simple_state.alt = false;
            keyboard.alt = false;
            if (DEBUG) display.printf("alt was released\n");
            break;
        default:
            // simple_state.current_char = 0;
            // simple_state.current_key = 0;
            simple_state.pressed = false;
        }

        // this doesn't matter as much unless we're making a game where its important to know if a key is held down
    }

    // printf("Scancode: %x, Pressed: \n", scancode);
}


u8 send_keyboard_command(u8 byte) {
    u8 input;
    do {
        wait_for(verify_keyboard_ready_write());
        outportb(KEYBOARD_DATA_PORT, byte);
        delay();
        input = inportb(KEYBOARD_DATA_PORT);
    } while (input != 0xFA && input == 0xFE);
    if (!verify_keyboard_response(inportb(KEYBOARD_DATA_PORT))) panic("keyboard sucks (send_keyboard_command failed)");
    return inportb(KEYBOARD_DATA_PORT);
}

u8 send_keyboard_command_word(u8 byte, u8 sub) {
    send_keyboard_command(byte);
    u8 ret = send_keyboard_command(sub);

    return ret;
}

void keyboard_init() {
    memset(&simple_state, 0, sizeof(struct keyboard_state));
    memset((void*)&keyboard, 0, sizeof(struct keyboard_advanced_state));

    send_keyboard_command(KEYCMD_DISABLE); // has the side effect of possibly resetting the keyboard anyway so that's good

    send_keyboard_command_word(0xF0, 0x02); // since 0x02 is guaranteed to be supported on all keyboards
    keyboard_current_scancode = 0x02;

    send_keyboard_command(KEYCMD_ENABLE);
    PIC_install(1, keyboard_handler);
}
