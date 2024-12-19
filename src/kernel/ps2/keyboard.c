//
// Created by Adithiya Venkatakrishnan on 14/12/2024.
//

#include "keyboard.h"

#include <display/display.h>
#include <exception/exception.h>
#include <modules/modules.h>
#include <idt/interrupt.h>
#include <timer/PIT.h>

#define wait_for(_cond) while (!(_cond)) { NOP(); }
#define delay() \
    for (int i = 0; i < 1000000; i++) { NOP(); }

const bool DEBUG = false;

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
                printf("%c", buffer[i]);
                buffer[i] = 0;
                i--;
                buffer[i] = 0;
            }
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
    wait_for(verify_keyboard_ready_read());
    u16 scancode = inportb(KEYBOARD_DATA_PORT);
    u8 hscancode = (u8)(scancode & 0xFF);
    u8 lscancode = scancode; // (u8)(scancode >> 8);

    // printf("Key was detected: %x", scancode);

    if (hscancode == KEYBOARD_EXTENDED) {
        // I don't wanna deal with this right now
        return;
    }

    if (KEY_PRESSED(lscancode)) {
        switch (KEY_SCANCODE(lscancode)) {
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
            ][KEY_SCANCODE(lscancode)];
            if (character != 0) {
                // print the character
                // printf("character %c was pressed\n", character);
                state.current_char = character;
                state.current_key = KEY_SCANCODE(lscancode);
                state.pressed = true;

                state.tick = !state.tick;
            }
        }
    }
    else {
        switch (KEY_SCANCODE(lscancode)) {
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

u8 send_keyboard_command(u8 byte) {
    do {
        wait_for(verify_keyboard_ready_write());
        outportb(KEYBOARD_DATA_PORT, byte);
        delay();
    } while (inportb(KEYBOARD_DATA_PORT) == 0xFE);
    if (!verify_keyboard_response(inportb(KEYBOARD_DATA_PORT))) panic("keyboard sucks (send_keyboard_command failed)");
    return inportb(KEYBOARD_DATA_PORT);
}

u8 send_keyboard_command_word(u8 byte, u8 sub) {
    send_keyboard_command(byte);
    u8 ret = send_keyboard_command(sub);

    return ret;
}

void disable_keyboard() {
    send_keyboard_command(0xF5);
}

void enable_keyboard() {
    send_keyboard_command(0xF4);
}

void keyboard_init() {
    disable_keyboard();


    // u8 bytein = 0, retries = 0;
    //
    // // Disable ps/2 port 1
    // do {
    //     wait_for(verify_keyboard_ready_write());
    //     outportb(KEYBOARD_CMD_PORT, 0xAD);
    //     bytein = inportb(KEYBOARD_DATA_PORT);
    // } while (bytein == 0xFE && retries++ < 3);
    // if (!verify_keyboard_response(bytein)) {
    //     panic("keyboard failed to respond to reset command");
    // }
    // printf("disabled: %x\n", bytein);
    // retries = 0;
    //
    // // Disable ps/2 port 2 (superfluous but just in case)
    // do {
    //     wait_for(verify_keyboard_ready_write());
    //     outportb(KEYBOARD_CMD_PORT, 0xA7);
    //     bytein = inportb(KEYBOARD_DATA_PORT);
    // } while (bytein == 0xFE && retries++ < 3);
    // if (!verify_keyboard_response(bytein)) {
    //     panic("keyboard failed to respond to reset command");
    // }
    // retries = 0;
    //
    // // flush the output buffer
    // while (verify_keyboard_ready_read()) {
    //     inportb(KEYBOARD_DATA_PORT);
    // }
    //
    // // read configuration byte
    // u8 config = read_configuration_byte();
    // printf("keyboard configuration: 0x%x\n", config);
    //
    // // set specific bits
    // config = BIT_SET(config, 0, 0); // disable keyboard irq
    // config = BIT_SET(config, 6, 0); // disable keyboard translation
    //
    // // write configuration byte
    // write_configuration_byte(config);
    // printf("written configuration byte: 0x%x\n", read_configuration_byte());
    //
    // // perform controller self-test
    // do {
    //     wait_for(verify_keyboard_ready_write());
    //     outportb(KEYBOARD_CMD_PORT, 0xAA);
    //     wait_for(verify_keyboard_ready_read());
    //     bytein = inportb(KEYBOARD_DATA_PORT);
    // } while (bytein == 0xFE && retries++ < 3);
    // if (!verify_keyboard_response(bytein)) {
    //     panic("keyboard failed self-test");
    // }
    // if (bytein != 0x55) {
    //     panic("keyboard failed self-test");
    // }
    //
    // printf("passed self-test\n");
    //
    // // restore controller configuration byte
    // // write configuration byte
    // do {
    //     wait_for(verify_keyboard_ready_write());
    //     outportb(KEYBOARD_CMD_PORT, 0x60);
    //     bytein = inportb(KEYBOARD_DATA_PORT);
    // } while (bytein == 0xFE && retries++ < 3);
    // if (!verify_keyboard_response(bytein)) {
    //     panic("keyboard failed to set configuration byte");
    // }
    // retries = 0;
    // do {
    //     wait_for(verify_keyboard_ready_write());
    //     outportb(KEYBOARD_DATA_PORT, config);
    //     bytein = inportb(KEYBOARD_DATA_PORT);
    // } while (bytein == 0xFE && retries++ < 3);
    // if (!verify_keyboard_response(bytein)) {
    //     panic("keyboard failed to set configuration byte");
    // }
    //
    // // enable ps/2 port 1
    // do {
    //     wait_for(verify_keyboard_ready_write());
    //     outportb(KEYBOARD_CMD_PORT, 0xAE);
    //     bytein = inportb(KEYBOARD_DATA_PORT);
    // } while (bytein == 0xFE && retries++ < 3);
    // if (!verify_keyboard_response(bytein)) {
    //     panic("keyboard failed to respond to reset command");
    // }
    //
    // // set scan code set to 2
    // do {
    //     wait_for(verify_keyboard_ready_write());
    //     outportb(KEYBOARD_DATA_PORT, 0xF0);
    //     bytein = inportb(KEYBOARD_DATA_PORT);
    // } while (bytein == 0xFE && retries++ < 3);
    // if (!verify_keyboard_response(bytein)) {
    //     panic("keyboard failed to respond to reset command");
    // }
    // retries = 0;
    //
    // do {
    //     wait_for(verify_keyboard_ready_write());
    //     outportb(KEYBOARD_DATA_PORT, 0x01);
    //     bytein = inportb(KEYBOARD_DATA_PORT);
    // } while (bytein == 0xFE && retries++ < 3);
    // if (!verify_keyboard_response(bytein)) {
    //     panic("keyboard failed to respond to reset command");
    // }
    // retries = 0;
    //
    // // read configuration byte
    // config = read_configuration_byte();
    // printf("keyboard configuration: 0x%x\n", config);
    //
    // // set specific bits
    // config = BIT_SET(config, 0, 1); // enable keyboard irq
    //
    // // write configuration byte
    // write_configuration_byte(config);
    // printf("written configuration byte: 0x%x\n", read_configuration_byte());

    send_keyboard_command_word(0xF0, 0x02);

    enable_keyboard();
    PIC_install(1, keyboard_handler);
}
