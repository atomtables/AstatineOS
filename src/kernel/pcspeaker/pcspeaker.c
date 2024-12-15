//
// Created by Adithiya Venkatakrishnan on 6/11/2024.
//

#include "pcspeaker.h"

#include <modules/modules.h>
#include <timer/PIT.h>

//Play sound using built-in speaker
static void play_sound(u32 nFrequence) {
    //Set the PIT to the desired frequency
    const u32 Div = PIT_HZ / nFrequence;
    outportb(PIT_CONTROL, 0b10110110);
    outportb(PIT_C, Div );
    outportb(PIT_C, Div >> 8);
 
    //And play the sound using the PC speaker
    const u8 tmp = inportb(PCSPEAKER);
    if (tmp != (tmp | 3)) {
        outportb(PCSPEAKER, tmp | 3);
    }
}
 
//make it shut up
static void nosound() {
    u8 tmp = inportb(PCSPEAKER) & 0xFC;
    outportb(PCSPEAKER, tmp);
}
 
//Make a beep
void beep() {
    play_sound(587);
    wait_and_do(500, nosound);
    //set_PIT_2(old_frequency);
}

void pcs_init() {
    // connect the pc speaker to the PIT
    outportb(0x61, inportb(0x61) | 3);
    play_sound(440);
    wait_and_do(1, nosound);
}