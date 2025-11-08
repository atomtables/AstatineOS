//
// Created by Adithiya Venkatakrishnan on 6/11/2024.
//

#ifndef PCSPEAKER_H
#define PCSPEAKER_H

#define PCSPEAKER 0x61

#define NOTE_C0 16
#define NOTE_Cs0 17
#define NOTE_D0 18
#define NOTE_Ds0 19
#define NOTE_E0 21
#define NOTE_F0 22
#define NOTE_Fs0 23
#define NOTE_G0 25
#define NOTE_Gs0 26
#define NOTE_A0 28
#define NOTE_As0 29
#define NOTE_B0 31

#define NOTE_C1 33
#define NOTE_Cs1 35
#define NOTE_D1 37
#define NOTE_Ds1 39
#define NOTE_E1 41
#define NOTE_F1 44
#define NOTE_Fs1 46
#define NOTE_G1 49
#define NOTE_Gs1 52
#define NOTE_A1 55
#define NOTE_As1 58
#define NOTE_B1 62

#define NOTE_C2 65
#define NOTE_Cs2 69
#define NOTE_D2 73
#define NOTE_Ds2 78
#define NOTE_E2 82
#define NOTE_F2 87
#define NOTE_Fs2 93
#define NOTE_G2 98
#define NOTE_Gs2 104
#define NOTE_A2 110
#define NOTE_As2 117
#define NOTE_B2 123

#define NOTE_C3 131
#define NOTE_Cs3 139
#define NOTE_D3 147
#define NOTE_Ds3 156
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_Fs3 185
#define NOTE_G3 196
#define NOTE_Gs3 208
#define NOTE_A3 220
#define NOTE_As3 233
#define NOTE_B3 247

#define NOTE_C4 262
#define NOTE_Cs4 277
#define NOTE_D4 294
#define NOTE_Ds4 311
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_Fs4 370
#define NOTE_G4 392
#define NOTE_Gs4 415
#define NOTE_A4 440
#define NOTE_As4 466
#define NOTE_B4 494

#define NOTE_C5 523
#define NOTE_Cs5 554
#define NOTE_D5 587
#define NOTE_Ds5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_Fs5 740
#define NOTE_G5 784
#define NOTE_Gs5 831
#define NOTE_A5 880
#define NOTE_As5 932
#define NOTE_B5 988

#define NOTE_C6 1047
#define NOTE_Cs6 1109
#define NOTE_D6 1175
#define NOTE_Ds6 1245
#define NOTE_E6 1319
#define NOTE_F6 1397
#define NOTE_Fs6 1480
#define NOTE_G6 1568
#define NOTE_Gs6 1661
#define NOTE_A6 1760
#define NOTE_As6 1865
#define NOTE_B6 1976

#define NOTE_C7 2093
#define NOTE_Cs7 2217
#define NOTE_D7 2349
#define NOTE_Ds7 2489
#define NOTE_E7 2637
#define NOTE_F7 2794
#define NOTE_Fs7 2960
#define NOTE_G7 3136
#define NOTE_Gs7 3322
#define NOTE_A7 3520
#define NOTE_As7 3729
#define NOTE_B7 3951

#define NOTE_C8 4186
#define NOTE_Cs8 4435
#define NOTE_D8 4699
#define NOTE_Ds8 4978
#define NOTE_E8 5274
#define NOTE_F8 5588
#define NOTE_Fs8 5920
#define NOTE_G8 6272
#define NOTE_Gs8 6645
#define NOTE_A8 7040
#define NOTE_As8 7459
#define NOTE_B8 7902

#define NOTE_REST 0
#include <modules/modules.h>

#include "pcspeaker.h"

void play_sound(u32 nFrequence);
void nosound();
void pwm_tick();

void pcs_init();
void beep();
void pcs_play_8bit(u8 *data, u32 length);

inline void pc_speaker_on() {
    u8 speaker_state = inportb(PCSPEAKER);
    outportb(PCSPEAKER, speaker_state | 0x03); // Enable speaker
}

inline void pc_speaker_off() {
    u8 speaker_state = inportb(PCSPEAKER);
    outportb(PCSPEAKER, speaker_state & ~0x03); // Disable speaker
}

#endif //PCSPEAKER_H
