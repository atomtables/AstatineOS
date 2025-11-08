// src/kernel/pcspeaker/pcspeaker.c

#include "pcspeaker.h"
#include <modules/modules.h>
#include <timer/PIT.h>

extern u8 musictrack[]; // Assuming musictrack is defined elsewhere

static volatile u16 offset = 0;
static volatile bool done = false;

// Play sound using built-in speaker
void play_sound(u32 nFrequence) {
    // Set the PIT to the desired frequency
    const u32 Div = PIT_HZ / nFrequence;
    outportb(PIT_CONTROL, 0b10110110);
    outportb(PIT_C, Div);
    outportb(PIT_C, Div >> 8);

    // And play the sound using the PC speaker
    const u8 tmp = inportb(PCSPEAKER);
    if (tmp != (tmp | 3)) {
        outportb(PCSPEAKER, tmp | 3);
    }
}

// make it shut up
void nosound() {
    u8 tmp = inportb(PCSPEAKER) & 0xFC;
    outportb(PCSPEAKER, tmp);
}

// Make a beep
void beep() {
    play_sound(587);
    wait_and_do(50, nosound);
    // set_PIT_2(old_frequency);
}

void pcs_play_8bit(u8 *data, u32 length) {
    for (u32 i = 0; i < length; i++) {
        sleep(1);
        u8 bit = data[i] & 0b01111111;
        outportb(0x43, 0xb0);
        outportb(0x42, bit);
        outportb(0x43, 0);
    }
}

void pcs_init() {
    // connect the pc speaker to the PIT
    outportb(0x61, inportb(0x61) | 3);
    nosound();
}