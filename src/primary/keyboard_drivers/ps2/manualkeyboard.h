//
// Created by Adithiya Venkatakrishnan on 20/12/2024.
//

#ifndef MANUALKEYBOARD_H
#define MANUALKEYBOARD_H

struct keyboard_advanced_state {
    bool caps_lock : 1;
    bool num_lock : 1;
    bool scroll_lock : 1;
    bool shift : 1;
    bool ctrl : 1;
    bool alt : 1;
    bool meta : 1; // not applicable yet
    bool fn : 1; // not applicable yet
    u8 keys[128];
    u8 chars[128];
};

extern volatile struct keyboard_advanced_state keyboard;

#define keyboard_key(_s) (keyboard.keys[(_s)])
#define keyboard_char(_c) (keyboard.chars[(_c)])

#define wait_for_key_release(_c) while (keyboard.chars[(_c)]) { NOP(); }

#endif //MANUALKEYBOARD_H
